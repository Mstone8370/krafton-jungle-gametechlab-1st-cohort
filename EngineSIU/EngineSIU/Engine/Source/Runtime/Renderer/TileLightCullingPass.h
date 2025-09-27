#pragma once
#include "RenderPassBase.h"

class FDXDShaderManager;
class FGraphicsDevice;
class FDXDBufferManager;

class FTileLightCullingPass : public FRenderPassBase
{
public:
    FTileLightCullingPass() = default;
    virtual ~FTileLightCullingPass() override = default;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    
private:
    
};

