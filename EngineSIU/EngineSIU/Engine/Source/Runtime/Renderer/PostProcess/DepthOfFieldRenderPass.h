#pragma once

#include "RenderPassBase.h"

struct FRenderTargetResource;
struct ID3D11SamplerState;

class FDepthOfFieldRenderPass : public FRenderPassBase
{
public:
    FDepthOfFieldRenderPass() = default;
    virtual ~FDepthOfFieldRenderPass() override = default;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    void PrepareDownSample(const std::shared_ptr<FEditorViewportClient>& Viewport, bool bNear);
    void CleanUpDownSample(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void PrepareLayerPass(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void CleanUpLayerPass(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void PrepareMaxFilter_Near(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void CleanUpMaxFilter_Near(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void PrepareCoCBlur(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void CleanUpCoCBlur(const std::shared_ptr<FEditorViewportClient>& Viewport);
    
    void PrepareBlur(const std::shared_ptr<FEditorViewportClient>& Viewport, bool bNear);
    void CleanUpBlur(const std::shared_ptr<FEditorViewportClient>& Viewport);
    
    void PrepareComposite(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void CleanUpComposite(const std::shared_ptr<FEditorViewportClient>& Viewport);
};
