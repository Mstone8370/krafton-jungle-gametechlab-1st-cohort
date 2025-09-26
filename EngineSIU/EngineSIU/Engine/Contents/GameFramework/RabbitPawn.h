#pragma once
#include "GameFramework/Pawn.h"
#include "Animation/RabbitAnimStateMachine.h"

class URabbitMovementComponent;
class RabbitCamera;
class USkeletalMeshComponent;
class UPrimitiveComponent;

class ARabbitPawn : public APawn
{
    DECLARE_CLASS(ARabbitPawn, APawn)
    
public:
    ARabbitPawn() = default;

    virtual void PostSpawnInitialize() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void BeginPlay() override;
    
    virtual void Tick(float DeltaTime) override;
    
    virtual FVector GetActorForwardVector() const override;
    virtual FVector GetActorRightVector() const override;
    
    int32 GetMaxHealth() const { return MaxHealth; }
    void SetMaxHealth(int32 Value);
    
    int32 GetCurrentHealth() const { return CurrentHealth; }
    void SetCurrentHealth(int32 Value);

    ERabbitAnimState GetAnimState() const { return AnimState; }
    void SetAnimState(ERabbitAnimState State) { AnimState = State; }

    virtual bool SetActorLocation(const FVector& NewLocation) override;

    virtual void Jump();
    
protected:
    URabbitMovementComponent* MovementComponent;
    USkeletalMeshComponent* SkeletalMeshComp;

    virtual void OnRabbitBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp);
    virtual void OnRabbitEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp);

    float HalfHeight = 40.f;
    float Radius = 20.f;

    float RotateSpeed = 100.0f;
    
    ERabbitAnimState AnimState = ERabbitAnimState::Attack;
    float MoveSpeed = 100.0f; // 이동 속도
    
    UPROPERTY(EditAnywhere, int32, MaxHealth, = 3)
    UPROPERTY(EditAnywhere, int32, CurrentHealth, = 3)

};

