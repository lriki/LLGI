#pragma once

#include "LLGI.PipelineStateDX12.h"
#include "../LLGI.CommandList.h"
#include "../LLGI.PipelineState.h"
#include "LLGI.ShaderDX12.h"

namespace LLGI
{

PipelineStateDX12::PipelineStateDX12(GraphicsDX12* graphics)
{
	SafeAddRef(graphics);
	graphics_ = CreateSharedPtr(graphics);
	shaders_.fill(nullptr);
}

PipelineStateDX12::~PipelineStateDX12()
{
	for (auto& shader : shaders_)
	{
		SafeRelease(shader);
	}
	SafeRelease(pipelineState_);
	SafeRelease(signature_);
	SafeRelease(rootSignature_);
}

void PipelineStateDX12::SetShader(ShaderStageType stage, Shader* shader)
{
	SafeAddRef(shader);
	SafeRelease(shaders_[static_cast<int>(stage)]);
	shaders_[static_cast<int>(stage)] = shader;
}

void PipelineStateDX12::Compile()
{
	CreateRootSignature();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};

	for (size_t i = 0; i < shaders_.size(); i++)
	{
		auto& shaderData = static_cast<ShaderDX12*>(shaders_[i])->GetData();

		if (i == static_cast<int>(ShaderStageType::Pixel))
		{
			pipelineStateDesc.PS.pShaderBytecode = shaderData.data();
			pipelineStateDesc.PS.BytecodeLength = shaderData.size();
		}
		else if (i == static_cast<int>(ShaderStageType::Vertex))
		{
			pipelineStateDesc.VS.pShaderBytecode = shaderData.data();
			pipelineStateDesc.VS.BytecodeLength = shaderData.size();
		}
	}

	// setup a vertex layout
	std::array<D3D12_INPUT_ELEMENT_DESC, 16> elementDescs;
	elementDescs.fill(D3D12_INPUT_ELEMENT_DESC{});
	int32_t elementOffset = 0;

	for (int i = 0; i < VertexLayoutCount; i++)
	{
		elementDescs[i].SemanticName = this->VertexLayoutNames[i].c_str();
		elementDescs[i].SemanticIndex = 0;
		elementDescs[i].AlignedByteOffset = elementOffset;
		elementDescs[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;

		if (VertexLayouts[i] == VertexLayoutFormat::R32G32_FLOAT)
		{
			elementDescs[i].Format = DXGI_FORMAT_R32G32_FLOAT;
			elementOffset += sizeof(float) * 2;
		}
		else if (VertexLayouts[i] == VertexLayoutFormat::R32G32B32_FLOAT)
		{
			elementDescs[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			elementOffset += sizeof(float) * 3;
		}
		else if (VertexLayouts[i] == VertexLayoutFormat::R32G32B32A32_FLOAT)
		{
			elementDescs[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			elementOffset += sizeof(float) * 4;
		}
		else if (VertexLayouts[i] == VertexLayoutFormat::R8G8B8A8_UNORM)
		{
			elementDescs[i].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			elementOffset += sizeof(float) * 1;
		}
		else if (VertexLayouts[i] == VertexLayoutFormat::R8G8B8A8_UINT)
		{
			elementDescs[i].Format = DXGI_FORMAT_R8G8B8A8_UINT;
			elementOffset += sizeof(float) * 1;
		}
		else
		{
			assert(0);
		}
	}

	// setup a topology
	if (Topology == TopologyType::Triangle)
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	if (Topology == TopologyType::Line)
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

	// TODO...(generate from parameters)
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	if (Culling == CullingMode::Clockwise)
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	if (Culling == CullingMode::CounterClockwise)
		rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
	if (Culling == CullingMode::DoubleSide)
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// setup render target blend
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	if (IsBlendEnabled)
	{
		renderTargetBlendDesc.BlendEnable = TRUE;

		std::array<D3D12_BLEND_OP, 10> blendOps;
		blendOps[static_cast<int>(BlendEquationType::Add)] = D3D12_BLEND_OP_ADD;
		blendOps[static_cast<int>(BlendEquationType::Sub)] = D3D12_BLEND_OP_SUBTRACT;
		blendOps[static_cast<int>(BlendEquationType::ReverseSub)] = D3D12_BLEND_OP_REV_SUBTRACT;
		blendOps[static_cast<int>(BlendEquationType::Min)] = D3D12_BLEND_OP_MIN;
		blendOps[static_cast<int>(BlendEquationType::Max)] = D3D12_BLEND_OP_MAX;

		std::array<D3D12_BLEND, 20> blendFuncs;
		blendFuncs[static_cast<int>(BlendFuncType::Zero)] = D3D12_BLEND_ZERO;
		blendFuncs[static_cast<int>(BlendFuncType::One)] = D3D12_BLEND_ONE;
		blendFuncs[static_cast<int>(BlendFuncType::SrcColor)] = D3D12_BLEND_SRC_COLOR;
		blendFuncs[static_cast<int>(BlendFuncType::OneMinusSrcColor)] = D3D12_BLEND_INV_SRC_COLOR;
		blendFuncs[static_cast<int>(BlendFuncType::SrcAlpha)] = D3D12_BLEND_SRC_ALPHA;
		blendFuncs[static_cast<int>(BlendFuncType::OneMinusSrcAlpha)] = D3D12_BLEND_INV_SRC_ALPHA;
		blendFuncs[static_cast<int>(BlendFuncType::DstColor)] = D3D12_BLEND_DEST_COLOR;
		blendFuncs[static_cast<int>(BlendFuncType::OneMinusDstColor)] = D3D12_BLEND_INV_DEST_COLOR;
		blendFuncs[static_cast<int>(BlendFuncType::DstAlpha)] = D3D12_BLEND_DEST_ALPHA;
		blendFuncs[static_cast<int>(BlendFuncType::OneMinusDstAlpha)] = D3D12_BLEND_INV_DEST_ALPHA;

		renderTargetBlendDesc.SrcBlend = blendFuncs[static_cast<int>(BlendSrcFunc)];
		renderTargetBlendDesc.DestBlend = blendFuncs[static_cast<int>(BlendDstFunc)];
		renderTargetBlendDesc.SrcBlendAlpha = blendFuncs[static_cast<int>(BlendSrcFuncAlpha)];
		renderTargetBlendDesc.DestBlendAlpha = blendFuncs[static_cast<int>(BlendDstFuncAlpha)];
		renderTargetBlendDesc.BlendOp = blendOps[static_cast<int>(BlendEquationRGB)];
		renderTargetBlendDesc.BlendOpAlpha = blendOps[static_cast<int>(BlendEquationAlpha)];

		// TODO:
		renderTargetBlendDesc.LogicOpEnable = FALSE;
		renderTargetBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	}
	else
		renderTargetBlendDesc.BlendEnable = FALSE;

	// ?
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// setup a blend state
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		blendDesc.RenderTarget[i] = renderTargetBlendDesc;
	}

	pipelineStateDesc.InputLayout.pInputElementDescs = elementDescs.data();
	pipelineStateDesc.InputLayout.NumElements = VertexLayoutCount;
	pipelineStateDesc.pRootSignature = rootSignature_;
	pipelineStateDesc.RasterizerState = rasterizerDesc;
	pipelineStateDesc.BlendState = blendDesc;

	// setup a depth stencil
	std::array<D3D12_COMPARISON_FUNC, 10> depthCompareOps;
	depthCompareOps[static_cast<int>(DepthFuncType::Never)] = D3D12_COMPARISON_FUNC_NEVER;
	depthCompareOps[static_cast<int>(DepthFuncType::Less)] = D3D12_COMPARISON_FUNC_LESS;
	depthCompareOps[static_cast<int>(DepthFuncType::Equal)] = D3D12_COMPARISON_FUNC_EQUAL;
	depthCompareOps[static_cast<int>(DepthFuncType::LessEqual)] = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthCompareOps[static_cast<int>(DepthFuncType::Greater)] = D3D12_COMPARISON_FUNC_GREATER;
	depthCompareOps[static_cast<int>(DepthFuncType::NotEqual)] = D3D12_COMPARISON_FUNC_NOT_EQUAL;
	depthCompareOps[static_cast<int>(DepthFuncType::GreaterEqual)] = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	depthCompareOps[static_cast<int>(DepthFuncType::Always)] = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthFunc = depthCompareOps[static_cast<int>(DepthFunc)];
	depthStencilDesc.DepthEnable = IsDepthTestEnabled;
	depthStencilDesc.StencilEnable = IsDepthTestEnabled;
	// TODO

	// TODO (from renderpass)
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	pipelineStateDesc.SampleDesc.Count = 1;

	pipelineStateDesc.SampleMask = UINT_MAX;

	auto hr = graphics_->GetDevice()->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState_));

	if (FAILED(hr))
	{
		goto FAILED_EXIT;
	}

	return;

FAILED_EXIT:
	SafeRelease(pipelineState_);
	return;
}

bool PipelineStateDX12::CreateRootSignature()
{
	D3D12_DESCRIPTOR_RANGE ranges[3] = {{}, {}, {}};
	D3D12_ROOT_PARAMETER rootParameters[2] = {{}, {}};

	// descriptor range for constant buffer view
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].NumDescriptors = static_cast<int>(ShaderStageType::Max);
	ranges[0].BaseShaderRegister = 0;
	ranges[0].RegisterSpace = 0;
	ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// descriptor range for shader resorce view
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[1].NumDescriptors = NumTexture * static_cast<int>(ShaderStageType::Max);
	ranges[1].BaseShaderRegister = 0;
	ranges[1].RegisterSpace = 0;
	ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// descriptor range for sampler
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	ranges[2].NumDescriptors = NumTexture * static_cast<int>(ShaderStageType::Max);
	ranges[2].BaseShaderRegister = 0;
	ranges[2].RegisterSpace = 0;
	ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// descriptor table for CBV/SRV
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 2;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// descriptor table for sampler
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[2];
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = 2;
	desc.pParameters = rootParameters;
	desc.NumStaticSamplers = 0;
	desc.pStaticSamplers = nullptr;
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	auto hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_, nullptr);

	if (FAILED(hr))
	{
		goto FAILED_EXIT;
	}

	hr = graphics_->GetDevice()->CreateRootSignature(
		0, signature_->GetBufferPointer(), signature_->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	if (FAILED(hr))
	{
		goto FAILED_EXIT;
		SafeRelease(signature_);
	}
	return true;

FAILED_EXIT:
	return false;
}

} // namespace LLGI
