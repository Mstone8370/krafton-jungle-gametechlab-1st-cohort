#pragma once
#include "UnrealClient.h"
#include "Delegates/DelegateCombination.h"

class AActor;
class UPrimitiveComponent;
class RabbitCamera;
enum class EPhotoType;

DECLARE_DELEGATE_ThreeParams(FOnPictureTaken, RabbitCamera* /* RabbitCamera */, UPrimitiveComponent* /* TargetComponent */, FVector /* PlayerLocation */);

class RabbitCamera
{
public:
    RabbitCamera();
    ~RabbitCamera();

private:
    int StoredPictureNum = 0;
    TArray<FRenderTargetRHI*> PicturesRHI;
    bool  bIsShutterAnimating = false;
    float ShutterTimer = 0.0f;
    const float ShutterDuration = 0.2f; // 예: 0.25초
    float CurrentApertureProgress = 0.0f;
    float MaxRange = 700.f; // 최대 거리
    bool CanTakePicture = true;
    float CameraCoolTime;
    float CameraCoolTimeInit = 3;

    AActor* OwnerActor = nullptr;

public:
    void SetMaxRange(float Range) { MaxRange = Range; }
    float GetMaxRange() { return MaxRange; }

    void ReleasePictures();
    void Tick(float deltaTime);
    void ResetRabbitCamera(int Size);
    float GetCameraCoolTime();
    float GetCameraCoolTimeInit();
    float GetCurrentApertureProgress() const;
    void SetCurrentApertureProgress(float value);
    void InitPictureArraySize(int Size);
    void TakePicture();
    void PlayCameraSound(bool IsEnd);

    TArray<FRenderTargetRHI*> GetPicturesRHI() const;

    FOnPictureTaken OnPictureTaken;
    
    void StorePicture(EPhotoType Type);

    AActor* GetOwner() const { return OwnerActor; }
    void SetOwner(AActor* InOwner) { OwnerActor = InOwner; }

private:
    FRenderTargetRHI* CopySource(const FRenderTargetRHI* InputRHI);
    FRenderTargetRHI* CaptureFrame();
    UPrimitiveComponent* CheckSubject();
    void TriggerShutterEffect();
    bool ValidateTakePicture();
    
};

