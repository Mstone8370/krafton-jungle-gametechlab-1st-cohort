#include "TileLightCullingPass.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

void FTileLightCullingPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManage);
}

void FTileLightCullingPass::PrepareRenderArr()
{
    FRenderPassBase::PrepareRenderArr();
}

void FTileLightCullingPass::ClearRenderArr()
{
    FRenderPassBase::ClearRenderArr();
}

void FTileLightCullingPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FTileLightCullingPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FTileLightCullingPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}
