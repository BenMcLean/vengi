/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "io/Filesystem.h"
#include "Texture.h"
#include "core/String.h"
#include "core/SharedPtr.h"
#include "core/collection/StringMap.h"

namespace video {

/**
 * @ingroup Video
 */
class TexturePool : public core::IComponent {
private:
	io::FilesystemPtr _filesystem;
	core::StringMap<TexturePtr> _cache;
	TexturePtr _empty;
public:
	TexturePool(const io::FilesystemPtr& filesystem);

	video::TexturePtr load(const core::String& name, bool emptyAsFallback);

	bool init() override;
	void shutdown() override;
	void clear();
};

typedef core::SharedPtr<TexturePool> TexturePoolPtr;

}
