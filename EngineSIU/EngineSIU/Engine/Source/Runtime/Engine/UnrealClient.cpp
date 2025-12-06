#include "UnrealClient.h"

#include "EngineLoop.h"
#include <array>

FViewportResource::FViewportResource()
{
    ClearColors.Add(EResourceType::ERT_Compositing, { 0.f, 0.f, 0.f, 1.f });
    ClearColors.Add(EResourceType::ERT_Scene,  { 0.025f, 0.025f, 0.025f, 1.0f });
    ClearColors.Add(EResourceType::ERT_PP_Fog, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_PP_CameraEffect, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_Debug, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_Editor, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_Gizmo, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_EditorOverlay, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_DepthOfField_LayerInfo, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_DepthOfField_LayerNear, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_DepthOfField_LayerFar, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_DepthOfField_Result, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_Temp1, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_Temp2, { 0.f, 0.f, 0.f, 0.f });
    ClearColors.Add(EResourceType::ERT_PostProcessCompositing, { 0.f, 0.f, 0.f, 0.f });
}

FViewportResource::~FViewportResource()
{
    Release();
}

void FViewportResource::Initialize(uint32 InWidth, uint32 InHeight)
{
    D3DViewport.TopLeftX = 0.f;
    D3DViewport.TopLeftY = 0.f;
    D3DViewport.Height = static_cast<float>(InHeight);
    D3DViewport.Width = static_cast<float>(InWidth);
    D3DViewport.MaxDepth = 1.0f;
    D3DViewport.MinDepth = 0.0f;
    
    // Essential resources
    HRESULT hr = S_OK;
    hr = CreateDepthStencil(EResourceType::ERT_Scene);
    if (FAILED(hr))
    {
        return;
    }
    
    hr = CreateDepthStencil(EResourceType::ERT_Gizmo);
    if (FAILED(hr))
    {
        return;
    }
    
    hr = CreateRenderTarget(EResourceType::ERT_Compositing);
    if (FAILED(hr))
    {
        return;
    }

    hr = CreateRenderTarget(EResourceType::ERT_Scene);
    if (FAILED(hr))
    {
        return;
    }
}

void FViewportResource::Resize(uint32 NewWidth, uint32 NewHeight)
{
    ReleaseAllResources();

    D3DViewport.Height = static_cast<float>(NewHeight);
    D3DViewport.Width = static_cast<float>(NewWidth);

    // 이후 GetRenderTarget 또는 GetDepthStencil 호출 시 리소스 자동으로 생성
}

void FViewportResource::Release()
{
    ReleaseAllResources();

    DepthStencils.Empty();
    RenderTargets.Empty();

    ClearColors.Empty();
}

HRESULT FViewportResource::CreateDepthStencil(EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (HasDepthStencil(Type, DownSampleScale))
    {
        ReleaseDepthStencil(Type, DownSampleScale);
    }

    FDepthStencilResource NewResource;
    
    HRESULT hr = S_OK;
    
    D3D11_TEXTURE2D_DESC DepthStencilTextureDesc = {};
    DepthStencilTextureDesc.Width = static_cast<uint32>(D3DViewport.Width / static_cast<float>(DownSampleScale));
    DepthStencilTextureDesc.Height = static_cast<uint32>(D3DViewport.Height / static_cast<float>(DownSampleScale));
    DepthStencilTextureDesc.MipLevels = 1;
    DepthStencilTextureDesc.ArraySize = 1;
    DepthStencilTextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    DepthStencilTextureDesc.SampleDesc.Count = 1;
    DepthStencilTextureDesc.SampleDesc.Quality = 0;
    DepthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    DepthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    DepthStencilTextureDesc.CPUAccessFlags = 0;
    DepthStencilTextureDesc.MiscFlags = 0;
    
    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&DepthStencilTextureDesc, nullptr, NewResource.Texture2D.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    
    D3D11_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
    DepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    DepthStencilViewDesc.Texture2D.MipSlice = 0;
    hr = FEngineLoop::GraphicDevice.Device->CreateDepthStencilView(NewResource.Texture2D.Get(),  &DepthStencilViewDesc,  &NewResource.DSV);
    if (FAILED(hr))
    {
        return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC DepthStencilDesc = {};
    DepthStencilDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    DepthStencilDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    DepthStencilDesc.Texture2D.MostDetailedMip = 0;
    DepthStencilDesc.Texture2D.MipLevels = 1;
    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(NewResource.Texture2D.Get(), &DepthStencilDesc, &NewResource.SRV);
    if (FAILED(hr))
    {
        return hr;
    }

    DepthStencils[Type][DownSampleScale] = NewResource;

    return hr;
}

const FDepthStencilResource* FViewportResource::GetDepthStencil(EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (!HasDepthStencil(Type, DownSampleScale))
    {
        if (FAILED(CreateDepthStencil(Type, DownSampleScale)))
        {
            return nullptr;
        }
    }
    return &DepthStencils[Type][DownSampleScale];
}

bool FViewportResource::HasDepthStencil(EResourceType Type, EDownSampleScale DownSampleScale) const
{
    return DepthStencils.Contains(Type) && DepthStencils[Type].Contains(DownSampleScale);
}

void FViewportResource::ClearDepthStencils(ID3D11DeviceContext* DeviceContext)
{
    for (auto& [Type, Item] : DepthStencils)
    {
        for (auto& [DownSampleScale, Resource] : Item)
        {
            ClearDepthStencil(DeviceContext, Type, DownSampleScale);
        }
    }
}

void FViewportResource::ClearDepthStencil(ID3D11DeviceContext* DeviceContext, EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (HasDepthStencil(Type, DownSampleScale))
    {
        if (const FDepthStencilResource* Resource = GetDepthStencil(Type, DownSampleScale))
        {
            DeviceContext->ClearDepthStencilView(Resource->DSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        }
    }
}

HRESULT FViewportResource::CreateRenderTarget(EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (HasRenderTarget(Type, DownSampleScale))
    {
        ReleaseRenderTarget(Type, DownSampleScale);
    }
    
    FRenderTargetResource NewResource;
    
    HRESULT hr = S_OK;
    
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = static_cast<uint32>(D3DViewport.Width / static_cast<float>(DownSampleScale));
    TextureDesc.Height = static_cast<uint32>(D3DViewport.Height / static_cast<float>(DownSampleScale));
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.SampleDesc.Quality = 0;
    TextureDesc.Usage = D3D11_USAGE_DEFAULT;
    TextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    TextureDesc.CPUAccessFlags = 0;
    TextureDesc.MiscFlags = 0;
    
    hr = FEngineLoop::GraphicDevice.Device->CreateTexture2D(&TextureDesc, nullptr, NewResource.Texture2D.GetAddressOf());
    if (FAILED(hr))
    {
        return hr;
    }
    
    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
    RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = FEngineLoop::GraphicDevice.Device->CreateRenderTargetView(NewResource.Texture2D.Get(), &RTVDesc, &NewResource.RTV);
    if (FAILED(hr))
    {
        return hr;
    }
    
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = TextureDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;
    hr = FEngineLoop::GraphicDevice.Device->CreateShaderResourceView(NewResource.Texture2D.Get(), &SRVDesc, &NewResource.SRV);
    if (FAILED(hr))
    {
        return hr;
    }

    RenderTargets[Type][DownSampleScale] = NewResource;

    return hr;
}

const FRenderTargetResource* FViewportResource::GetRenderTarget(EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (!HasRenderTarget(Type, DownSampleScale))
    {
        if (FAILED(CreateRenderTarget(Type, DownSampleScale)))
        {
            return nullptr;
        }
    }
    return &RenderTargets[Type][DownSampleScale];
}

bool FViewportResource::HasRenderTarget(EResourceType Type, EDownSampleScale DownSampleScale) const
{
    return RenderTargets.Contains(Type) && RenderTargets[Type].Contains(DownSampleScale);
}

void FViewportResource::ClearRenderTargets(ID3D11DeviceContext* DeviceContext)
{
    for (auto& [Type, Item] : RenderTargets)
    {
        for (auto& [DownSampleScale, Resource] : Item)
        {
            ClearRenderTarget(DeviceContext, Type, DownSampleScale);
        }
    }
}

void FViewportResource::ClearRenderTarget(ID3D11DeviceContext* DeviceContext, EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (HasRenderTarget(Type, DownSampleScale))
    {
        if (const FRenderTargetResource* Resource = GetRenderTarget(Type, DownSampleScale))
        {
            DeviceContext->ClearRenderTargetView(Resource->RTV.Get(), ClearColors[Type].data());
        }
    }
}

std::array<float, 4> FViewportResource::GetClearColor(EResourceType Type) const
{
    if (const std::array<float, 4>* Found = ClearColors.Find(Type))
    {
        return *Found;
    }
    return { 0.0f, 0.0f, 0.0f, 1.0f };
}

void FViewportResource::ReleaseAllResources()
{
    for (auto& [Type, Item] : RenderTargets)
    {
        Item.Empty();
    }
    RenderTargets.Empty();
    for (auto& [Type, Item] : DepthStencils)
    {
        Item.Empty();
    }
    DepthStencils.Empty();
}

void FViewportResource::ReleaseDepthStencil(EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (HasDepthStencil(Type, DownSampleScale))
    {
        DepthStencils[Type].Remove(DownSampleScale);
    }
    else if (DownSampleScale == EDownSampleScale::DSS_MAX)
    {
        DepthStencils[Type].Empty();
    }
}

void FViewportResource::ReleaseRenderTarget(EResourceType Type, EDownSampleScale DownSampleScale)
{
    if (HasRenderTarget(Type, DownSampleScale))
    {
        RenderTargets[Type].Remove(DownSampleScale);
    }
    else if (DownSampleScale == EDownSampleScale::DSS_MAX)
    {
        RenderTargets[Type].Empty();
    }
}


FViewport::FViewport()
    : FViewport(EViewScreenLocation::EVL_MAX)
{
}

FViewport::FViewport(EViewScreenLocation InViewLocation)
    : ViewportResource(std::make_unique<FViewportResource>())
    , ViewLocation(InViewLocation) 
{
}

FViewport::~FViewport()
{
}

void FViewport::Initialize(const FRect& InRect)
{
    Rect = InRect;
    const uint32 Width = static_cast<uint32>(Rect.Width);
    const uint32 Height = static_cast<uint32>(Rect.Height);

    ViewportResource->Initialize(Width, Height);
}

void FViewport::ResizeViewport(const FRect& InRect)
{
    Rect = InRect;
    const uint32 Width = static_cast<uint32>(Rect.Width);
    const uint32 Height = static_cast<uint32>(Rect.Height);

    ViewportResource->Resize(Width, Height);
}

void FViewport::ResizeViewport(const FRect& Top, const FRect& Bottom, const FRect& Left, const FRect& Right)
{
    switch (ViewLocation)
    {
    case EViewScreenLocation::EVL_TopLeft:
        Rect.TopLeftX = Left.TopLeftX;
        Rect.TopLeftY = Top.TopLeftY;
        Rect.Width = Left.Width;
        Rect.Height = Top.Height;
        break;
    case EViewScreenLocation::EVL_TopRight:
        Rect.TopLeftX = Right.TopLeftX;
        Rect.TopLeftY = Top.TopLeftY;
        Rect.Width = Right.Width;
        Rect.Height = Top.Height;
        break;
    case EViewScreenLocation::EVL_BottomLeft:
        Rect.TopLeftX = Left.TopLeftX;
        Rect.TopLeftY = Bottom.TopLeftY;
        Rect.Width = Left.Width;
        Rect.Height = Bottom.Height;
        break;
    case EViewScreenLocation::EVL_BottomRight:
        Rect.TopLeftX = Right.TopLeftX;
        Rect.TopLeftY = Bottom.TopLeftY;
        Rect.Width = Right.Width;
        Rect.Height = Bottom.Height;
        break;
    default:
        return;
    }
    
    const uint32 Width = static_cast<uint32>(Rect.Width);
    const uint32 Height = static_cast<uint32>(Rect.Height);
    ViewportResource->Resize(Width, Height);
}

bool FViewport::bIsHovered(const FVector2D& InPoint) const
{
    return (Rect.TopLeftX <= static_cast<float>(InPoint.X) && static_cast<float>(InPoint.X) <= Rect.TopLeftX + Rect.Width) &&
           (Rect.TopLeftY <= static_cast<float>(InPoint.Y) && static_cast<float>(InPoint.Y) <= Rect.TopLeftY + Rect.Height);
}
