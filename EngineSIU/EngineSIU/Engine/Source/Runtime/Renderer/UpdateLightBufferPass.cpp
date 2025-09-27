#include "Define.h"
#include "UObject/Casts.h"
#include "UpdateLightBufferPass.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/AmbientLightComponent.h"

#include "Engine/EditorEngine.h"
#include "UObject/UObjectIterator.h"


void FUpdateLightBufferPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FUpdateLightBufferPass::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<ULightComponentBase>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(Iter))
            {
                PointLightComps.Add(PointLight);
            }
            else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Iter))
            {
                SpotLightComps.Add(SpotLight);
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
}

void FUpdateLightBufferPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UpdateLightBuffer();

    // 전역 조명 리스트
    Graphics->DeviceContext->PSSetShaderResources(10, 1, &PointLightSRV);
    Graphics->DeviceContext->PSSetShaderResources(11, 1, &SpotLightSRV);
    // 타일별 조명 인덱스 리스트
    Graphics->DeviceContext->PSSetShaderResources(12, 1, &PointLightIndexBufferSRV);
    Graphics->DeviceContext->PSSetShaderResources(13, 1, &SpotLightIndexBufferSRV);
}

void FUpdateLightBufferPass::UpdateLightBuffer() const
{
    FSceneLightConstants LightBufferData = {};

    int32 DirectionalLightsCount=0;
    int32 AmbientLightsCount=0;
    
    int32 PointLightsCount=0;
    int32 SpotLightsCount=0;
    
    for (auto Light : SpotLightComps)
    {
        if (SpotLightsCount < MAX_SPOT_LIGHT)
        {
            LightBufferData.SpotLights[SpotLightsCount] = Light->GetSpotLightInfo();
            LightBufferData.SpotLights[SpotLightsCount].Position = Light->GetComponentLocation();
            LightBufferData.SpotLights[SpotLightsCount].Direction = Light->GetDirection();
            SpotLightsCount++;
        }
    }

    for (auto Light : PointLightComps)
    {
        if (PointLightsCount < MAX_POINT_LIGHT)
        {
            LightBufferData.PointLights[PointLightsCount] = Light->GetPointLightInfo();
            LightBufferData.PointLights[PointLightsCount].Position = Light->GetComponentLocation();
            PointLightsCount++;
        }
    }

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
       
        DirectionalLightsCount++;
    }

    if (AmbientLightComp)
    {
        LightBufferData.AmbientLightInfo = AmbientLightComp->GetAmbientLightInfo();
        LightBufferData.AmbientLightInfo.AmbientColor = AmbientLightComp->GetLightColor();
        AmbientLightsCount++;
    }
    
    LightBufferData.DirectionalLightsCount = DirectionalLightsCount;
    LightBufferData.AmbientLightsCount = AmbientLightsCount;
    
    LightBufferData.PointLightsCount = PointLightsCount;
    LightBufferData.SpotLightsCount = SpotLightsCount;

    BufferManager->UpdateConstantBuffer(TEXT("FLightInfoBuffer"), LightBufferData);
}

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

void FUpdateLightBufferPass::UpdatePointLightPerTilesBuffer()
{
    if (GPointLightPerTiles.Num() == 0 || !PointLightPerTilesBuffer)
    {
        return;
    }

    TArray<PointLightPerTile> TempBuffer;
    TempBuffer.SetNum(MAX_TILE);
    for (uint32 Idx = 0; std::cmp_less(Idx, GPointLightPerTiles.Num()); ++Idx)
    {
        TempBuffer[Idx] = GPointLightPerTiles[Idx];
    }
    // 이제 TempBuffer에 대해 업데이트
    Graphics->DeviceContext->UpdateSubresource(PointLightPerTilesBuffer, 0, nullptr,
        TempBuffer.GetData(), 0, 0);
}

void FUpdateLightBufferPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FUpdateLightBufferPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FUpdateLightBufferPass::CreateResource()
{
    BufferManager->CreateStructuredBufferGeneric<FLightData>("LightDataBuffer", nullptr, MAX_LIGHT, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}
