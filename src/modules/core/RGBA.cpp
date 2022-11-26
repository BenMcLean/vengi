/**
 * @file
 */

#include "RGBA.h"
#include <glm/common.hpp>
#include <glm/ext/vector_uint4_sized.hpp>

namespace core {

RGBA RGBA::mix(const core::RGBA rgba1, const core::RGBA rgba2) {
	if (rgba1 == rgba2) {
		return rgba1;
	}
	const glm::u8vec4 c1(rgba1.r, rgba1.g, rgba1.b, rgba1.a);
	const glm::u8vec4 c2(rgba2.r, rgba2.g, rgba2.b, rgba2.a);
	const glm::u8vec4 mixed = glm::mix(c1, c2, 0.5f);
	return core::RGBA(mixed.r, mixed.g, mixed.b, mixed.a);
}

} // namespace core
