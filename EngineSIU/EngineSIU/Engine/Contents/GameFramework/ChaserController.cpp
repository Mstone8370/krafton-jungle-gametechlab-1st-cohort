#include "ChaserController.h"
#include "Engine/Contents/Actors/GridMapActor.h"
#include "Engine/Contents/Navigation/PathFinder.h"
#include "Engine/World/World.h"
#include "Engine/Contents/GameFramework/ChasePawn.h"
#include "Engine/Contents/GameFramework/RabbitPlayer.h"
#include "RabbitEnemy.h"

void AChaserController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    PathUpdateTimer += DeltaTime;
    if (PathUpdateTimer >= PathUpdateInterval)
    {
        PathUpdateTimer = 0.0f;
        
    }

    // 현재 이동 중인 목표를 ChasePawn에 전달.
    if (IsPathValid())
    {
        // FIXME : NodeGrid에서 FVector 반환하도록 변경.
        //FVector NextTarget = FVector(CurrentPath[CurrentPathIndex]->X, CurrentPath[CurrentPathIndex]->Y, GetActorLocation().Z);
        //GetPossesedRabbitEnemy()->SetTargetLocation(NextTarget);
    }
}

void AChaserController::UpdatePath()
{
    if (!GetPossesedRabbitEnemy()|| !GetTargetRabbitPlayer())
    {
        //UE_LOG(ELogLevel::Warning, "Invalid Player or ChasePawn");
        return;
    }
    //AGridMapActor* GridMapActor = GetWorld()->GetActorByClass<AGridMapActor>();
    FVector StartLocation = GetActorLocation();    
    FVector TargetLocation = PossessedPawn->GetActorLocation();

    // 경로 계산
    // TODO 
    CurrentPath.Empty();
    FGridNode* Node = new FGridNode();
    Node->X = static_cast<int32>(TargetLocation.X);
    Node->Y = static_cast<int32>(TargetLocation.Y);
    //CurrentPath.Add(Node);

    CurrentPathIndex = 0;
}

ARabbitPlayer* AChaserController::GetTargetRabbitPlayer()
{
    return Cast<ARabbitPlayer>(TargetPawn);
}

ARabbitEnemy* AChaserController::GetPossesedRabbitEnemy()
{
    return Cast<ARabbitEnemy>(PossessedPawn);
}



