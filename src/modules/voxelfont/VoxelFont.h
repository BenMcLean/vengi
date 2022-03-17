/**
 * @file
 */

#pragma once

#include "core/Color.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Mesh.h"
#include <unordered_map>
#include "core/UTF8.h"
#include "core/StringUtil.h"

struct stbtt_fontinfo;

namespace voxelfont {

/**
 * @brief Will take any TTF font and rasterizes into voxels
 */
class VoxelFont {
private:
	std::unordered_map<uint32_t, voxel::Mesh*> _cache;
	stbtt_fontinfo* _font = nullptr;
	uint8_t *_ttfBuffer = nullptr;
	int _size = 0;
	float _scale = 0.0f;
	int _height = 0;
	int _spaceWidth = 0;
	int _thickness = 1;
	int _lineGap = 0;
	/**
	 * ascent is the coordinate above the baseline the font extends;
	 */
	int _ascent = 0;
	/**
	 * descent is the coordinate below the baseline the font extends (i.e. it is typically negative)
	 */
	int _descent = 0;
	uint8_t _optionMask = 0u;

	bool renderGlyphs(const char* string);

	void getGlyphMetrics(int c, int& advance, int& xOffset, int& yOffset, int& ascent);

public:
	~VoxelFont();

	static const uint8_t MergeQuads        = 1 << 0;
	static const uint8_t OriginUpperLeft   = 1 << 1;

	bool init(const char* font, uint8_t fontSize, int thickness, uint8_t optionMask, const char* glyphs);
	void shutdown();

	int stringWidth(const char *str, int length) const;

	inline int lineHeight() const {
		return _size;
	}

	template<class T, class FUNC>
	int render(const char* string, core::DynamicArray<T>& out, voxel::IndexArray& indices, FUNC&& func) {
		const char **s = &string;
		const int newlines = core::string::count(string, '\n');

		int xBase = 0;
		int yBase = newlines * lineHeight();

		int charCount = 0;

		for (int c = core::utf8::next(s); c != -1; c = core::utf8::next(s), ++charCount) {
			if (c == ' ') {
				xBase += _spaceWidth;
				continue;
			}
			if (c == '\n') {
				xBase = 0;
				yBase -= lineHeight();
				continue;
			}

			auto iter = _cache.find(c);
			if (iter == _cache.end()) {
				xBase += _spaceWidth;
				Log::trace("Could not find character glyph cache for %i", c);
				continue;
			}

			int advance;
			int xOffset, yOffset;
			int ascent;
			getGlyphMetrics(c, advance, xOffset, yOffset, ascent);

			const int x = xBase + xOffset;
			const int y = yBase + yOffset + ascent;

			const voxel::Mesh* mesh = iter->second;
			const voxel::IndexType* meshIndices = mesh->getRawIndexData();
			const voxel::VoxelVertex* meshVertices = mesh->getRawVertexData();

			const size_t meshNumberIndices = mesh->getNoOfIndices();
			core_assert(meshNumberIndices > 0);
			const size_t meshNumberVertices = mesh->getNoOfVertices();
			core_assert(meshNumberVertices > 0);

			const size_t positionSize = out.size();
			const size_t indicesSize = indices.size();
			out.reserve(positionSize + meshNumberVertices);
			indices.reserve(indicesSize + meshNumberIndices);

			for (size_t mv = 0; mv < meshNumberVertices; ++mv) {
				const voxel::VoxelVertex& vp = meshVertices[mv];
				func(vp, out, x, y);
			}
			for (size_t mi = 0; mi < meshNumberIndices; ++mi) {
				// offset by the already added vertices
				indices.push_back(meshIndices[mi] + positionSize);
			}

			xBase += advance;
		}
		return charCount;
	}

	int render(const char* string, core::DynamicArray<glm::vec4>& pos, voxel::IndexArray& indices);
	int render(const char* string, voxel::VertexArray& vertices, voxel::IndexArray& indices);
};

}
