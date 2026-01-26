#pragma once

#include "Tasks/Task.h"
#include "Templates/SharedPointer.h"
#include "UObject/SoftObjectPtr.h"

class UTexture2D;

class FBangoAsyncBillboardGenerator : public TSharedFromThis<FBangoAsyncBillboardGenerator>
{
public:
	FBangoAsyncBillboardGenerator(const TSoftObjectPtr<UTexture2D>& InOverlayTexture);
	
public:
	void Run();

	bool IsRunning() const { return bRunning; }
	
	TSoftObjectPtr<UTexture2D> GetTextureSource() const { return OverlayTexture; }
	
	UTexture2D* GetGeneratedTexture() const;

protected:
	UE::Tasks::TTask<void> GenerationTask;
	
	TSoftObjectPtr<UTexture2D> OverlayTexture;
	
	TStrongObjectPtr<UTexture2D> GeneratedBillboardTexture;
	
	bool bRunning = false;
	
	void OnOverlayLoaded();
};
