#pragma once

#include "RenderPassBase.h"
#include "Container/Queue.h"

struct ID3D11Buffer;
struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;

class FSpecularPrefilterBakePass : public FRenderPassBase
{
public:
    FSpecularPrefilterBakePass();
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    int32 GetMaxReflectionLod() const { return MaxReflectionLod; }
    
protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void CreateResource() override;

    virtual void Release() override;
    
private:
    int32 MaxReflectionLod;
    
    uint32 TextureSize;
    
    ID3D11Buffer* ConstantBuffer;
    ID3D11Texture2D* PrefilterTexture;
    ID3D11ShaderResourceView* PrefilterSRV;
    TArray<ID3D11UnorderedAccessView*> PrefilterUAVs;
    
    struct FPrefilterData
    {
        float Roughness = 0.0f;
        
        uint32 SourceTextureSize = 0;
        
        uint32 SourceNumMipLevels = 0;
        
        int32 Padding;
    };
    
    FPrefilterData PrefilterData;
};
