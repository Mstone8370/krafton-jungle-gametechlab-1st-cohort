#include "IrradianceBakePass.h"

#include <d3d11.h>

#include "EngineLoop.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/Texture.h"

FIrradianceBakePass::FIrradianceBakePass()
    : Texture(nullptr)
    , SRV(nullptr)
    , UAV(nullptr)
    , TextureSize(32)
{}

void FIrradianceBakePass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FIrradianceBakePass::PrepareRenderArr()
{
    FRenderPassBase::PrepareRenderArr();
}

void FIrradianceBakePass::ClearRenderArr()
{
    FRenderPassBase::ClearRenderArr();
}

void FIrradianceBakePass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);
    
    // Run
    constexpr UINT ThreadGroupSize = 32;
    const UINT NumGroupsX = (TextureSize + ThreadGroupSize - 1) / ThreadGroupSize;
    const UINT NumGroupsY = (TextureSize + ThreadGroupSize - 1) / ThreadGroupSize;
    Graphics->DeviceContext->Dispatch(
        NumGroupsX, 
        NumGroupsY,
        6
    );
    
    CleanUpRender(Viewport);
}

void FIrradianceBakePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // Create Texture
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = TextureSize;
    TextureDesc.Height = TextureSize;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 6;
    TextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    
    HRESULT hr = Graphics->Device->CreateTexture2D(&TextureDesc, nullptr, &Texture);
    if (FAILED(hr))
    {
        return;
    }
    
    // Create UAV
    D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.Format = TextureDesc.Format;
    UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    UAVDesc.Texture2DArray.MipSlice = 0;
    UAVDesc.Texture2DArray.FirstArraySlice = 0;
    UAVDesc.Texture2DArray.ArraySize = TextureDesc.ArraySize;
    
    hr = Graphics->Device->CreateUnorderedAccessView(Texture, &UAVDesc, &UAV);
    if (FAILED(hr))
    {
        return;
    }
    
    // Create SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = TextureDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SRVDesc.TextureCube.MostDetailedMip = 0;
    SRVDesc.TextureCube.MipLevels = 1;
    
    hr = Graphics->Device->CreateShaderResourceView(Texture, &SRVDesc, &SRV);
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
    
    hr = ShaderManager->AddComputeShader(L"EnvironmentIrradiance", L"Shaders/Bake/IBL/EnvironmentIrradiance.hlsl", "main", Defines);
    if (FAILED(hr))
    {
        return;
    }
    
    // Set Shader
    ID3D11ComputeShader* Shader = ShaderManager->GetComputeShaderByKey(L"EnvironmentIrradiance");
    Graphics->DeviceContext->CSSetShader(Shader, nullptr, 0);
    
    // Bind UAV
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, &UAV, nullptr);
    
    // Bind Source SRV
    if (std::shared_ptr<FTexture> EnvCubeMap = FEngineLoop::ResourceManager.GetTexture(L"EnvironmentCubeMap"))
    {
        Graphics->DeviceContext->CSSetShaderResources(0, 1, &EnvCubeMap->TextureSRV);
    }
    
    // Bind Sampler
    Graphics->DeviceContext->CSSetSamplers(0, 1, &Graphics->SamplerState_LinearWrap);
}

void FIrradianceBakePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // Unbind Shader
    Graphics->DeviceContext->CSSetShader(nullptr, nullptr, 0);
    
    // Unbind UAV
    ID3D11UnorderedAccessView* NullUAV[] = { nullptr };
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, NullUAV, nullptr);
    
    // Unbind SRV
    ID3D11ShaderResourceView* NullSRV[] = { nullptr };
    Graphics->DeviceContext->CSSetShaderResources(0, 1, NullSRV);
    
    // Release UAV
    UAV->Release();
    
    // Add Texture
    FWString TextureName = L"EnvironmentIrradiance";
    std::shared_ptr<FTexture> TextureAsset = std::make_shared<FTexture>(SRV, Texture, ESamplerType::Linear, TextureName, TextureSize, TextureSize);
    FEngineLoop::ResourceManager.AddTexture(TextureName, std::move(TextureAsset));
}

void FIrradianceBakePass::CreateResource()
{
    FRenderPassBase::CreateResource();
}

void FIrradianceBakePass::Release()
{
    FRenderPassBase::Release();
}
