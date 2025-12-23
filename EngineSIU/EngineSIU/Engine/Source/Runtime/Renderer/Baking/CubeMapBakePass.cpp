#include "CubeMapBakePass.h"

#include <d3d11.h>

#include "EngineLoop.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"

void FCubeMapBakePass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FCubeMapBakePass::PrepareRenderArr()
{
    FRenderPassBase::PrepareRenderArr();
}

void FCubeMapBakePass::ClearRenderArr()
{
    FRenderPassBase::ClearRenderArr();
}

void FCubeMapBakePass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (BakeQueue.IsEmpty())
    {
        return;
    }
    
    D3D11_TEXTURE2D_DESC Desc = {};
    Desc.Width = TextureSize;
    Desc.Height = Desc.Width;
    Desc.MipLevels = 0;
    Desc.ArraySize = 6;
    Desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
    Desc.CPUAccessFlags = 0;
    Desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
    HRESULT hr = Graphics->Device->CreateTexture2D(&Desc, nullptr, &CubeMapTexture);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.Format = Desc.Format;
    UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    UAVDesc.Texture2DArray.MipSlice = 0;
    UAVDesc.Texture2DArray.FirstArraySlice = 0;
    UAVDesc.Texture2DArray.ArraySize = Desc.ArraySize;
    
    hr = Graphics->Device->CreateUnorderedAccessView(CubeMapTexture, &UAVDesc, &CubeMapUAV);
    if (FAILED(hr))
    {
        return;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = Desc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SRVDesc.TextureCube.MostDetailedMip = 0;
    SRVDesc.TextureCube.MipLevels = -1;
    
    hr = Graphics->Device->CreateShaderResourceView(CubeMapTexture, &SRVDesc, &CubeMapSRV);
    if (FAILED(hr))
    {
        return;
    }
    
    // Bind
    FWString FileName = *BakeQueue.Peek();
    BakeQueue.Dequeue();
    std::shared_ptr<FTexture> SourceTexture = FEngineLoop::ResourceManager.GetTexture(FileName);
    Graphics->DeviceContext->CSSetShaderResources(0, 1, &SourceTexture->TextureSRV);
    
    UINT InitialCounts[] = { 0 };
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, &CubeMapUAV, InitialCounts);
    
    Graphics->DeviceContext->CSSetSamplers(0, 1, &Graphics->SamplerState_LinearWrap);
    
    // shader
    ID3D11ComputeShader* ComputeShader = ShaderManager->GetComputeShaderByKey(L"EquirectToCube");
    Graphics->DeviceContext->CSSetShader(ComputeShader, nullptr, 0);
    
    // run
    constexpr UINT ThreadGroupSize = 32;
    const UINT NumGroupsX = (Desc.Width + ThreadGroupSize - 1) / ThreadGroupSize;
    const UINT NumGroupsY = (Desc.Height + ThreadGroupSize - 1) / ThreadGroupSize;
    Graphics->DeviceContext->Dispatch(
        NumGroupsX, 
        NumGroupsY,
        6
    );
    
    Graphics->DeviceContext->GenerateMips(CubeMapSRV);
    
    // end
    ID3D11ShaderResourceView* NullSRV = nullptr;
    Graphics->DeviceContext->CSSetShaderResources(0, 1, &NullSRV);
    
    ID3D11UnorderedAccessView* NullUAV = nullptr;
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, nullptr);
    
    // Add Texture Asset
    FWString TextureName = L"EnvironmentCubeMap";
    std::shared_ptr<FTexture> TextureAsset = std::make_shared<FTexture>(CubeMapSRV, CubeMapTexture, ESamplerType::Linear, TextureName, TextureSize, TextureSize);
    FEngineLoop::ResourceManager.AddTexture(TextureName, std::move(TextureAsset));
    
    // Release UAV
    CubeMapUAV->Release();
}

void FCubeMapBakePass::EnqueueCubeMapBake(const FWString& FilePath)
{
    BakeQueue.Enqueue(FilePath);
}

void FCubeMapBakePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    
}

void FCubeMapBakePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FCubeMapBakePass::CreateResource()
{
    constexpr D3D_SHADER_MACRO Defines[] = {
        { "THREADS_X", "32" },
        { "THREADS_Y", "32" },
        { nullptr, nullptr }
    };
    
    HRESULT hr = ShaderManager->AddComputeShader(L"EquirectToCube", L"Shaders/Bake/EquirectToCube.hlsl", "main", Defines);
    if (FAILED(hr))
    {
        return;
    }
}

void FCubeMapBakePass::Release()
{
}
