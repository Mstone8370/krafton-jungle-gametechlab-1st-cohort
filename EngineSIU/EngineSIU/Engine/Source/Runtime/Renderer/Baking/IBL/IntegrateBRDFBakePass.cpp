#include "IntegrateBRDFBakePass.h"

FIntegrateBRDFBakePass::FIntegrateBRDFBakePass()
    : BakePath(L"Assets/Texture/IBL/LUT/LUT.dds")
    , NumSamples(1024)
    , bBake(false)
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

void FIntegrateBRDFBakePass::Bake()
{
}

void FIntegrateBRDFBakePass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FIntegrateBRDFBakePass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FIntegrateBRDFBakePass::CreateResource()
{
    
}

void FIntegrateBRDFBakePass::Release()
{
    FRenderPassBase::Release();
}
