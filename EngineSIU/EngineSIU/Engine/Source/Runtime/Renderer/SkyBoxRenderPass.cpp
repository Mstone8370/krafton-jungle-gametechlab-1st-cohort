#include "SkyBoxRenderPass.h"

#include "UnrealClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "UnrealEd/EditorViewportClient.h"

void FSkyBoxRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FSkyBoxRenderPass::PrepareRenderArr()
{
    FRenderPassBase::PrepareRenderArr();
}

void FSkyBoxRenderPass::ClearRenderArr()
{
    FRenderPassBase::ClearRenderArr();
}

void FSkyBoxRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);
    
    Graphics->DeviceContext->Draw(36, 0);
    
    CleanUpRender(Viewport);
}

void FSkyBoxRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    Graphics->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    Graphics->DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"SkyBoxVertexShader");
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"SkyBoxPixelShader");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    
    // Begin TEMP
    /*
    if (const std::shared_ptr<FTexture> PrefilterTexture = FEngineLoop::ResourceManager.GetTexture(L"EnvironmentPrefilter"))
    {
        Graphics->DeviceContext->PSSetShaderResources(0, 1, &PrefilterTexture->TextureSRV);
    }
    if (const std::shared_ptr<FTexture> IrradianceTexture = FEngineLoop::ResourceManager.GetTexture(L"EnvironmentIrradiance"))
    {
        Graphics->DeviceContext->PSSetShaderResources(0, 1, &IrradianceTexture->TextureSRV);
    }
    */
    // End TEMP
    if (const std::shared_ptr<FTexture> EnvCubeMapTexture = FEngineLoop::ResourceManager.GetTexture(L"EnvironmentCubeMap"))
    {
        Graphics->DeviceContext->PSSetShaderResources(0, 1, &EnvCubeMapTexture->TextureSRV);
    }

    constexpr EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const FRenderTargetResource* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    const FDepthStencilResource* DepthStencilRHI = ViewportResource->GetDepthStencil(ResourceType);

    ID3D11RenderTargetView* RTV = RenderTargetRHI->RTV.Get();
    ID3D11DepthStencilView* DSV = DepthStencilRHI->DSV.Get();
    Graphics->DeviceContext->OMSetRenderTargets(1, &RTV, DSV);

    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
}

void FSkyBoxRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 1, NullSRV);
}

void FSkyBoxRenderPass::CreateResource()
{
    HRESULT hr = ShaderManager->AddVertexShader(L"SkyBoxVertexShader", L"Shaders/SkyBoxShader.hlsl", "mainVS");
    if (FAILED(hr))
    {
        return;
    }
    
    hr = ShaderManager->AddPixelShader(L"SkyBoxPixelShader", L"Shaders/SkyBoxShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }
}

void FSkyBoxRenderPass::Release()
{
    FRenderPassBase::Release();
}
