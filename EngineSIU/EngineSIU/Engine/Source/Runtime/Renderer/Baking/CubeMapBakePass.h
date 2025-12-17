#pragma once
#include "RenderPassBase.h"
#include "Container/Queue.h"

struct ID3D11Texture2D;
struct ID3D11UnorderedAccessView;
struct ID3D11ShaderResourceView;

class FCubeMapBakePass : public FRenderPassBase
{
public:
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    void EnqueueCubeMapBake(const FWString& FilePath);
    
    ID3D11ShaderResourceView* GetCubeMapSRV() const { return CubeMapSRV; }
    
protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    virtual void Release() override;
    
private:
    TQueue<FWString> BakeQueue;
    
    ID3D11Texture2D* CubeMapTexture = nullptr;
    ID3D11UnorderedAccessView* CubeMapUAV = nullptr;
    ID3D11ShaderResourceView* CubeMapSRV = nullptr;
};
