#pragma once

#include "RenderPassBase.h"

struct ID3D11ShaderResourceView;

class FSkyBoxRenderPass : public FRenderPassBase
{
public:
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    
    void SetCubeMapSRV(ID3D11ShaderResourceView* InCubeMapSRV);

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    virtual void Release() override;
    
private:
    ID3D11ShaderResourceView* CubeMapSRV = nullptr;
};
