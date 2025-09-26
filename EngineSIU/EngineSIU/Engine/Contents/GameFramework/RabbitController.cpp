#include "RabbitController.h"

#include "Engine/Engine.h"
#include "Engine/Contents/GameFramework/RabbitPawn.h"
#include "Engine/Contents/GameFramework/RabbitPlayer.h"
#include "UObject/Casts.h"
#include "Windows/WindowsCursor.h"

void ARabbitController::BeginPlay()
{
    Super::BeginPlay();

    MouseSensitivityCurrent = MouseSensitivityBase;
    
    SetInputMode(EInputMode::GameOnly);
}

void ARabbitController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    
    SetInputMode(EInputMode::GameAndUI); // 게임 종료시 UI 모드로 전환
}

void ARabbitController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentInputMode == EInputMode::GameOnly)
    {
        RECT ClientRect;
        GetClientRect(GEngineLoop.AppWnd, &ClientRect);
        int32 Width = ClientRect.right - ClientRect.left;
        int32 Height = ClientRect.bottom - ClientRect.top;
        int32 TargetMousePositionX = ClientRect.left + static_cast<int32>(static_cast<float>(Width) * 0.5f);
        int32 TargetMousePositionY = ClientRect.top + static_cast<int32>(static_cast<float>(Height) * 0.5f);
        FWindowsCursor::SetPosition(TargetMousePositionX, TargetMousePositionY);
    }
}

void ARabbitController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("W", [this](float DeltaTime) { MoveForward(); });
    InputComponent->BindAction("S", [this](float DeltaTime) { MoveBack(); });
    InputComponent->BindAction("D", [this](float DeltaTime) { MoveRight(); });
    InputComponent->BindAction("A", [this](float DeltaTime) { MoveLeft(); });
    InputComponent->BindAction("E", [this](float DeltaTime) { ZoomIn(DeltaTime); });
    InputComponent->BindAction("Q", [this](float DeltaTime) { ZoomOut(DeltaTime); });

    InputComponent->BindAction("SPACE_Down", [this](float DeltaTime) { Jump(); });

    InputComponent->BindAxis("MouseX", [this](float Value) { AddYawInput(Value); });
    InputComponent->BindAxis("MouseY", [this](float Value) { AddPitchInput(-Value); });

    InputComponent->BindAction("ESC_Pressed", [this](float DeltaTime) { OnESCPressed(); });
    InputComponent->BindAction("TAB_Pressed", [this](float DeltaTime) { OnESCPressed(); });

    //마우스 클릭
    InputComponent->BindAction("L_Pressed", [this](float DeltaTime) { TakePicture();});
    InputComponent->BindAction("R_Pressed", [this](float DeltaTime) { StartADS(); });
    InputComponent->BindAction("R_Released", [this](float DeltaTime) { EndADS(); });
}

void ARabbitController::MoveForward()
{
    if (ShouldIgnoreInput())
    {
        return;
    }
    
    if (ARabbitPlayer* Pawn = Cast<ARabbitPlayer>(GetPawn()))
    {
        Pawn->AddMovementInput(Pawn->GetActorForwardVector(), 1.f);
    }
}

void ARabbitController::MoveBack()
{
    if (ShouldIgnoreInput())
    {
        return;
    }
    
    if (ARabbitPlayer* Pawn = Cast<ARabbitPlayer>(GetPawn()))
    {
        Pawn->AddMovementInput(Pawn->GetActorForwardVector(), -1.f);
    }
}

void ARabbitController::MoveRight()
{
    if (ShouldIgnoreInput())
    {
        return;
    }
    
    if (ARabbitPlayer* Pawn = Cast<ARabbitPlayer>(GetPawn()))
    {
        Pawn->AddMovementInput(Pawn->GetActorRightVector(), 1.f);
    }
}

void ARabbitController::MoveLeft()
{
    if (ShouldIgnoreInput())
    {
        return;
    }
    
    if (ARabbitPlayer* Pawn = Cast<ARabbitPlayer>(GetPawn()))
    {
        Pawn->AddMovementInput(Pawn->GetActorRightVector(), -1.f);
    }
}

void ARabbitController::Jump()
{
    if (ShouldIgnoreInput())
    {
        return;
    }
    
    if (ARabbitPlayer* Pawn = Cast<ARabbitPlayer>(GetPawn()))
    {
        Pawn->Jump();
    }
}

void ARabbitController::ZoomIn(float DeltaTime)
{
    if (ShouldIgnoreInput())
    {
        return;
    }
    
    if (ARabbitPlayer* Pawn = Cast<ARabbitPlayer>(GetPawn()))
    {
        Pawn->ZoomIn(DeltaTime);
    }
}

void ARabbitController::ZoomOut(float DeltaTime)
{
    if (ShouldIgnoreInput())
    {
        return;
    }
    
    if (ARabbitPlayer* Pawn = Cast<ARabbitPlayer>(GetPawn()))
    {
        Pawn->ZoomOut(DeltaTime);
    }
}

void ARabbitController::AddYawInput(float Value)
{
    if (ShouldIgnoreInput())
    {
        return;
    }

    if (APawn* Pawn = GetPawn())
    {
        Super::AddYawInput(Value * MouseSensitivityCurrent);
    }
}

void ARabbitController::AddPitchInput(float Value)
{
    if (ShouldIgnoreInput())
    {
        return;
    }

    if (APawn* Pawn = GetPawn())
    {
        Super::AddPitchInput(Value * MouseSensitivityCurrent);
    }
}

void ARabbitController::SetInputMode(EInputMode NewInputMode)
{
    CurrentInputMode = NewInputMode;
    switch (CurrentInputMode)
    {
    case EInputMode::UIOnly:
        FWindowsCursor::SetShowMouseCursor(true);
        FWindowsCursor::LockToWindow(false);
        FWindowsCursor::SetPosition(static_cast<int32>(MousePinPosition.X), static_cast<int32>(MousePinPosition.Y));
        break;
    case EInputMode::GameOnly:
        FWindowsCursor::SetShowMouseCursor(false);
        FWindowsCursor::LockToWindow(true);
        MousePinPosition = FWindowsCursor::GetPosition();
        break;
    case EInputMode::GameAndUI:
        FWindowsCursor::SetShowMouseCursor(true);
        FWindowsCursor::LockToWindow(false);
        break;
    }
}

void ARabbitController::OnESCPressed()
{
    if (CurrentInputMode == EInputMode::UIOnly)
    {
        SetInputMode(EInputMode::GameOnly);
    }
    else
    {
        SetInputMode(EInputMode::UIOnly);
    }
}

void ARabbitController::TakePicture()
{
    if (ShouldIgnoreInput())
    {
        return;
    }

    if (ARabbitPlayer* RabbitPawn = Cast<ARabbitPlayer>(PossessedPawn))
    {
        RabbitPawn->TakePicture();
    }
}

void ARabbitController::ToggleADS()
{
    if (ShouldIgnoreInput())
    {
        return;
    }

    if (ARabbitPlayer* RabbitPawn = Cast<ARabbitPlayer>(PossessedPawn))
    {
        RabbitPawn->ToggleADS();
    }
}

void ARabbitController::StartADS()
{
    if (ShouldIgnoreInput())
    {
        return;
    }

    if (ARabbitPlayer* RabbitPawn = Cast<ARabbitPlayer>(PossessedPawn))
    {
        RabbitPawn->StartADS();
    }
}

void ARabbitController::EndADS()
{
    if (ShouldIgnoreInput())
    {
        return;
    }

    if (ARabbitPlayer* RabbitPawn = Cast<ARabbitPlayer>(PossessedPawn))
    {
        RabbitPawn->EndADS();
    }
}

bool ARabbitController::ShouldIgnoreInput() const
{
    return !bInputEnabled || CurrentInputMode == EInputMode::UIOnly;
}
