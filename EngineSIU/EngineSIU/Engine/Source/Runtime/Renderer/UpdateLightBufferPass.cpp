#include "Define.h"
#include "UObject/Casts.h"
#include "UpdateLightBufferPass.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "Components/Light/LightComponentBase.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/AmbientLightComponent.h"

#include "Engine/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"


void FUpdateLightBufferPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);

    LightData.Reserve(MAX_LIGHT);
    LightData.SetNum(MAX_LIGHT);
}

void FUpdateLightBufferPass::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<ULightComponentBase>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Iter))
            {
                SpotLightComps.Add(SpotLight);
            }
            else if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(Iter))
            {
                PointLightComps.Add(PointLight);
            }
            else if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(Iter))
            {
                DirectionLightComp = DirectionalLight; // 하나만 허용
            }
            else if (UAmbientLightComponent* AmbientLight = Cast<UAmbientLightComponent>(Iter))
            {
                AmbientLightComp = AmbientLight; // 하나만 허용
            }
        }
    }
}

void FUpdateLightBufferPass::ClearRenderArr()
{
    PointLightComps.Empty();
    SpotLightComps.Empty();

    DirectionLightComp = nullptr;
    AmbientLightComp = nullptr;
}

void FUpdateLightBufferPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UpdateLightBuffer(Viewport);
    
    BufferManager->BindStructuredBufferSRV("FLightDataBuffer", 10, EShaderStage::Pixel);
}

void FUpdateLightBufferPass::UpdateLightBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FSceneLightConstants LightBufferData = {};
    
    if (DirectionLightComp)
    {
        LightBufferData.DirectionalLightInfo = DirectionLightComp->GetDirectionalLightInfo();
        LightBufferData.DirectionalLightInfo.Direction = DirectionLightComp->GetDirection();
        LightBufferData.DirectionalLightInfo.LightViewProj = DirectionLightComp->GetViewProjectionMatrix();
        LightBufferData.DirectionalLightInfo.LightInvProj = FMatrix::Inverse(DirectionLightComp->GetProjectionMatrix());
        //ShadowData.LightNearZ = Light->GetShadowNearPlane();
        //ShadowData.LightFrustumWidth = Light->GetShadowFrustumWidth();
                    
        //ShadowData.ShadowMapWidth = Light->GetShadowMapWidth();
        //ShadowData.ShadowMapHeight = Light->GetShadowMapHeight();
       
        LightBufferData.DirectionalLightsCount = 1;
    }

    if (AmbientLightComp)
    {
        LightBufferData.AmbientLightInfo = AmbientLightComp->GetAmbientLightInfo();
        LightBufferData.AmbientLightInfo.AmbientColor = AmbientLightComp->GetLightColor();
        
        LightBufferData.AmbientLightsCount = 1;
    }

    /*
     * TODO: 아래 단계 진행 전에 어떤 조명이 그림자 맵을 렌더할 지를 결정해야 함.
     *       cast shadow가 켜져있는 조명 중에서 중요도 기반으로 결정.
     */

    const int32 PointLightCount = PointLightComps.Num();
    const int32 SpotLightCount = SpotLightComps.Num();
    const int32 TotalLightCount = PointLightCount + SpotLightCount;
    const int32 AvailableLightCount = FMath::Min(MAX_LIGHT, TotalLightCount);
    LightBufferData.TotalActiveLightCount = AvailableLightCount;
    
    BufferManager->UpdateConstantBuffer(TEXT("FLightInfoBuffer"), LightBufferData);

    if (AvailableLightCount < TotalLightCount)
    {
        // 현재 존재하는 조명 개수가 최대 허용 개수를 초과하는 경우.
        TArray<FLightCandidate> Candidates;
        Candidates.Reserve(TotalLightCount);
        Candidates.SetNum(TotalLightCount);

        const FVector& ViewLocation = Viewport->GetCameraLocation();

        for (int32 i = 0; i < SpotLightCount; ++i)
        {
            const USpotLightComponent* Light = SpotLightComps[i];

            const float Distance = FVector::DistSquared(Light->GetComponentLocation(), ViewLocation);
            const float ScreenRadius = (Light->GetRadius() / Distance) * Viewport->Projection[1][1];

            float Importance = (ScreenRadius * Light->GetIntensity()) / (Distance * 0.1f + 1.f);

            if (Light->GetCastShadows())
            {
                Importance *= 10.f;
            }

            Candidates[i].Importance = Importance;
            Candidates[i].OriginalIndex = i;
            Candidates[i].Type = 1;
        }
        
        for (int32 i = 0; i < PointLightCount; ++i)
        {
            const int32 TargetIdx = SpotLightCount + i;
            
            const UPointLightComponent* Light = PointLightComps[i];

            const float Distance = FVector::DistSquared(Light->GetComponentLocation(), ViewLocation);
            const float ScreenRadius = (Light->GetRadius() / Distance) * Viewport->Projection[1][1];

            float Importance = (ScreenRadius * Light->GetIntensity()) / (Distance * 0.1f + 1.f);

            if (Light->GetCastShadows())
            {
                Importance *= 10.f;
            }

            Candidates[TargetIdx].Importance = Importance;
            Candidates[TargetIdx].OriginalIndex = i;
            Candidates[TargetIdx].Type = 1;
        }

        std::sort(Candidates.begin(), Candidates.end());

        for (int32 i = 0; i < AvailableLightCount; ++i)
        {
            const FLightCandidate& Candidate = Candidates[i];
            if (Candidate.Type == 0) // SpotLight
            {
                const USpotLightComponent* Light = SpotLightComps[Candidate.OriginalIndex]; 
                const FSpotLightInfo& SpotLightInfo = SpotLightComps[Candidate.OriginalIndex]->GetSpotLightInfo();
                
                LightData[i].LightColor = Light->GetLightColor();
                LightData[i].Location = Light->GetComponentLocation();
                LightData[i].Radius = Light->GetRadius();
                LightData[i].Direction = Light->GetForwardVector();
                LightData[i].Intensity = Light->GetIntensity();
                LightData[i].UpVector = Light->GetUpVector();
                LightData[i].ShadowBias = SpotLightInfo.ShadowBias;
                LightData[i].SpotRadians = FVector2D(Light->GetOuterRad(), Light->GetInnerRad());
                LightData[i].Type = 0 + (Light->GetCastShadows() ? 1 : 0) * 2;
                LightData[i].ShadowMapIndex = i;
            }
            else // PointLight
            {
                const UPointLightComponent* Light = PointLightComps[Candidate.OriginalIndex];
                FPointLightInfo PointLightInfo = PointLightComps[Candidate.OriginalIndex]->GetPointLightInfo();
                
                LightData[i].LightColor = Light->GetLightColor();
                LightData[i].Location = Light->GetComponentLocation();
                LightData[i].Radius = Light->GetRadius();
                LightData[i].Intensity = Light->GetIntensity();
                LightData[i].ShadowBias = PointLightInfo.ShadowBias;
                LightData[i].Type = 1 + (Light->GetCastShadows() ? 1 : 0) * 2;
            }
        }
    }
    else
    {
        for (int32 i = 0; i < SpotLightCount; ++i)
        {
            const USpotLightComponent* Light = SpotLightComps[i];
            const FSpotLightInfo& SpotLightInfo = SpotLightComps[i]->GetSpotLightInfo();
            
            LightData[i].LightColor = Light->GetLightColor();
            LightData[i].Location = Light->GetComponentLocation();
            LightData[i].Radius = Light->GetRadius();
            LightData[i].Direction = Light->GetForwardVector();
            LightData[i].Intensity = Light->GetIntensity();
            LightData[i].UpVector = Light->GetUpVector();
            LightData[i].ShadowBias = SpotLightInfo.ShadowBias;
            LightData[i].SpotRadians = FVector2D(Light->GetOuterRad(), Light->GetInnerRad());
            LightData[i].Type = 0 + (Light->GetCastShadows() ? 1 : 0) * 2;
            LightData[i].ShadowMapIndex = i;
        }
        
        for (int32 i = 0; i < PointLightCount; ++i)
        {
            const int32 TargetIdx = SpotLightCount + i;

            const UPointLightComponent* Light = PointLightComps[i];
            const FPointLightInfo& PointLightInfo = PointLightComps[i]->GetPointLightInfo();
            
            LightData[TargetIdx].LightColor = Light->GetLightColor();
            LightData[TargetIdx].Location = Light->GetComponentLocation();
            LightData[TargetIdx].Radius = Light->GetRadius();
            LightData[TargetIdx].Intensity = Light->GetIntensity();
            LightData[TargetIdx].ShadowBias = PointLightInfo.ShadowBias;
            LightData[TargetIdx].Type = 1 + (Light->GetCastShadows() ? 1 : 0) * 2;
            LightData[TargetIdx].ShadowMapIndex = i;
        }
    }

    BufferManager->UpdateStructuredBuffer("FLightDataBuffer", LightData);
}

float FUpdateLightBufferPass::CalculateLightImportance(const ULightComponentBase* Light, const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    const FVector& ViewLocation = Viewport->GetCameraLocation();
    
    const float Distance = FVector::DistSquared(Light->GetComponentLocation(), ViewLocation);
    const float ScreenRadius = (Light->GetRadius() / Distance) * Viewport->Projection[1][1];

    float Importance = (ScreenRadius * Light->GetIntensity()) / (Distance * 0.1f + 1.f);

    if (Light->GetCastShadows())
    {
        Importance *= 10.f;
    }

    return Importance;
}

/*
void FUpdateLightBufferPass::UpdatePointLightBuffer()
{
    if (PointLights.Num() == 0 || !PointLightBuffer)
    {
        return;
    }

    for (uint32 LightIdx = 0; std::cmp_less(LightIdx, PointLights.Num()); ++LightIdx)
    {
        FPointLightInfo& LightInfo = PointLights[LightIdx]->GetPointLightInfo();
        LightInfo.Position = PointLights[LightIdx]->GetComponentLocation();
        for (int ProjectionIndex = 0; ProjectionIndex < 6; ++ProjectionIndex)
        {
            LightInfo.LightViewProjs[ProjectionIndex] = PointLights[LightIdx]->GetViewProjectionMatrix(ProjectionIndex);
        }
        LightInfo.ShadowMapArrayIndex = LightIdx;
        LightInfo.ShadowBias = 0.005f;
        PointLightInfo[LightIdx] = LightInfo;
    }
    // 이제 TempBuffer에 대해 업데이트
    Graphics->DeviceContext->UpdateSubresource(PointLightBuffer, 0, nullptr,
        PointLightInfo.GetData(), 0, 0);
}
 
void FUpdateLightBufferPass::UpdateSpotLightBuffer()
{
    if (SpotLights.Num() == 0 || !SpotLightBuffer)
    {
        return;
    }
    
    for (uint32 Idx = 0; std::cmp_less(Idx, SpotLights.Num()); ++Idx)
    {
        FSpotLightInfo& LightInfo = SpotLights[Idx]->GetSpotLightInfo();
        LightInfo.Position = SpotLights[Idx]->GetComponentLocation();
        LightInfo.Direction = SpotLights[Idx]->GetDirection();
        LightInfo.LightViewProj = SpotLights[Idx]->GetViewMatrix() * SpotLights[Idx]->GetProjectionMatrix();
        LightInfo.ShadowMapArrayIndex = Idx;
        LightInfo.ShadowBias = 0.005f;
        SpotLightInfo[Idx] = LightInfo;
    }
    // 이제 TempBuffer에 대해 업데이트
    Graphics->DeviceContext->UpdateSubresource(SpotLightBuffer, 0, nullptr,
        SpotLightInfo.GetData(), 0, 0);
}
*/

void FUpdateLightBufferPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FUpdateLightBufferPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FUpdateLightBufferPass::CreateResource()
{
    UINT LightInfoBufferSize = sizeof(FSceneLightConstants);
    BufferManager->CreateBufferGeneric<FSceneLightConstants>("FLightInfoBuffer", nullptr, LightInfoBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    BufferManager->CreateStructuredBufferGeneric<FLightData>("FLightDataBuffer", nullptr, MAX_LIGHT, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}
