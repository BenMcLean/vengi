/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "TextureConfig.h"
#include "io/IOResource.h"
#include "core/SharedPtr.h"

namespace image {

class Image;
typedef core::SharedPtr<Image> ImagePtr;

}

namespace video {

/**
 * @ingroup Video
 */
class Texture: public io::IOResource {
private:
	core::String _name;
	Id _handle = video::InvalidId;
	int _width;
	int _height;
	int _layerCount = 1;
	TextureConfig _config;
	mutable TextureUnit _boundUnit = TextureUnit::Zero;
	image::ImagePtr _image;
	bool _dummy = false;

	void validate();

public:
	Texture(const image::ImagePtr& image);
	Texture(const TextureConfig& cfg, int width = 1, int height = 1, const core::String& name = "");
	~Texture();
	void shutdown();

	operator Id ();
	TextureType type() const;
	TextureFormat format() const;
	const TextureConfig& config() const;
	int layers() const;
	int width();
	int height();
	Id handle();

	// updates the texture with the new data
	void upload(TextureFormat format, TextureFilter filter, int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(TextureFormat format, int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(int width, int height, const uint8_t* data = nullptr, int index = 1);
	void upload(const uint8_t* data = nullptr, int index = 1);
	void upload(const image::ImagePtr &image, int index = 1);

	/**
	 * @note The returned buffer should get freed with @c core_free()
	 */
	uint8_t* data();

	/**
	 * @sa unbind()
	 */
	void bind(TextureUnit unit = TextureUnit::Zero) const;
	/**
	 * @sa bind()
	 */
	void unbind() const;
};

inline int Texture::layers() const {
	return _layerCount;
}

inline const TextureConfig &Texture::config() const {
	return _config;
}

inline TextureFormat Texture::format() const {
	return _config.format();
}

inline TextureType Texture::type() const {
	return _config.type();
}

typedef core::SharedPtr<Texture> TexturePtr;

class ScopedTexture {
private:
	const TexturePtr _texture;
public:
	ScopedTexture(const TexturePtr& texture, TextureUnit unit = video::TextureUnit::Zero) :
			_texture(texture) {
		_texture->bind(unit);
	}

	~ScopedTexture() {
		_texture->unbind();
	}
};

/** @brief creates empty texture with placeholder pixel in */
extern TexturePtr createEmptyTexture(const core::String& name);

/** @brief creates white texture with placeholder pixel in */
extern TexturePtr createWhiteTexture(const core::String& name);

extern TexturePtr createTextureFromImage(const image::ImagePtr& image);

extern TexturePtr createTextureFromImage(const core::String& filename);

extern TexturePtr createTexture(const TextureConfig& cfg, int width = 1, int height = 1, const core::String& name = "");

extern bool bindTexture(TextureUnit unit, const Texture& texture);

extern bool bindTexture(TextureUnit unit, const TexturePtr& texture);

}
