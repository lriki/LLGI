
#pragma once

#include "../LLGI.ConstantBuffer.h"
#include "LLGI.BaseDX12.h"
#include "LLGI.GraphicsDX12.h"

using namespace DirectX;

namespace LLGI
{

class ConstantBufferDX12 : public ConstantBuffer
{
private:
	ID3D12Resource* constantBuffer_ = nullptr;
	int memSize_ = 0;
	uint8_t* mapped_ = nullptr;

public:
	bool Initialize(GraphicsDX12* graphics, int32_t size);

	ConstantBufferDX12();
	virtual ~ConstantBufferDX12();

	void* Lock() override;
	void* Lock(int32_t offset, int32_t size) override;
	void Unlock() override;
	int32_t GetSize() override;

	ID3D12Resource* Get() { return constantBuffer_; }
};

} // namespace LLGI
