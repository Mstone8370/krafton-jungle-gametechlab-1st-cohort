#include <algorithm>

#include "Components/StaticMeshComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/FObjLoader.h"
#include "Launch/EngineLoop.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"
#include "GameFramework/RabbitGameMode.h"
#include "GameFramework/Actor.h"

UStaticMeshComponent::UStaticMeshComponent()
{
}

UObject* UStaticMeshComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->StaticMesh = StaticMesh;
    NewComponent->SelectedSubMeshIndex = SelectedSubMeshIndex;
    NewComponent->PhotoType = PhotoType;
    NewComponent->MeshType = MeshType;

    return NewComponent;
}

void UStaticMeshComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    
    //StaticMesh 경로 저장
    UStaticMesh* CurrentMesh = GetStaticMesh(); 
    if (CurrentMesh != nullptr) {

        // 1. std::wstring 경로 얻기
        std::wstring PathWString = CurrentMesh->GetOjbectName(); // 이 함수가 std::wstring 반환 가정

        // 2. std::wstring을 FString으로 변환
        FString PathFString(PathWString.c_str()); // c_str()로 const wchar_t* 얻어서 FString 생성
       // PathFString = CurrentMesh->ConvertToRelativePathFromAssets(PathFString);

        FWString PathWString2 = PathFString.ToWideString();

        OutProperties.Add(TEXT("PhotoType"), FString::FromInt(static_cast<uint8>(PhotoType)));
        OutProperties.Add(TEXT("StaticMeshPath"), PathFString);
        OutProperties.Add(TEXT("StaticMeshType"), FString::FromInt(static_cast<uint8>(MeshType)));
    } else
    {
        OutProperties.Add(TEXT("StaticMeshPath"), TEXT("None")); // 메시 없음 명시
    }
}

void UStaticMeshComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;

    
    // --- StaticMesh 설정 ---
    TempStr = InProperties.Find(TEXT("StaticMeshPath"));
    if (TempStr) // 키가 존재하는지 확인
    {
        if (*TempStr != TEXT("None")) // 값이 "None"이 아닌지 확인
        {
            // 경로 문자열로 UStaticMesh 에셋 로드 시도
            if (UStaticMesh* Mesh2Set = Cast<UStaticMesh>(UAssetManager::Get().GetAsset(EAssetType::StaticMesh, *TempStr)))
            {
                SetStaticMesh(Mesh2Set);
            }
            else if (UStaticMesh* MeshToSet = FObjManager::CreateStaticMesh(*TempStr))
            {
                // TODO: FilePath가 올바르지 않아도 비어있는 UStaticMesh를 반환하는 문제 있음.
                SetStaticMesh(MeshToSet); // 성공 시 메시 설정
                //UE_LOG(ELogLevel::Display, TEXT("Set StaticMesh '%s' for %s"), **TempStr, *GetName());
            }
            else
            {
                // 로드 실패 시 경고 로그
                //UE_LOG(ELogLevel::Warning, TEXT("Could not load StaticMesh '%s' for %s"), **TempStr, *GetName());
                SetStaticMesh(nullptr); // 안전하게 nullptr로 설정
            }
        }
        else // 값이 "None"이면
        {
            SetStaticMesh(nullptr); // 명시적으로 메시 없음 설정
            //UE_LOG(ELogLevel::Display, TEXT("Set StaticMesh to None for %s"), *GetName());
        }
    }
    else // 키 자체가 없으면
    {
        // 키가 없는 경우 어떻게 처리할지 결정 (기본값 유지? nullptr 설정?)
        // 여기서는 기본값을 유지하거나, 안전하게 nullptr로 설정할 수 있습니다.
        // SetStaticMesh(nullptr); // 또는 아무것도 안 함
        //UE_LOG(ELogLevel::Display, TEXT("StaticMeshPath key not found for %s, mesh unchanged."), *GetName());
    }

    if (InProperties.Contains(TEXT("StaticMeshType")))
    {
        MeshType = static_cast<EStaticMeshType>(FString::ToInt(InProperties[TEXT("StaticMeshType")]));
    }

    if (InProperties.Contains(TEXT("PhotoType")))
    {
        PhotoType = static_cast<EPhotoType>(FString::ToInt(InProperties[TEXT("PhotoType")]));
    }
}

uint32 UStaticMeshComponent::GetNumMaterials() const
{
    if (StaticMesh == nullptr) return 0;

    return StaticMesh->GetMaterials().Num();
}

UMaterial* UStaticMeshComponent::GetMaterial(uint32 ElementIndex) const
{
    if (StaticMesh != nullptr)
    {
        if (OverrideMaterials[ElementIndex] != nullptr)
        {
            return OverrideMaterials[ElementIndex];
        }
    
        if (StaticMesh->GetMaterials().IsValidIndex(ElementIndex))
        {
            return StaticMesh->GetMaterials()[ElementIndex]->Material;
        }
    }
    return nullptr;
}

uint32 UStaticMeshComponent::GetMaterialIndex(FName MaterialSlotName) const
{
    if (StaticMesh == nullptr) return -1;

    return StaticMesh->GetMaterialIndex(MaterialSlotName);
}

TArray<FName> UStaticMeshComponent::GetMaterialSlotNames() const
{
    TArray<FName> MaterialNames;
    if (StaticMesh == nullptr) return MaterialNames;

    for (const FStaticMaterial* Material : StaticMesh->GetMaterials())
    {
        MaterialNames.Emplace(Material->MaterialSlotName);
    }

    return MaterialNames;
}

void UStaticMeshComponent::GetUsedMaterials(TArray<UMaterial*>& Out) const
{
    if (StaticMesh == nullptr)
    {
        return;
    }

    StaticMesh->GetUsedMaterials(Out);
    for (uint32 MaterialIndex = 0; MaterialIndex < GetNumMaterials(); MaterialIndex++)
    {
        if (OverrideMaterials[MaterialIndex] != nullptr)
        {
            Out[MaterialIndex] = OverrideMaterials[MaterialIndex];
        }
    }
}

int UStaticMeshComponent::CheckRayIntersection(
    const FVector& InRayOrigin, const FVector& InRayDirection,
    float& OutHitDistance, FVector& OutHitNormal) const
{
    if (!AABB.Intersect(InRayOrigin, InRayDirection, OutHitDistance))
    {
        return 0;
    }
    if (StaticMesh == nullptr)
    {
        return 0;
    }
    
    OutHitDistance = FLT_MAX;
    
    int IntersectionNum = 0;

    FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

    const TArray<FStaticMeshVertex>& Vertices = RenderData->Vertices;
    const int32 VertexNum = Vertices.Num();
    if (VertexNum == 0)
    {
        return 0;
    }
    
    const TArray<UINT>& Indices = RenderData->Indices;
    const int32 IndexNum = Indices.Num();
    const bool bHasIndices = (IndexNum > 0);
    
    int32 TriangleNum = bHasIndices ? (IndexNum / 3) : (VertexNum / 3);
    for (int32 i = 0; i < TriangleNum; i++)
    {
        int32 Idx0 = i * 3;
        int32 Idx1 = i * 3 + 1;
        int32 Idx2 = i * 3 + 2;
        
        if (bHasIndices)
        {
            Idx0 = Indices[Idx0];
            Idx1 = Indices[Idx1];
            Idx2 = Indices[Idx2];
        }

        // 각 삼각형의 버텍스 위치를 FVector로 불러옵니다.
        FVector V0 = FVector(Vertices[Idx0].X, Vertices[Idx0].Y, Vertices[Idx0].Z);
        FVector V1 = FVector(Vertices[Idx1].X, Vertices[Idx1].Y, Vertices[Idx1].Z);
        FVector V2 = FVector(Vertices[Idx2].X, Vertices[Idx2].Y, Vertices[Idx2].Z);

        float HitDistance = FLT_MAX;
        FVector HitNormal = FVector::ZeroVector;
        if (IntersectRayTriangle(InRayOrigin, InRayDirection, V0, V1, V2, HitDistance, HitNormal))
        {
            if (HitDistance < OutHitDistance)
            {
                OutHitDistance = HitDistance;
                OutHitNormal = HitNormal;
            }
            IntersectionNum++;
        }
    }
    return IntersectionNum;
}

EPhotoType UStaticMeshComponent::GetPhotoType()
{
    return PhotoType;
}

void UStaticMeshComponent::SetPhotoType(EPhotoType Type)
{
    int Check = static_cast<int>(EPhotoType::END) - static_cast<int>(Type);

    if (Check < 1)
    {
        PhotoType = EPhotoType::NONE;
        return;
    }

    PhotoType = Type;
}

EStaticMeshType UStaticMeshComponent::GetMeshType()
{
    return MeshType;
}

void UStaticMeshComponent::SetStMeshType(EStaticMeshType Type)
{
    int Check = static_cast<int>(EStaticMeshType::END) - static_cast<int>(Type);
    if (Check < 1)
    {
        MeshType = EStaticMeshType::NONE;
        return;
    }

    MeshType = Type;
        
        MeshType = Type;
}
