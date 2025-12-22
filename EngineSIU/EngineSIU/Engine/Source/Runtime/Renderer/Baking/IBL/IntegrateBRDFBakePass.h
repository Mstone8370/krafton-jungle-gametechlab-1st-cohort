#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"


class FIntegrateBRDFBakePass : public FRenderPassBase
{
public:
    FIntegrateBRDFBakePass();
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    void Bake();
    
protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    virtual void Release() override;
    
private:
    FWString BakePath;
    
    uint32 NumSamples;
    
    bool bBake;
};
