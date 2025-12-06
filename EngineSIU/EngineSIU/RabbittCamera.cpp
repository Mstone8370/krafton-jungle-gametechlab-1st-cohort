#include "RabbitCamera.h"
#include <EngineLoop.h>
#include "SoundManager.h"
#include "LevelEditor/SLevelEditor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "Components/PrimitiveComponent.h"
#include <UObject/UObjectIterator.h>
#include <Actors/Player.h>
#include <Engine/Engine.h>
#include "World/World.h"
#include "GameFramework/Pawn.h"
#include <GameFramework/RabbitGameMode.h>

RabbitCamera::RabbitCamera()
{
    FSoundManager::GetInstance().LoadSound("Shutter", "Contents/Rabbit/Sound/Shutter.mp3");
    FSoundManager::GetInstance().LoadSound("Error", "Contents/Rabbit/Sound/Error.mp3");
    FSoundManager::GetInstance().LoadSound("Attack", "Contents/Rabbit/Sound/RabbitAttack.mp3");
    FSoundManager::GetInstance().LoadSound("SlowShutter", "Contents/Rabbit/Sound/SlowShutter.mp3");

    CameraCoolTime = CameraCoolTimeInit;

}

RabbitCamera::~RabbitCamera()
{
    ReleasePictures();
}

FRenderTargetRHI* RabbitCamera::CopySource(const FRenderTargetRHI* InputRHI)
{
    auto Device = GEngineLoop.GraphicDevice.Device;
    auto DeviceContext = GEngineLoop.GraphicDevice.DeviceContext;

    // 0. 원본 및 Graphics 객체 유효성 검사
    if (!InputRHI || !InputRHI->Texture2D)
    {
        // 필요한 정보가 없으면 nullptr 반환
        // (오류 로그를 남기는 것이 좋습니다)
        return nullptr;
    }

    // 1. 새로운 FRenderTargetRHI 객체를 힙에 할당
    FRenderTargetRHI* destination = new (std::nothrow) FRenderTargetRHI();
    if (!destination)
    {
        // 메모리 할당 실패
        return nullptr;
    }

    // 2. 원본 텍스처 디스크립션 가져오기
    D3D11_TEXTURE2D_DESC texDesc;
    InputRHI->Texture2D->GetDesc(&texDesc);

    // 3. 새로운 텍스처 생성 (깊은 복사 대상)
 
    HRESULT hr = Device->CreateTexture2D(&texDesc, nullptr, &destination->Texture2D);
    if (FAILED(hr) || !destination->Texture2D)
    {
        delete destination; // 실패 시 할당된 FRenderTargetRHI 객체 메모리 해제
        return nullptr;
    }

    // 4. 원본 텍스처 내용을 새 텍스처로 복사 (GPU 상에서 깊은 복사)
    DeviceContext->CopyResource(destination->Texture2D.Get(), InputRHI->Texture2D.Get());

    // 5. 새로 복사된 텍스처에 대한 SRV 생성
    if (destination->Texture2D)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = texDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = (texDesc.MipLevels == 0) ? -1 : texDesc.MipLevels; // 모든 밉 레벨 또는 명시적 값

        hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(destination->Texture2D.Get(), &srvDesc, &destination->SRV);
        if (FAILED(hr))
        {
            // destination->Release(); // Texture2D 해제 포함
            delete destination;
            return nullptr;
        }
    }
    else // 이 경우는 거의 발생하지 않아야 함
    {
        delete destination;
        return nullptr;
    }

    // destination->RTV는 nullptr로 유지됩니다.

    // 성공적으로 깊은 복사된 FRenderTargetRHI 객체의 포인터 반환
    return destination;
}

FRenderTargetRHI* RabbitCamera::CaptureFrame()
{
    const FRenderTargetRHI* Source = GEngineLoop.GetLevelEditor()
        ->GetActiveViewportClient()
        ->GetViewportResource()
        ->GetRenderTarget(EResourceType::ERT_DepthOfField_Result);

    return CopySource(Source);
}

float RabbitCamera::GetCameraCoolTime()
{
    return CameraCoolTime;
}

float RabbitCamera::GetCameraCoolTimeInit()
{
    return CameraCoolTimeInit;
}

void RabbitCamera::TakePicture()
{
    if (!ValidateTakePicture())
    {
        return;
    }
   
    CanTakePicture = false;
    TriggerShutterEffect();

    auto HitComp = CheckSubject();

    FVector OwnerLocation;
    if (GetOwner())
    {
        OwnerLocation = GetOwner()->GetActorLocation();
    }
    
    OnPictureTaken.Execute(this, HitComp, OwnerLocation);
}

void RabbitCamera::PlayCameraSound(bool IsEnd)
{
    if (IsEnd)
    {
       // FSoundManager::GetInstance().PlaySound("SlowShutter");
    }
    else
    {
        FSoundManager::GetInstance().PlaySound("Shutter");
    }
    
}

void RabbitCamera::ReleasePictures()
{
    for (auto Picture : PicturesRHI)
    {
        if (Picture)
        {
            //Picture->Release();
        }
    }

    PicturesRHI.Empty();
}

TArray<FRenderTargetRHI*> RabbitCamera::GetPicturesRHI() const
{
    return PicturesRHI;
}

void RabbitCamera::TriggerShutterEffect()
{
    bIsShutterAnimating = true;
    ShutterTimer = 0.0f;
    CurrentApertureProgress = 0.0f; // 시작 시점은 이미 0이거나, 여기서 명시적으로 0으로 설정
}

bool RabbitCamera::ValidateTakePicture()
{
    if (!CanTakePicture)
    {
        FSoundManager::GetInstance().PlaySound("Error");
        return false;
    }

    return true;
}

void RabbitCamera::StorePicture(EPhotoType Type)
{
    auto CapturedSource = CaptureFrame();

    if (CapturedSource)
    {
        int Index = static_cast<int>(Type);
        PicturesRHI[Index - 1] = CapturedSource;
        StoredPictureNum++;
    }
}

void RabbitCamera::Tick(float DeltaTime)
{
    if (!CanTakePicture)
    {
        CameraCoolTime -= DeltaTime;

        if (CameraCoolTime <= 0)
        {
            CanTakePicture = true;
            CameraCoolTime = CameraCoolTimeInit;
        }
    }


    if (!bIsShutterAnimating) 
    {
        // 애니메이션 중이 아니면 항상 열린 상태로 설정 (선택적)
        CurrentApertureProgress = 0.0f; 
        return;
    }

    ShutterTimer += DeltaTime;
    float NormalizedTime = ShutterTimer / ShutterDuration;

    if (NormalizedTime < 0.4f) 
    {
        CurrentApertureProgress = (NormalizedTime / 0.4f);
    }
    else if (NormalizedTime < 0.6f)
    {
        CurrentApertureProgress = 1.0f;
    }
    else if (NormalizedTime < 1.0f)
    {
        CurrentApertureProgress = 1.0f - ((NormalizedTime - 0.6f) / 0.4f);
    }
    else 
    {
        CurrentApertureProgress = 0.0f;
        bIsShutterAnimating = false;
    }

    CurrentApertureProgress = std::max(0.0f, std::min(1.0f, CurrentApertureProgress));
}

void RabbitCamera::ResetRabbitCamera(int Size)
{
    InitPictureArraySize(Size);
}

float RabbitCamera::GetCurrentApertureProgress() const
{
    return CurrentApertureProgress;
}

void RabbitCamera::SetCurrentApertureProgress(float value)
{
    CurrentApertureProgress = value;
}

void RabbitCamera::InitPictureArraySize(int Size)
{
    PicturesRHI.Init(nullptr, Size);
}

UPrimitiveComponent* RabbitCamera::CheckSubject()
{
    auto Player = GEngine->ActiveWorld->GetMainPlayer();

    FVector PlayerForward = Player->GetActorForwardVector();
    FVector PlayerPosition = Player->GetActorLocation();


   
    float FOV = FMath::Cos(FMath::DegreesToRadians(60.f)); // 느슨한 시야각 (총 30도)

    UStaticMeshComponent* HitComponent = nullptr;
    float MinHitDistance = MaxRange;

    bool bPrintedOutOfFOV = false;
    bool bPrintedTooFar = false;

    for (auto CurrentComponent : TObjectRange<UStaticMeshComponent>()) {
        if (!CurrentComponent ||
            CurrentComponent->GetPhotoType() <= EPhotoType::NONE ||
            CurrentComponent->GetPhotoType() >= EPhotoType::END)
        {
            continue;
        }

        FVector ObjectLocation = CurrentComponent->GetComponentLocation();
        FVector ToObject = ObjectLocation - PlayerPosition;
        float DistanceToObject = ToObject.Length();

        if (DistanceToObject > MaxRange)
        {
            if (!bPrintedTooFar)
            {
                bPrintedTooFar = true;
            }
            continue;
        }

        FVector ToObjectDir = ToObject.GetSafeNormal();
        float Dot = FVector::DotProduct(PlayerForward, ToObjectDir);

        if (Dot >= FOV) {
            if (DistanceToObject < MinHitDistance) {
                MinHitDistance = DistanceToObject;
                HitComponent = CurrentComponent;
            }
        }
        else
        {
            if (!bPrintedOutOfFOV)
            {
                bPrintedOutOfFOV = true;
            }
        }
    }

    return HitComponent;
}
