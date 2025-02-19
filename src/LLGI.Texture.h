
#pragma once

#include "LLGI.Base.h"

namespace LLGI
{

class Texture : public ReferenceObject
{
private:
public:
	Texture() = default;
	virtual ~Texture() = default;

	virtual void* Lock();
	virtual void Unlock();
	virtual Vec2I GetSizeAs2D();
	virtual bool IsRenderTexture() const;
	virtual bool IsDepthTexture() const;
};

} // namespace LLGI