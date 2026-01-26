#include "BangoAsyncBillboardGenerator.h"

#include "TextureResource.h"
#include "AssetUtils/Texture2DUtil.h"
#include "Async/Async.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoScriptsEditorLog.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Image/ImageBuilder.h"

FBangoAsyncBillboardGenerator::FBangoAsyncBillboardGenerator(const TSoftObjectPtr<UTexture2D>& InOverlayTexture)
	: OverlayTexture(InOverlayTexture)
{
	
}

void FBangoAsyncBillboardGenerator::Run()
{
	check(!bRunning);
	
	bRunning = true;
	
	TWeakPtr<FBangoAsyncBillboardGenerator> _WeakThis = AsWeak();
	
	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	FStreamableAsyncLoadParams Params;
	
	FStreamableDelegate OnOverlayLoaded = FStreamableDelegate::CreateSP(this, &FBangoAsyncBillboardGenerator::OnOverlayLoaded);
	
	StreamableManager.RequestAsyncLoad(OverlayTexture.ToSoftObjectPath(), OnOverlayLoaded);
}

UTexture2D* FBangoAsyncBillboardGenerator::GetGeneratedTexture() const
{
	check(GeneratedBillboardTexture.IsValid());
	
	return GeneratedBillboardTexture.Get();
}

void FBangoAsyncBillboardGenerator::OnOverlayLoaded()
{
	check(OverlayTexture.IsValid());
	
	TSharedPtr<FBangoAsyncBillboardGenerator> SharedThis = AsShared();
	
	UE::Geometry::TImageBuilder<FVector4f> OverlayParsedImage;
	if (!UE::AssetUtils::ReadTexture(OverlayTexture.Get(), OverlayParsedImage))
	{
		UE_LOG(LogBangoEditor, Error, TEXT("Could not load overlay texture: %s"), *SharedThis->OverlayTexture.ToString());
		SharedThis->bRunning = false;
		return;
	}

	// This is always stored in a strong pointer elsewhere so I can be lax with it
	UTexture2D* Base = Bango::Debug::GetDefaultScriptBillboardSprite();
	
	int32 BaseWidth = Base->GetSizeX();
	int32 BaseHeight = Base->GetSizeY();
	
	UTexture2D* Result = UTexture2D::CreateTransient(BaseWidth, BaseHeight, PF_B8G8R8A8);
	Result->MipGenSettings = TMGS_NoMipmaps;
	Result->AddressX = TA_Clamp;
	Result->AddressY = TA_Clamp;
	Result->CompressionNone = true;
	Result->LODGroup = TEXTUREGROUP_UI;
	Result->Filter = TF_Bilinear;
	Result->SRGB = true;
	
	TStrongObjectPtr<UTexture2D> ResultStrong(Result);
    
    bool bOverlaySRGB = OverlayTexture->SRGB;
	
	GenerationTask = UE::Tasks::Launch(UE_SOURCE_LOCATION, [SharedThis, OverlayParsedImage, Base, ResultStrong, bOverlaySRGB]
	{
		FTaskTagScope Scope(ETaskTag::EParallelRenderingThread);
		
		FTexture2DMipMap& BaseMip = Base->GetPlatformData()->Mips[0];
		FColor* BasePixels = static_cast<FColor*>(BaseMip.BulkData.Lock(LOCK_READ_ONLY));

		int32 BaseWidth = Base->GetSizeX();
		int32 BaseHeight = Base->GetSizeY();

		FTexture2DMipMap& ResultMipMap = ResultStrong->GetPlatformData()->Mips[0];
		FColor* ResultPixels = static_cast<FColor*>(ResultMipMap.BulkData.Lock(LOCK_READ_WRITE));
		
		// ------------------------------------------
		// Fill it with the original data
		
		for (int32 BaseY = 0; BaseY < BaseHeight; ++BaseY)
		{
			for (int32 BaseX = 0; BaseX < BaseWidth; ++BaseX)
			{
				int32 BaseIndex = BaseX + BaseY * BaseWidth;
				ResultPixels[BaseIndex] = BasePixels[BaseIndex];
				ResultPixels[BaseIndex].R *= 0.75f; // Darken the normal icon a bit
				ResultPixels[BaseIndex].G *= 0.75f; // Darken the normal icon a bit
				ResultPixels[BaseIndex].B *= 0.75f; // Darken the normal icon a bit
				// ResultPixels[BaseIndex].A *= 0.5f; // Fade the normal icon a bit
			}
		}
		
		// ------------------------------------------
		// Build the final overlay texture
		
		int32 OverlayTextureWidth = OverlayParsedImage.GetDimensions().GetWidth();
		int32 OverlayTextureHeight = OverlayParsedImage.GetDimensions().GetHeight();
		
		int32 OverlayHorizOffset = BaseWidth * 1 / 4;
		int32 OverlayVertOffset = BaseHeight * 1 / 4;
		int32 OverlayPadding = 1.0; // We pad the top-right by a pixel to avoid border wrap issue
		
		// We need to apply overlay onto each quadrant.
		TArray<FIntPoint> Offsets = 
		{
			// Top-left overlay origin
			{ 1 * OverlayHorizOffset - OverlayPadding, 0 * OverlayVertOffset + OverlayPadding },
			
			// Top-right overlay origin
			{ 3 * OverlayHorizOffset - OverlayPadding, 0 * OverlayVertOffset + OverlayPadding },
			
			// Bottom-left overlay origin
			{ 1 * OverlayHorizOffset - OverlayPadding, 2 * OverlayVertOffset + OverlayPadding },
			
			// Bottom-right overlay origin
			{ 3 * OverlayHorizOffset - OverlayPadding, 2 * OverlayVertOffset + OverlayPadding },
		};
		
		int32 OverlayDrawWidth = BaseWidth / 4;
		int32 OverlayDrawHeight = BaseHeight / 4;
		
		for (FIntPoint BaseOffset : Offsets)
		{
			int32 OverlayXStart = BaseOffset.X;
			int32 OverlayXEnd = BaseOffset.X + OverlayDrawWidth;
			int32 OverlayYStart = BaseOffset.Y;
			int32 OverlayYEnd = BaseOffset.Y + OverlayDrawHeight;

			for (int32 PixelY = OverlayYStart; PixelY < OverlayYEnd; ++PixelY)
			{
				for (int32 PixelX = 0; PixelX < OverlayXEnd; ++PixelX)
				{
					int32 PixelIndex = PixelX + PixelY * BaseWidth;
				
					// Transfer the base pixel coordinates into overlay texure coordinates
					float OverlayXLerp = float(PixelX - OverlayXStart) / float(OverlayXEnd - OverlayXStart);
					float OverlayYLerp = float(PixelY - OverlayYStart) / float(OverlayYEnd - OverlayYStart);
				
					if (OverlayXLerp < 0.0f || OverlayXLerp > 1.0f || OverlayYLerp < 0.0f || OverlayYLerp > 1.0f)
					{
						continue;
					}
					else
					{
						int32 OverlayTexX = FMath::Clamp(OverlayXLerp * OverlayTextureWidth, 0, OverlayTextureWidth - 1);
						int32 OverlayTexY = FMath::Clamp(OverlayYLerp * OverlayTextureHeight, 0, OverlayTextureHeight - 1);
						
						// Pixel 1 = base pixel
						// Pixel 2 = overlay pixel
						FVector4f P1 = ResultPixels[PixelIndex].ReinterpretAsLinear();
						FVector4f P2 = OverlayParsedImage.GetPixel(OverlayTexX, OverlayTexY);

						// Auto-convert overlay texture to sRGB if needed (maybe I should not do this?)
						P2 = FLinearColor(FLinearColor(P2).ToFColor(bOverlaySRGB).ReinterpretAsLinear());
						
						float a2 = P2[3];
						float a1 = P1[3];

						float One = 1.0f;
						
						FVector3f C1(P1.X, P1.Y, P1.Z);
						FVector3f C2(P2.X, P2.Y, P2.Z);

						float ao = a2 + a1 * (One - a2);
						FVector3f Co;
						
						if (ao < KINDA_SMALL_NUMBER)
						{
							Co = FVector4f(0.0f);
						}
						else
						{
							Co = (C2 * a2 + C1 * a1 * (One - a2)) / ao; 
						}

						FVector4f Cof(Co.X, Co.Y, Co.Z, ao);
						
						ResultPixels[PixelIndex] = FLinearColor(Cof).ToFColor(false);
					}
				}
			}
		}
	
		BaseMip.BulkData.Unlock();
		ResultMipMap.BulkData.Unlock();
		SharedThis->GeneratedBillboardTexture = ResultStrong;
		
		AsyncTask(ENamedThreads::GameThread, [SharedThis]
		{
			SharedThis->bRunning = false;
			SharedThis->GeneratedBillboardTexture->UpdateResource();
		});
	});
}
