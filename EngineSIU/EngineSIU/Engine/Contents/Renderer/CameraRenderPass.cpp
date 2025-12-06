#include "CameraRenderPass.h"

#include "RendererHelpers.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/Engine.h"
#include "GameFramework/RabbitPlayer.h"
#include "UnrealEd/EditorViewportClient.h"
#include "World/World.h"

void FCameraRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
}

void FCameraRenderPass::PrepareRenderArr()
{
    if (GEngine->ActiveWorld->WorldType == EWorldType::PIE)
    {
        if (APlayerController* PC = GEngine->ActiveWorld->GetPlayerController())
        {
            if (ARabbitPlayer* Rabbit = Cast<ARabbitPlayer>(PC->GetPawn()))
            {
                if (Rabbit->IsADS())
                {
                    bShouldRender = true;
                    RabbitPawn = Rabbit;
                    return;
                }
            }
        }
    }
    bShouldRender = false;
    RabbitPawn = nullptr;
}

void FCameraRenderPass::ClearRenderArr()
{
    
}

void FCameraRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);
    
    APlayerCameraManager* PCM = nullptr;
    float TextureAspectRatio = 1.f;
    
    if (bShouldRender && RabbitPawn)
    {
        auto Texture = FEngineLoop::ResourceManager.GetTexture(L"Contents/Texture/CameraViewFinder.png");
        Graphics->DeviceContext->PSSetShaderResources(0, 1, &Texture->TextureSRV);
        
        D3D11_TEXTURE2D_DESC Desc;
        Texture->Texture->GetDesc(&Desc);
        const float TextureHeight = static_cast<float>(Desc.Height);
        const float TextureWidth = static_cast<float>(Desc.Width);
        TextureAspectRatio = TextureWidth / TextureHeight;
        
        if (APlayerController* PC = RabbitPawn->GetPlayerController())
        {
            PCM = PC->PlayerCameraManager;
        }
    }
    else
    {
        ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
        Graphics->DeviceContext->PSSetShaderResources(0, 1, NullSRV);
    }
    
    FQuadTransform QuadTransform;
    if (PCM)
    {
        const float LetterBoxAspectRatio = PCM->GetLetterBoxRatio();

        const FRect& Rect = Viewport->GetViewport()->GetRect();
        const float ScreenWidth = Rect.Width;
        const float ScreenHeight = Rect.Height;
        const float ScreenAspectRatio = ScreenWidth / ScreenHeight;

        float ActualSceneRenderWidth;
        float ActualSceneRenderHeight;
        if (ScreenAspectRatio > LetterBoxAspectRatio)
        {
            ActualSceneRenderHeight = ScreenHeight;
            ActualSceneRenderWidth = ActualSceneRenderHeight * LetterBoxAspectRatio;
        }
        else
        {
            ActualSceneRenderWidth = ScreenWidth;
            ActualSceneRenderHeight = ActualSceneRenderWidth / LetterBoxAspectRatio;
        }
        
        float TargetTextureWidth;
        float TargetTextureHeight;
        if (LetterBoxAspectRatio > TextureAspectRatio)
        {
            TargetTextureHeight = ActualSceneRenderHeight;
            TargetTextureWidth = TargetTextureHeight * TextureAspectRatio;
        }
        else
        {
            TargetTextureWidth = ActualSceneRenderWidth;
            TargetTextureHeight = TargetTextureWidth / TextureAspectRatio;
        }

        QuadTransform.Scale.X = TargetTextureWidth / ScreenWidth;
        QuadTransform.Scale.Y = TargetTextureHeight / ScreenHeight;

        constexpr float MarginScaleFactor = 0.8f;
        QuadTransform.Scale.X *= MarginScaleFactor;
        QuadTransform.Scale.Y *= MarginScaleFactor;
    }
    
    BufferManager->UpdateConstantBuffer<FQuadTransform>("FSlateTransform", QuadTransform);
    Graphics->DeviceContext->Draw(6, 0);

    CleanUpRender(Viewport);
}

void FCameraRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    constexpr EResourceType ResourceType = EResourceType::ERT_CameraW13; 
    const FRenderTargetRHI* RenderTargetRHI = Viewport->GetViewportResource()->GetRenderTarget(ResourceType);
    ID3D11RenderTargetView* RTV = RenderTargetRHI->RTV.Get();
    Graphics->DeviceContext->OMSetRenderTargets(1, &RTV, nullptr);
    
    Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    Graphics->DeviceContext->IASetInputLayout(nullptr);
    
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(L"W13CameraVertex");
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);

    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(L"W13CameraPixel");
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);

    BufferManager->BindConstantBuffer(TEXT("FSlateTransform"), 0, EShaderStage::Vertex);
}

void FCameraRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 1, NullSRV);
}

void FCameraRenderPass::CreateResource()
{
    HRESULT hr = ShaderManager->AddVertexShader(L"W13CameraVertex", L"Shaders/W13/W13CameraShader.hlsl", "mainVS");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to load vertex shader", L"Error", MB_OK);
    }

    hr = ShaderManager->AddPixelShader(L"W13CameraPixel", L"Shaders/W13/W13CameraShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to load Pixel shader", L"Error", MB_OK);
    }
}
