/**
 * @file
 */

#include "Math.h"
#include <glm/mat4x4.hpp>

namespace math {

glm::ivec3 transform(const glm::mat4x4 &mat, const glm::ivec3 &pos, const glm::vec3 &pivot) {
	const glm::vec4 v((float)pos.x - 0.5f - pivot.x, (float)pos.y - 0.5f - pivot.y, (float)pos.z - 0.5f - pivot.z,
					  1.0f);
	const glm::vec3 &e = glm::vec3(mat * v) + 0.5f + pivot;
	const glm::ivec3 f = glm::floor(e);
	return f;
}

glm::vec3 transform(const glm::mat4x4 &mat, const glm::vec3 &pos, const glm::vec3 &pivot) {
	const glm::vec4 v(pos.x - 0.5f - pivot.x, pos.y - 0.5f - pivot.y, pos.z - 0.5f - pivot.z,
					  1.0f);
	return glm::vec3(mat * v) + 0.5f + pivot;
}

} // namespace math
