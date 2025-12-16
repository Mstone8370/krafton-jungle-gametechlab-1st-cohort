#include "IrradianceBakePass.h"

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
}

void FIrradianceBakePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FIrradianceBakePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FIrradianceBakePass::CreateResource()
{
    FRenderPassBase::CreateResource();
}

void FIrradianceBakePass::Release()
{
    FRenderPassBase::Release();
}
