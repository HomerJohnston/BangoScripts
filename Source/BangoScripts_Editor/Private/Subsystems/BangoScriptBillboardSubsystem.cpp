#include "BangoScriptBillboardSubsystem.h"

#include "Editor.h"
#include "TextureResource.h"
#include "BangoScripts/EditorTooling/BangoDebugUtility.h"
#include "BangoScripts/EditorTooling/BangoEditorDelegates.h"
#include "BangoScripts/Interfaces/BangoScriptContainerObjectInterface.h"
#include "Engine/Texture2D.h"
#include "Image/ImageBuilder.h"

void UBangoScriptBillboardSubsystem::OnCustomBillboardRequested(IBangoScriptHolderInterface* Requester, const TSoftObjectPtr<UTexture2D>& OverlayTexture)
{
	if (!Requester)
	{
		return;
	}

	UObject* RequesterObject = Requester->_getUObject();
	
	if (!IsValid(RequesterObject))
	{
		return;
	}
	
	if (OverlayTexture.IsNull())
	{
		return;
	}
	
	// If this texture is already in the queue, just add this requester to its list
	FBangoBillboardRequest& Request = QueuedRequests.FindOrAdd(OverlayTexture);
	
	// Make sure the request has a generator
	if (!Request.Generator.IsValid())
	{
		Request.Generator = MakeShared<FBangoAsyncBillboardGenerator>(OverlayTexture);
	}
	
	Request.RequestingObjects.Add(RequesterObject);

	if (!TickTimerHandle.IsValid())
	{
		FTimerManagerTimerParameters Params;
		Params.bMaxOncePerFrame = true;
		Params.bLoop = true;
		Params.FirstDelay = 0.0f;
		
		auto TickDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::Tick);
		
		GEditor->GetTimerManager()->SetTimer(TickTimerHandle, TickDelegate, 0.01f, Params);
	}
}

void UBangoScriptBillboardSubsystem::Tick()
{
	// All done - stop processing
	if (QueuedRequests.IsEmpty())
	{
		GEditor->GetTimerManager()->ClearTimer(TickTimerHandle);
		return;
	}
	
	if (ActiveGenerator.IsValid())
	{
		if (ActiveGenerator->IsRunning())
		{
			// Let the active generator keep running
			return;
		}
		else
		{
			// Broadcast complete generator
			OnGenerationComplete(ActiveGenerator);
		}
	}
	
	// Activate a generator
	for (auto [SoftTexture, Request] : QueuedRequests)
	{
		ActiveGenerator = Request.Generator;
		ActiveGenerator->Run();
		break;
	}
}

void UBangoScriptBillboardSubsystem::OnGenerationComplete(TSharedPtr<FBangoAsyncBillboardGenerator> Generator)
{
	ActiveGenerator.Reset();
	
	FBangoBillboardRequest* Request = QueuedRequests.Find(Generator->GetTextureSource());
	check(Request);
	
	if (Request)
	{
		TMap<TSoftObjectPtr<UTexture2D>, TStrongObjectPtr<UTexture2D>> Copy1 = FBangoScriptBillboards::GeneratedBillboards;
		FBangoScriptBillboards::GeneratedBillboards.Add(Generator->GetTextureSource(), TStrongObjectPtr<UTexture2D>(Generator->GetGeneratedTexture()) );
		TMap<TSoftObjectPtr<UTexture2D>, TStrongObjectPtr<UTexture2D>> Copy2 = FBangoScriptBillboards::GeneratedBillboards;

		auto Array = Request->RequestingObjects.Array();
		
		for (const FObjectKey& RequesterKey : Array)
		{
			FWeakObjectPtr Requester = RequesterKey.GetWeakObjectPtr();
			if (Requester.IsValid())
			{
				IBangoScriptHolderInterface* ScriptHolder = Cast<IBangoScriptHolderInterface>(Requester.Get());
				check(ScriptHolder);
				
				ScriptHolder->UpdateBillboard();				
			}
		}
	}

	QueuedRequests.Remove(Generator->GetTextureSource());
}

void UBangoScriptBillboardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	FBangoEditorDelegates::OnCustomBillboardRequested.AddUObject(this, &ThisClass::OnCustomBillboardRequested);
	FEditorDelegates::OnMapLoad.AddLambda([] (const FString&, FCanLoadMap&) { FBangoScriptBillboards::GeneratedBillboards.Empty(); });
}

void UBangoScriptBillboardSubsystem::Deinitialize()
{
	FBangoEditorDelegates::OnCustomBillboardRequested.RemoveAll(this);
	
	Super::Deinitialize();
}

