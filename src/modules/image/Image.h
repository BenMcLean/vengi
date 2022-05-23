/**
 * @file
 */

#pragma once

#include "io/IOResource.h"
#include "io/File.h"
#include "core/SharedPtr.h"
#include "io/Stream.h"

namespace image {

/**
 * @brief Wrapper for image loading
 */
class Image: public io::IOResource {
private:
	core::String _name;
	int _width = -1;
	int _height = -1;
	int _depth = -1;
	uint8_t* _data = nullptr;

public:
	Image(const core::String& name);
	~Image();

	bool load(const io::FilePtr& file);
	bool load(const uint8_t* buffer, int length);
	/**
	 * Loads a raw RGBA buffer
	 */
	bool loadRGBA(const uint8_t* buffer, int length, int width, int height);

	static void flipVerticalRGBA(uint8_t *pixels, int w, int h);
	static bool writePng(io::SeekableWriteStream &stream, const uint8_t* buffer, int width, int height, int depth);
	static bool writePng(const char *name, const uint8_t *buffer, int width, int height, int depth);
	bool writePng() const;
	core::String pngBase64() const;

	const uint8_t* at(int x, int y) const;

	inline const core::String& name() const {
		return _name;
	}

	inline const uint8_t* data() const {
		return _data;
	}

	inline int width() const {
		return _width;
	}

	inline int height() const {
		return _height;
	}

	inline int depth() const {
		return _depth;
	}
};

typedef core::SharedPtr<Image> ImagePtr;

// creates an empty image
inline ImagePtr createEmptyImage(const core::String& name) {
	return core::make_shared<Image>(name);
}

extern uint8_t* createPng(const void *pixels, int width, int height, int depth, int *pngSize);
extern ImagePtr loadImage(const io::FilePtr& file, bool async = true);
extern ImagePtr loadImage(const core::String& filename, bool async = true);

}
