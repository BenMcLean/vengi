/**
 * @file
 */

#include "TreeGenerator.h"

namespace voxelgenerator {
namespace tree {

Tree::Tree(const glm::ivec3& position, int trunkHeight, int branchLength,
	int crownWidth, int crownHeight, int crownDepth, float branchSize, unsigned int seed, float trunkSizeFactor) :
			SpaceColonization(glm::ivec3(position.x, position.y + trunkHeight, position.z),
					branchLength, crownWidth, crownHeight, crownDepth, branchSize, seed),
			_trunkHeight(trunkHeight), _trunkSizeFactor(trunkSizeFactor) {
	_root->_position.y -= (float)trunkHeight;
	_position.y -= (float)trunkHeight;
	generateBranches(_branches, glm::up(), (float)_trunkHeight, (float)_branchLength);
}

// TODO: use the PoolAllocator here
void Tree::generateBranches(Branches& branches, const glm::vec3& direction, float maxSize, float branchLength) const {
	float branchSize = _branchSize;
	const float deviation = 0.5f;
	const float random1 = _random.randomBinomial(deviation);
	const glm::vec3 d1 = direction + random1;
	const glm::vec3& branchPos1 = _position + d1 * branchLength;
	Branch* current = new Branch(_root, branchPos1, d1, branchSize);
	branches.put(current->_position, current);

	// grow until the max distance between root and branch is reached
	const float size2 = maxSize * maxSize;
	while (glm::distance2(current->_position, _root->_position) < size2) {
		const float random2 = _random.randomBinomial(deviation);
		const glm::vec3 d2 = direction + random2;
		const glm::vec3& branchPos2 = current->_position + d2 * branchLength;
		Branch *branch = new Branch(current, branchPos2, d2, branchSize);
		branches.put(branch->_position, branch);
		current = branch;
		branchSize *= _trunkSizeFactor;
		branchLength *= _branchSizeFactor;
	}
}

}
}
