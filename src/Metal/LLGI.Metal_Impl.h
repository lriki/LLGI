#pragma once

#include "../LLGI.Graphics.h"
#include "../LLGI.PipelineState.h"

#import <MetalKit/MetalKit.h>

namespace LLGI
{

struct CommandList_Impl;
struct Buffer_Impl;
struct Texture_Impl;
    
struct Graphics_Impl
{
	id<MTLDevice> device = nullptr;
	id<MTLCommandQueue> commandQueue = nullptr;
	id<CAMetalDrawable> drawable;

	Graphics_Impl();
	virtual ~Graphics_Impl();
	bool Initialize();
	void Execute(CommandList_Impl* commandBuffer);
};

struct RenderPass_Impl
{
	MTLRenderPassDescriptor* renderPassDescriptor;
	Color8 clearColor;
	bool isColorCleared;
	bool isDepthCleared;
	MTLPixelFormat pixelFormat = MTLPixelFormatBGRA8Unorm;

	RenderPass_Impl();
	~RenderPass_Impl();
	bool Initialize();
    void UpdateTarget(Graphics_Impl* graphics);
    void UpdateTarget(Texture_Impl** textures, int32_t textureCount, Texture_Impl* depthTexture);
};

struct RenderPassPipelineState_Impl
{
	MTLPixelFormat pixelFormat = MTLPixelFormatBGRA8Unorm;
};

struct CommandList_Impl
{
	Graphics_Impl* graphics_ = nullptr;
	id<MTLCommandBuffer> commandBuffer = nullptr;
	id<MTLRenderCommandEncoder> renderEncoder = nullptr;

	CommandList_Impl();
	~CommandList_Impl();

	bool Initialize(Graphics_Impl* graphics);

	void Begin();
	void End();
	void BeginRenderPass(RenderPass_Impl* renderPass);
	void EndRenderPass();
	void SetScissor(int32_t x, int32_t y, int32_t width, int32_t height);
	void SetVertexBuffer(Buffer_Impl* vertexBuffer, int32_t stride, int32_t offset);
};

struct Shader_Impl
{
	id<MTLLibrary> library = nullptr;

	Shader_Impl();
	~Shader_Impl();
	bool Initialize(Graphics_Impl* graphics, void* data, int size);
};

struct PipelineState_Impl
{
	id<MTLRenderPipelineState> pipelineState;
	id<MTLDepthStencilState> depthStencilState = nullptr;
	MTLRenderPipelineDescriptor* pipelineStateDescriptor = nullptr;

	PipelineState_Impl();
	~PipelineState_Impl();

	void Compile(PipelineState* self, Graphics_Impl* graphics);
};

struct Buffer_Impl
{
	id<MTLBuffer> buffer = nullptr;
	int32_t size_;

	Buffer_Impl();
	virtual ~Buffer_Impl();

	bool Initialize(Graphics_Impl* graphics, int32_t size);

	void* GetBuffer();
};

struct Texture_Impl
{
	id<MTLTexture> texture;
	Vec2I size_;

	Texture_Impl();
	virtual ~Texture_Impl();

	bool Initialize(Graphics_Impl* graphics, const Vec2I& size, bool isRenderTexture, bool isDepthTexture);

	void Write(const uint8_t* data);
};

} // namespace LLGI
