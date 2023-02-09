/**
 * @file
 */

#pragma once

#include "FrameBufferConfig.h"
#include "Texture.h"
#include "core/NonCopyable.h"
#include "core/SharedPtr.h"

namespace video {

class RenderBuffer;
typedef core::SharedPtr<RenderBuffer> RenderBufferPtr;

/**
 * @brief A frame buffer is a collection of buffers that can be used as the destination for rendering.
 *
 * @sa FrameBufferConfig
 * @ingroup Video
 */
class FrameBuffer : public core::NonCopyable {
	friend class ScopedFrameBuffer;
private:
	ClearFlag _clearFlag = ClearFlag::None;
	Id _fbo = video::InvalidId;
	Id _oldFramebuffer = video::InvalidId;
	TexturePtr _colorAttachments[core::enumVal(FrameBufferAttachment::Max)];
	RenderBufferPtr _bufferAttachments[core::enumVal(FrameBufferAttachment::Max)];

	glm::ivec2 _dimension { 0 };

	int32_t _viewport[4] = {0, 0, 0, 0};

	void addColorAttachment(FrameBufferAttachment attachment, const TexturePtr& texture);
	bool hasColorAttachment(FrameBufferAttachment attachment);

	void addBufferAttachment(FrameBufferAttachment attachment, const RenderBufferPtr& renderBuffer);
	bool hasBufferAttachment(FrameBufferAttachment attachment);

	bool prepareAttachments(const FrameBufferConfig& cfg);
public:
	~FrameBuffer();

	bool init(const FrameBufferConfig& cfg);
	void shutdown();

	bool bindTextureAttachment(FrameBufferAttachment attachment, int layerIndex, bool clear);
	void bind(bool clear);
	void unbind();

	video::Id handle() const;
	TexturePtr texture(FrameBufferAttachment attachment = FrameBufferAttachment::Color0) const;
	image::ImagePtr image(const core::String &name, FrameBufferAttachment attachment = FrameBufferAttachment::Color0) const;

	/**
	 * @return two uv coordinates lower left and upper right (a and c)
	 */
	glm::vec4 uv() const;

	const glm::ivec2& dimension() const;
};

inline const glm::ivec2& FrameBuffer::dimension() const {
	return _dimension;
}

inline video::Id FrameBuffer::handle() const {
	return _fbo;
}

extern bool bindTexture(TextureUnit unit, const FrameBuffer& frameBuffer, FrameBufferAttachment attachment = FrameBufferAttachment::Color0);

}
