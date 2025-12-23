#include "SpecularPrefilterBakePass.h"

#include "EngineLoop.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/Texture.h"

FSpecularPrefilterBakePass::FSpecularPrefilterBakePass()
    : MaxReflectionLod(10)
    , TextureSize(0)
    , PreFilterTexture(nullptr) 
    , PreFilterSRV(nullptr)
    , PreFilterData()
{}

void FSpecularPrefilterBakePass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FSpecularPrefilterBakePass::PrepareRenderArr()
{
    FRenderPassBase::PrepareRenderArr();
}

void FSpecularPrefilterBakePass::ClearRenderArr()
{
    FRenderPassBase::ClearRenderArr();
}

void FSpecularPrefilterBakePass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);
    
    const int32 NumMipLevels = static_cast<int32>(PreFilterUAVs.Num());
    for (int32 i = 0; i < NumMipLevels; ++i)
    {
        // Bind UAV
        Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, &PreFilterUAVs[i], nullptr);
        
        // Update Constant Buffer
        const float Roughness = static_cast<float>(i) / static_cast<float>(NumMipLevels - 1);
        PreFilterData.Roughness = Roughness;
        BufferManager->BindConstantBuffer("FPreFilterData", 0, EShaderStage::Compute);
        BufferManager->UpdateConstantBuffer("FPreFilterData", PreFilterData);
        
        // Run
        const uint32 CurrentTextureSize = FMath::Max(TextureSize >> i, 1u);
        constexpr UINT ThreadGroupSize = 32;
        Graphics->DeviceContext->Dispatch(
            CurrentTextureSize / ThreadGroupSize, 
            CurrentTextureSize / ThreadGroupSize, 
            6
        );
    }
    
    CleanUpRender(Viewport);
}

void FSpecularPrefilterBakePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // Get CubeMap
    std::shared_ptr<FTexture> CubeMapTexture = FEngineLoop::ResourceManager.GetTexture(L"EnvironmentCubeMap");
    if (CubeMapTexture == nullptr)
    {
        return;
    }
    
    D3D11_TEXTURE2D_DESC CubeMapDesc;
    CubeMapTexture->Texture->GetDesc(&CubeMapDesc);
    PreFilterData.SourceTextureSize = CubeMapDesc.Width;
    PreFilterData.SourceNumMipLevels = CubeMapDesc.MipLevels;
    
    TextureSize = CubeMapDesc.Width / 2; // 반사용으로 쓰일거니 해상도를 적당히 낮춤 (256 or 512)
    
    // Create Texture2D
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = TextureSize;
    TextureDesc.Height = TextureDesc.Width;
    TextureDesc.MipLevels = MaxReflectionLod;
    TextureDesc.ArraySize = 6;
    TextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    
    HRESULT hr = Graphics->Device->CreateTexture2D(&TextureDesc, nullptr, &PreFilterTexture);
    if (FAILED(hr))
    {
        return;
    }
    
    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = TextureDesc.Format;
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SrvDesc.TextureCube.MostDetailedMip = 0;
    SrvDesc.TextureCube.MipLevels = -1;
    
    hr = Graphics->Device->CreateShaderResourceView(PreFilterTexture, &SrvDesc, &PreFilterSRV);
    if (FAILED(hr))
    {
        return;
    }
    
    // Create UAVs
    D3D11_TEXTURE2D_DESC TextureDesc_Created;
    PreFilterTexture->GetDesc(&TextureDesc_Created);
    
    const int32 NumMipLevels = static_cast<int32>(TextureDesc_Created.MipLevels);
    PreFilterUAVs.SetNum(NumMipLevels);
    
    D3D11_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
    UavDesc.Format = TextureDesc.Format;
    UavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    UavDesc.Texture2DArray.FirstArraySlice = 0;
    UavDesc.Texture2DArray.ArraySize = 6;
    
    for (int32 i = 0; i < NumMipLevels; ++i)
    {
        UavDesc.Texture2DArray.MipSlice = i;
        
        hr = Graphics->Device->CreateUnorderedAccessView(PreFilterTexture, &UavDesc, &PreFilterUAVs[i]);
        if (FAILED(hr))
        {
            return;
        }
    }
    
    // Create Constant Buffer
    hr = BufferManager->CreateBufferGeneric<FPreFilterData>(
        "FPreFilterData", 
        nullptr, 
        sizeof(FPreFilterData),
        D3D11_BIND_CONSTANT_BUFFER,
        D3D11_USAGE_DYNAMIC,
        D3D11_CPU_ACCESS_WRITE
    );
    if (FAILED(hr))
    {
        return;
    }
    
    // Compile Shader
    constexpr D3D_SHADER_MACRO Defines[] = {
        { "THREADS_X", "32" },
        { "THREADS_Y", "32" },
        { nullptr, nullptr }
    };
    
    hr = ShaderManager->AddComputeShader(L"EnvironmentPreFilterBake", L"Shaders/Bake/IBL/EnvironmentPreFilter.hlsl", "main", Defines);
    if (FAILED(hr))
    {
        return;
    }
    
    // Bind Shader
    ID3D11ComputeShader* ComputeShader = ShaderManager->GetComputeShaderByKey(L"EnvironmentPreFilterBake");
    Graphics->DeviceContext->CSSetShader(ComputeShader, nullptr, 0);
    
    // Bind SRV
    Graphics->DeviceContext->CSSetShaderResources(0, 1, &CubeMapTexture->TextureSRV);
    
    // Bind Sampler
    Graphics->DeviceContext->CSSetSamplers(1, 1, &Graphics->SamplerState_LinearClamp);
}

void FSpecularPrefilterBakePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // Unbind SRV
    ID3D11ShaderResourceView* NullSRV[] = { nullptr };
    Graphics->DeviceContext->CSSetShaderResources(0, 1, NullSRV);
    
    // Unbind UAV
    ID3D11UnorderedAccessView* NullUAV[] = { nullptr };
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, NullUAV, nullptr);
    
    // Unbind Shader
    Graphics->DeviceContext->CSSetShader(nullptr, nullptr, 0);
    
    // Unbind Constant Buffer
    BufferManager->BindConstantBuffer("", 0, EShaderStage::Compute);
    
    // Release UAVs
    for (auto& UAV : PreFilterUAVs)
    {
        UAV->Release();
    }
    PreFilterUAVs.Empty();
    
    // Add Texture
    FWString TextureName = L"EnvironmentPreFilter";
    std::shared_ptr<FTexture> TextureAsset = std::make_shared<FTexture>(PreFilterSRV, PreFilterTexture, ESamplerType::Linear, TextureName, TextureSize, TextureSize);
    FEngineLoop::ResourceManager.AddTexture(TextureName, std::move(TextureAsset));
}

void FSpecularPrefilterBakePass::CreateResource()
{
    FRenderPassBase::CreateResource();
}

void FSpecularPrefilterBakePass::Release()
{
    FRenderPassBase::Release();
}
