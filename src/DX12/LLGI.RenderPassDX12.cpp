#include "LLGI.RenderPassDX12.h"
#include "LLGI.GraphicsDX12.h"
#include "LLGI.TextureDX12.h"

namespace LLGI
{

RenderPassDX12::RenderPassDX12(GraphicsDX12* graphics, bool isStrongRef) : graphics_(graphics), isStrongRef_(isStrongRef)
{
	if (isStrongRef_)
	{
		SafeAddRef(graphics_);
	}
}

RenderPassDX12 ::~RenderPassDX12()
{
	if (isStrongRef_)
	{
		SafeRelease(graphics_);
	}
}

bool RenderPassDX12::Initialize() { return false; }

bool RenderPassDX12::Initialize(TextureDX12** textures, int numTextures, TextureDX12* depthTexture)
{
	isScreen_ = false;
	renderPass_ = textures[0]->Get();
	textures_ = textures;
	auto size = textures[0]->GetSizeAs2D();
	screenWindowSize.X = size.X;
	screenWindowSize.Y = size.Y;

	return renderPass_ != nullptr;
}

RenderPassPipelineState* RenderPassDX12::CreateRenderPassPipelineState()
{
	auto ret = renderPassPipelineState.get();
	SafeAddRef(ret);
	return ret;
}

RenderPassPipelineStateDX12* RenderPassDX12::GetRenderPassPipelineState()
{
	auto ret = renderPassPipelineState.get();
	SafeAddRef(ret);
	return ret;
}

} // namespace LLGI
