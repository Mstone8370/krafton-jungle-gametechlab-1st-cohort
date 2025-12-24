#pragma once

#include "RenderPassBase.h"

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;

class FIrradianceBakePass : public FRenderPassBase
{
public:
    FIrradianceBakePass();
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    virtual void Release() override;
    
private:
    ID3D11Texture2D* Texture;
    ID3D11ShaderResourceView* SRV;
    ID3D11UnorderedAccessView* UAV;
    
    uint32 TextureSize;
    
    struct FIrradianceData
    {
        uint32 SourceResolution = 0; // 원본 큐브맵의 해상도
        
        uint32 SourceNumMipLevels = 0; // 원본 큐브맵의 밉맵 개수
        
        uint32 FIrradianceData_Padding[2];
    };
};
