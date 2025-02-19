#include "LLGI.CommandListMetal.h"
#include "LLGI.ConstantBufferMetal.h"
#include "LLGI.GraphicsMetal.h"
#include "LLGI.IndexBufferMetal.h"
#include "LLGI.Metal_Impl.h"
#include "LLGI.PipelineStateMetal.h"
#include "LLGI.TextureMetal.h"
#include "LLGI.VertexBufferMetal.h"

#import <MetalKit/MetalKit.h>

namespace LLGI
{

CommandList_Impl::CommandList_Impl() {}

CommandList_Impl::~CommandList_Impl()
{
	if (commandBuffer != nullptr)
	{
		//[commandBuffer release];
		commandBuffer = nullptr;
	}

	if (renderEncoder != nullptr)
	{
		[renderEncoder release];
	}
}

bool CommandList_Impl::Initialize(Graphics_Impl* graphics)
{
	graphics_ = graphics;
	return true;
}

void CommandList_Impl::Begin()
{
	// is it true?
	commandBuffer = [graphics_->commandQueue commandBuffer];
}

void CommandList_Impl::End() {}

void CommandList_Impl::BeginRenderPass(RenderPass_Impl* renderPass)
{
	if (renderPass->isColorCleared)
	{
		auto r_ = renderPass->clearColor.R / 255.0;
		auto g_ = renderPass->clearColor.G / 255.0;
		auto b_ = renderPass->clearColor.B / 255.0;
		auto a_ = renderPass->clearColor.A / 255.0;

		renderPass->renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
		renderPass->renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(r_, g_, b_, a_);
	}
	else
	{
		renderPass->renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionDontCare;
	}

	renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPass->renderPassDescriptor];
}

void CommandList_Impl::EndRenderPass()
{
	if (renderEncoder)
	{
		[renderEncoder endEncoding];
		renderEncoder = nullptr;
	}
}

void CommandList_Impl::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height)
{
	MTLScissorRect rect;
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;
	[renderEncoder setScissorRect:rect];
}

void CommandList_Impl::SetVertexBuffer(Buffer_Impl* vertexBuffer, int32_t stride, int32_t offset)
{
	[renderEncoder setVertexBuffer:vertexBuffer->buffer offset:offset atIndex:0];
}

CommandListMetal::CommandListMetal() { impl = new CommandList_Impl(); }

CommandListMetal::~CommandListMetal()
{
	for (int w = 0; w < 2; w++)
	{
		for (int f = 0; f < 2; f++)
		{
			[samplers[w][f] release];
			[samplerStates[w][f] release];
		}
	}

	SafeDelete(impl);
	SafeRelease(graphics_);
}

bool CommandListMetal::Initialize(Graphics* graphics)
{
	SafeAddRef(graphics);
	SafeRelease(graphics_);
	graphics_ = graphics;

	auto graphics_metal_ = static_cast<GraphicsMetal*>(graphics);

	// Sampler
	for (int w = 0; w < 2; w++)
	{
		for (int f = 0; f < 2; f++)
		{
			MTLSamplerAddressMode ws[2];
			ws[0] = MTLSamplerAddressModeClampToEdge;
			ws[1] = MTLSamplerAddressModeRepeat;

			MTLSamplerMinMagFilter fsmin[2];
			fsmin[0] = MTLSamplerMinMagFilterNearest;
			fsmin[1] = MTLSamplerMinMagFilterLinear;

			MTLSamplerDescriptor* samplerDescriptor = [MTLSamplerDescriptor new];
			samplerDescriptor.minFilter = fsmin[f];
			samplerDescriptor.magFilter = fsmin[f];
			samplerDescriptor.sAddressMode = ws[w];
			samplerDescriptor.tAddressMode = ws[w];

			samplers[w][f] = samplerDescriptor;
			samplerStates[w][f] = [graphics_metal_->GetImpl()->device newSamplerStateWithDescriptor:samplerDescriptor];
		}
	}

	return impl->Initialize(graphics_metal_->GetImpl());
}

void CommandListMetal::Begin()
{
	impl->Begin();

	CommandList::Begin();
}

void CommandListMetal::End() { impl->End(); }

void CommandListMetal::SetScissor(int32_t x, int32_t y, int32_t width, int32_t height) { impl->SetScissor(x, y, width, height); }

void CommandListMetal::Draw(int32_t pritimiveCount)
{
	BindingVertexBuffer vb_;
	IndexBuffer* ib_ = nullptr;
	PipelineState* pip_ = nullptr;

	bool isVBDirtied = false;
	bool isIBDirtied = false;
	bool isPipDirtied = false;

	GetCurrentVertexBuffer(vb_, isVBDirtied);
	GetCurrentIndexBuffer(ib_, isIBDirtied);
	GetCurrentPipelineState(pip_, isPipDirtied);

	assert(vb_.vertexBuffer != nullptr);
	assert(ib_ != nullptr);
	assert(pip_ != nullptr);

	auto vb = static_cast<VertexBufferMetal*>(vb_.vertexBuffer);
	auto ib = static_cast<IndexBufferMetal*>(ib_);
	auto pip = static_cast<PipelineStateMetal*>(pip_);

	if (isVBDirtied)
	{
		impl->SetVertexBuffer(vb->GetImpl(), vb_.stride, vb_.offset);
	}

	// assign constant buffer
	ConstantBuffer* vcb = nullptr;
	GetCurrentConstantBuffer(ShaderStageType::Vertex, vcb);
	if (vcb != nullptr)
	{
		auto vcb_ = static_cast<ConstantBufferMetal*>(vcb);
		[impl->renderEncoder setVertexBuffer:vcb_->GetImpl()->buffer offset:0 atIndex:1];
	}

	ConstantBuffer* pcb = nullptr;
	GetCurrentConstantBuffer(ShaderStageType::Pixel, pcb);
	if (pcb != nullptr)
	{
		auto pcb_ = static_cast<ConstantBufferMetal*>(pcb);
		[impl->renderEncoder setFragmentBuffer:pcb_->GetImpl()->buffer offset:0 atIndex:1];
	}

	// Assign textures
	for (int stage_ind = 0; stage_ind < (int32_t)ShaderStageType::Max; stage_ind++)
	{
		for (int unit_ind = 0; unit_ind < currentTextures[stage_ind].size(); unit_ind++)
		{
			if (currentTextures[stage_ind][unit_ind].texture == nullptr)
				continue;

			auto texture = (TextureMetal*)currentTextures[stage_ind][unit_ind].texture;
			auto wm = (int32_t)currentTextures[stage_ind][unit_ind].wrapMode;
			auto mm = (int32_t)currentTextures[stage_ind][unit_ind].minMagFilter;

			if (stage_ind == (int32_t)ShaderStageType::Vertex)
			{
				[impl->renderEncoder setVertexTexture:texture->GetImpl()->texture atIndex:unit_ind];
				[impl->renderEncoder setVertexSamplerState:samplerStates[wm][mm] atIndex:unit_ind];
			}

			if (stage_ind == (int32_t)ShaderStageType::Pixel)
			{
				[impl->renderEncoder setFragmentTexture:texture->GetImpl()->texture atIndex:unit_ind];
				[impl->renderEncoder setFragmentSamplerState:samplerStates[wm][mm] atIndex:unit_ind];
			}
		}
	}

	if (isPipDirtied)
	{
		[impl->renderEncoder setRenderPipelineState:pip->GetImpl()->pipelineState];
	}

	// draw
	int indexPerPrim = 0;
	if (pip->Topology == TopologyType::Triangle)
		indexPerPrim = 3;
	if (pip->Topology == TopologyType::Line)
		indexPerPrim = 2;

	MTLPrimitiveType topology = MTLPrimitiveTypeTriangle;
	MTLIndexType indexType = MTLIndexTypeUInt32;

	if (pip->Topology == TopologyType::Line)
	{
		topology = MTLPrimitiveTypeLine;
	}

	if (ib->GetStride() == 2)
	{
		indexType = MTLIndexTypeUInt16;
	}

	[impl->renderEncoder drawIndexedPrimitives:topology
									indexCount:pritimiveCount * indexPerPrim
									 indexType:indexType
								   indexBuffer:ib->GetImpl()->buffer
							 indexBufferOffset:0];
}

void CommandListMetal::BeginRenderPass(RenderPass* renderPass)
{
	auto renderPass_ = static_cast<RenderPassMetal*>(renderPass)->GetImpl();
	impl->BeginRenderPass(renderPass_);
    
    CommandList::BeginRenderPass(renderPass);
}

void CommandListMetal::EndRenderPass() { impl->EndRenderPass(); }

CommandList_Impl* CommandListMetal::GetImpl() { return impl; }

}
