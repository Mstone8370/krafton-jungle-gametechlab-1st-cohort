#include "IntegrateBRDFBakePass.h"

#include <DirectXTex/DirectXTex.h>

#include "D3D11RHI/DXDShaderManager.h"

#define FCONSTANT_TOSTRING(x) #x

FIntegrateBRDFBakePass::FIntegrateBRDFBakePass()
    : TextureSize(512)
    , Texture2D(nullptr)
    , UAV(nullptr)
    , SRV(nullptr)
{}

void FIntegrateBRDFBakePass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FIntegrateBRDFBakePass::PrepareRenderArr()
{
    FRenderPassBase::PrepareRenderArr();
}

void FIntegrateBRDFBakePass::ClearRenderArr()
{
    FRenderPassBase::ClearRenderArr();
}

void FIntegrateBRDFBakePass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    
}

void FIntegrateBRDFBakePass::Bake(const FWString& Path)
{
    std::shared_ptr<FEditorViewportClient> DummyViewport = nullptr;
    
    PrepareRender(DummyViewport);
    
    // shader
    ID3D11ComputeShader* ComputeShader = ShaderManager->GetComputeShaderByKey(L"IntegrateBRDFBakePass");
    Graphics->DeviceContext->CSSetShader(ComputeShader, nullptr, 0);
    
    // run
    UINT ThreadGroupSize = 32;
    Graphics->DeviceContext->Dispatch(
        TextureSize / ThreadGroupSize, 
        TextureSize / ThreadGroupSize,
        1
    );
    
    CleanUpRender(DummyViewport);
    
    // Save to Disk
    DirectX::ScratchImage ScratchImage;
    HRESULT hr = DirectX::CaptureTexture(Graphics->Device, Graphics->DeviceContext, Texture2D, ScratchImage);
    if (FAILED(hr))
    {
        return;
    }
    
    hr = DirectX::SaveToDDSFile(
        ScratchImage.GetImages(),
        ScratchImage.GetImageCount(),
        ScratchImage.GetMetadata(),
        DirectX::DDS_FLAGS_NONE,
        Path.c_str()
    );
    if (FAILED(hr))
    {
        return;
    }
}

void FIntegrateBRDFBakePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // Compile Shader
    constexpr D3D_SHADER_MACRO Defines[] = {
        { "THREADS_X", "32" },
        { "THREADS_Y", "32" },
        { nullptr, nullptr }
    };
    
    HRESULT hr = ShaderManager->AddComputeShader(L"IntegrateBRDFBakePass", L"Shaders/Bake/IBL/IntegrateBRDF.hlsl", "main", Defines);
    if (FAILED(hr))
    {
        return;
    }
    
    // Create Resource
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = TextureSize;
    TextureDesc.Height = TextureDesc.Width;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;
    
    hr = Graphics->Device->CreateTexture2D(&TextureDesc, nullptr, &Texture2D);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D11_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
    UavDesc.Format = TextureDesc.Format;
    UavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    UavDesc.Texture2D.MipSlice = 0;
    
    hr = Graphics->Device->CreateUnorderedAccessView(Texture2D, &UavDesc, &UAV);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = TextureDesc.Format;
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SrvDesc.Texture2D.MostDetailedMip = 0;
    SrvDesc.Texture2D.MipLevels = 1;
    
    hr = Graphics->Device->CreateShaderResourceView(Texture2D, &SrvDesc, &SRV);
    if (FAILED(hr))
    {
        return;
    }
    
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, &UAV, nullptr);
}

void FIntegrateBRDFBakePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // TODO: Release Compute Shader
    
    // Move To ResourceManager
    FWString TextureName = L"EnvironmentBRDF";
    
    std::shared_ptr<FTexture> TextureAsset = std::make_shared<FTexture>(SRV, Texture2D, ESamplerType::Linear, TextureName, TextureSize, TextureSize);
    
    FEngineLoop::ResourceManager.AddTexture(TextureName, std::move(TextureAsset));
    
    // Unbind UAV
    ID3D11UnorderedAccessView* NullUAV[] = { UAV };
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, NullUAV, nullptr);
    
    // Release UAV
    UAV->Release();
}

void FIntegrateBRDFBakePass::CreateResource()
{
    
}

void FIntegrateBRDFBakePass::Release()
{
    FRenderPassBase::Release();
}
