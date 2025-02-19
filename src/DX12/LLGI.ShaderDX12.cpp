
#include "LLGI.ShaderDX12.h"
#include "../LLGI.Shader.h"
#include "LLGI.CompilerDX12.h"

namespace LLGI
{

bool ShaderDX12::Initialize(DataStructure* data, int32_t count)
{
	auto p = static_cast<uint8_t*>(data->Data);
	data_.resize(data->Size);
	memcpy(data_.data(), p, data_.size());
	return true;
}

} // namespace LLGI
