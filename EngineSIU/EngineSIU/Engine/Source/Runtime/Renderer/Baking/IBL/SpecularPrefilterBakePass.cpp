#include "SpecularPrefilterBakePass.h"

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
}

void FSpecularPrefilterBakePass::EnqueueSpecularPrefilterBake(const FWString& FilePath)
{
    BakeQueue.Enqueue(FilePath);
}

void FSpecularPrefilterBakePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FSpecularPrefilterBakePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FSpecularPrefilterBakePass::CreateResource()
{
    FRenderPassBase::CreateResource();
}

void FSpecularPrefilterBakePass::Release()
{
    FRenderPassBase::Release();
}
