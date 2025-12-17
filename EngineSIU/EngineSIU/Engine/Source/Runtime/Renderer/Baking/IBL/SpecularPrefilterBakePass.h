#pragma once

#include "RenderPassBase.h"
#include "Container/Queue.h"

class FSpecularPrefilterBakePass : public FRenderPassBase
{
public:
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    void EnqueueSpecularPrefilterBake(const FWString& FilePath);
    
protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    virtual void Release() override;
    
private:
    TQueue<FWString> BakeQueue;
};
