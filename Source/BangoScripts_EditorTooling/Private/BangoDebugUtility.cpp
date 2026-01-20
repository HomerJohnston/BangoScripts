// Copyright Ghost Pepper Games, Inc. All Rights Reserved.

#include "BangoScripts/EditorTooling/BangoDebugUtility.h"

#include "DrawDebugHelpers.h"
#include "IImageWrapperModule.h"
#include "PixelFormat.h"
#include "TextureResource.h"
#include "Components/ActorComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IPluginManager.h"
#include "Math/MathFwd.h"
#include "Math/Ray.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"

// ----------------------------------------------

UTexture2D* Bango::Debug::GetScriptDebugDrawIcon()
{
    static TStrongObjectPtr<UTexture2D> IconTexture = nullptr;
    
    static bool bLoaded = false;
    
    if (!bLoaded)
    {
        // Only attempt to load it once
        LoadIcon(IconTexture, "ViewportIcons/Icon_Script_Small.png");
        bLoaded = true;
    }
    
    return IconTexture.Get();
}

// ----------------------------------------------

UTexture2D* Bango::Debug::GetScriptBillboardIcon()
{
    static TStrongObjectPtr<UTexture2D> IconTexture = nullptr;
    
    static bool bLoaded = false;
    
    if (!bLoaded)
    {
        // Only attempt to load it once
        LoadIcon(IconTexture, "ViewportIcons/Icon_Script_BillboardComponent.png");
        bLoaded = true;
    }
    
    return IconTexture.Get();
}

// ----------------------------------------------

void Bango::Debug::LoadIcon(TStrongObjectPtr<UTexture2D>& Destination, const FString& Path)
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("BangoScripts"));
    check(Plugin.IsValid());

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    
    const FString ResourcesDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources")));
	
    TArray<uint8> PNGData;
    FFileHelper::LoadFileToArray(PNGData, *(ResourcesDir / *Path));

    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
    
    ImageWrapper->SetCompressed(PNGData.GetData(), PNGData.Num());
    
    UTexture2D* Texture = UTexture2D::CreateTransient(
           ImageWrapper->GetWidth(),
           ImageWrapper->GetHeight(),
           PF_B8G8R8A8
       );

    TArray<uint8> RawData;
    ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);
    
    Texture->MipGenSettings = TMGS_NoMipmaps;
    Texture->SRGB = true;
    
    void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
    Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
    
    Texture->UpdateResource();
    
    Destination = TStrongObjectPtr<UTexture2D>(Texture);
}

// ----------------------------------------------

void Bango::Debug::PrintComponentState(UActorComponent* Component, FString Msg)
{
	uint32 Flags = (uint32)Component->GetFlags();
	uint32 IntFlags = (uint32)Component->GetInternalFlags();
	
	FString BitString1;
	FString BitString2;
	BitString1.Reserve(32);
	BitString2.Reserve(32);

	while (Msg.Len() < 64)
	{
		Msg += " ";
	}
	
	for (int32 i = 31; i >= 0; --i)
	{
		BitString1.AppendChar(((Flags >> i) & 1) ? TEXT('1') : TEXT('0'));
		BitString2.AppendChar(((IntFlags >> i) & 1) ? TEXT('1') : TEXT('0'));
	}
	
	UE_LOG(LogBangoEditor, VeryVerbose, TEXT("%s: %s"), *Component->GetName(), *FString::Format(TEXT("{0} --- Flags: {1}                          --- Internal Flags: {2}"), { Msg, *BitString1, *BitString2 } ));
}

// ----------------------------------------------

void Bango::Debug::PrintFlagNames()
{
	static const TMap<int64, FString> FlagDefs =
	{
		{ RF_Public,						TEXT("RF_Public") },
		{ RF_Standalone,					TEXT("RF_Standalone") },
		{ RF_MarkAsNative,					TEXT("RF_MarkAsNative") },
		{ RF_Transactional,					TEXT("RF_Transactional") },
		{ RF_ClassDefaultObject,			TEXT("RF_ClassDefaultObject") },
		{ RF_ArchetypeObject,				TEXT("RF_ArchetypeObject") },
		{ RF_Transient,						TEXT("RF_Transient") },
		{ RF_MarkAsRootSet,					TEXT("RF_MarkAsRootSet") },
		{ RF_TagGarbageTemp,				TEXT("RF_TagGarbageTemp") },
		{ RF_NeedInitialization,			TEXT("RF_NeedInitialization") },
		{ RF_NeedLoad,						TEXT("RF_NeedLoad") },
		{ RF_NeedPostLoad,					TEXT("RF_NeedPostLoad") },
		{ RF_NeedPostLoadSubobjects,		TEXT("RF_NeedPostLoadSubobjects") },
		{ RF_NewerVersionExists,			TEXT("RF_NewerVersionExists") },
		{ RF_BeginDestroyed,				TEXT("RF_BeginDestroyed") },	
		{ RF_FinishDestroyed,				TEXT("RF_FinishDestroyed") },
		{ RF_BeingRegenerated,				TEXT("RF_BeingRegenerated") },
		{ RF_DefaultSubObject,				TEXT("RF_DefaultSubObject") },
		{ RF_WasLoaded,						TEXT("RF_WasLoaded") },
		{ RF_TextExportTransient,			TEXT("RF_TextExportTransient") },
		{ RF_LoadCompleted,					TEXT("RF_LoadCompleted") },
		{ RF_InheritableComponentTemplate,	TEXT("RF_InheritableComponentTemplate") },
		{ RF_DuplicateTransient,			TEXT("RF_DuplicateTransient") },
		{ RF_StrongRefOnFrame,				TEXT("RF_StrongRefOnFrame") },
		{ RF_NonPIEDuplicateTransient,		TEXT("RF_NonPIEDuplicateTransient") },
		{ RF_ImmutableDefaultObject,		TEXT("RF_ImmutableDefaultObject") },
		{ RF_WillBeLoaded,					TEXT("RF_WillBeLoaded") },
		{ RF_HasExternalPackage,			TEXT("RF_HasExternalPackage") },
//		{ RF_MigratingAsset,				TEXT("RF_MigratingAsset") }, // 5.7 onward only
		{ RF_MirroredGarbage,				TEXT("RF_MirroredGarbage") },
		{ RF_AllocatedInSharedPage,			TEXT("RF_AllocatedInSharedPage") },
	};
			
	static const TMap<int64, FString> InternalFlagDefs = 
	{
		{ 1u << 14, TEXT("ReachabilityFlag0") },
		{ 1u << 15, TEXT("ReachabilityFlag1") },
		{ 1u << 16, TEXT("ReachabilityFlag2") },
		{ 1u << 17, TEXT("AutoRTFMConstructionAborted") },
		{ 1u << 20, TEXT("LoaderImport") },
		{ 1u << 21, TEXT("Garbage") },
		{ 1u << 22, TEXT("AsyncLoadingPhase1") },
		{ 1u << 23, TEXT("ReachableInCluster") },
		{ 1u << 24, TEXT("ClusterRoot") },
		{ 1u << 25, TEXT("Native") },
		{ 1u << 26, TEXT("Async") },
		{ 1u << 27, TEXT("AsyncLoadingPhase2") },
		{ 1u << 28, TEXT("Unreachable") },
		{ 1u << 29, TEXT("RefCounted") },
		{ 1u << 30, TEXT("RootSet") },
		{ 1u << 31, TEXT("PendingConstruction") },
	};
			
	for (int64 i = 0; i <= 30; ++i)
	{
		FString Print;
		Print = Print.LeftPad(89);
		FString Indent;
		Indent = FString::ChrN(31 - i, '|');
		FString Pointout = "o-";

		FString FinalPrint = Print + Indent + Pointout;
				
		const FString* Value = FlagDefs.Find((uint64)1 << i);
				
		if (Value)
		{
			FinalPrint += *Value;
		}
		else
		{
			FinalPrint += "NONE";
		}
			
		UE_LOG(LogBangoEditor, Display, TEXT("%s"), *FinalPrint);
	}
}

// ----------------------------------------------

void Bango::Debug::Draw::DebugDrawDashedLine(UWorld* World, const FVector& Start, const FVector& End, float DashLength, const FColor& Color, bool bPersistentLines, float Lifetime, uint8 DepthPriority, float Thickness)
{
	float Distance = (End - Start).Size();

	FRay Ray(Start, End - Start);
	
	DebugDrawDashedLine(World, Ray, Distance, DashLength, Color, bPersistentLines, Lifetime, DepthPriority, Thickness);
}

// ----------------------------------------------

void Bango::Debug::Draw::DebugDrawDashedLine(UWorld* World, const FVector& Start, const FVector& End, int32 NumDashes, const FColor& Color, bool bPersistentLines, float Lifetime, uint8 DepthPriority, float Thickness)
{
	float Distance = (End - Start).Size();

	float DashLength = Distance / NumDashes;

	FRay Ray(Start, End - Start);
	
	DebugDrawDashedLine(World, Ray, Distance, DashLength, Color, bPersistentLines, Lifetime, DepthPriority, Thickness);
}

// ----------------------------------------------

void Bango::Debug::Draw::DebugDrawDashedLine(UWorld* World, const FRay& Ray, float Distance, float DashLength, const FColor& Color, bool bPersistentLines, float Lifetime, uint8 DepthPriority, float Thickness)
{
	FVector Dir = Ray.Direction;

	if (Dir == FVector::ZeroVector)
	{
		return;
	}
	
	int32 TotalSegments = 1 + Distance / (DashLength + DashLength);
	int32 CurrentSegment = 0;

	while (CurrentSegment < TotalSegments)
	{
		FVector DashStart = Ray.Origin + CurrentSegment * (DashLength + DashLength) * Dir;

		FVector DashEnd = DashStart + DashLength * Dir;

		if ((DashEnd - Ray.Origin).SizeSquared() > FMath::Square(Distance))
		{
			DashEnd = Ray.Origin + Distance * Ray.Direction;
		}

		DrawDebugLine(World, DashStart, DashEnd, Color, bPersistentLines, Lifetime, DepthPriority, Thickness);

		CurrentSegment++;
	}
}

// ----------------------------------------------
