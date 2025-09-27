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

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;
    
private:
    void UpdateLightBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport);

    float CalculateLightImportance(const ULightComponentBase* Light, const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    
    TArray<UPointLightComponent*> PointLightComps;
    TArray<USpotLightComponent*> SpotLightComps;

    UDirectionalLightComponent* DirectionLightComp;
    UAmbientLightComponent* AmbientLightComp;

    TArray<FLightData> LightData;

    const int32 MAX_LIGHT = 1024; // 한 씬에 허용하는 최대 조명 개수

    struct FLightCandidate
    {
        float Importance;
        int32 OriginalIndex;
        int32 Type;

        bool operator<(const FLightCandidate& Other) const
        {
            return Importance > Other.Importance;
        }
    };
};
