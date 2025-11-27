#pragma once

#include "RenderPassBase.h"

class FDXDShaderManager;
class UWorld;
class FEditorViewportClient;

class ULightComponentBase;
class UPointLightComponent;
class USpotLightComponent;
class UDirectionalLightComponent;
class UAmbientLightComponent;

class FUpdateLightBufferPass : public FRenderPassBase
{
public:
    FUpdateLightBufferPass() = default;
    virtual ~FUpdateLightBufferPass() override = default;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    const TArray<FLightData>& GetLightData() const { return LightData; }
    
protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;
    
private:
    void UpdateLightBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport);

    // 씬의 모든 조명 중에서 현재 프레임에 사용 할 조명들을 고르는 작업
    int32 SelectActiveLights(const std::shared_ptr<FEditorViewportClient>& Viewport);
    
    float CalculateLightImportance(const ULightComponentBase* Light, const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    
    TArray<UPointLightComponent*> PointLightComps;
    TArray<USpotLightComponent*> SpotLightComps;

    UDirectionalLightComponent* DirectionLightComp;
    UAmbientLightComponent* AmbientLightComp;

    TArray<FLightData> LightData;

    const int32 MAX_LIGHT = 1024; // 한 씬에 허용하는 최대 조명 개수
    const int32 MAX_SHADOW_LIGHT = 16; // TODO: 포인트 라이트와 스팟 라이트를 구분해서 다뤄야 함

    struct FLightCandidate
    {
        float Importance = 0.f;
        int32 OriginalIndex = -1;
        int32 Type = -1;

        bool operator<(const FLightCandidate& Other) const
        {
            return Importance > Other.Importance;
        }
    };
};
