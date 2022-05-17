/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "math/Math.h"
#include <stdint.h>

namespace animation {

/**
 * @ingroup Animation
 */
struct alignas(16) Vertex {
	glm::vec3h pos;
	uint8_t colorIndex;
	uint8_t boneId;
	uint8_t ambientOcclusion;
	uint8_t padding;

	constexpr Vertex() :
			pos{}, colorIndex(0), boneId(0), ambientOcclusion(0), padding(0) {
	}

	Vertex(const glm::vec3 &_pos, uint8_t _colorIndex, uint8_t _boneId, uint8_t _ambientOcclusion) :
			pos(_pos), colorIndex(_colorIndex), boneId(_boneId), ambientOcclusion(_ambientOcclusion), padding(0) {
	}
};
static_assert(sizeof(Vertex) == 16, "Unexpected size of the vertex struct");

/**
 * @ingroup Animation
 */
using IndexType = uint16_t;
/**
 * @ingroup Animation
 */
using Vertices = core::DynamicArray<Vertex>;
/**
 * @ingroup Animation
 */
using Indices = core::DynamicArray<IndexType>;

}
