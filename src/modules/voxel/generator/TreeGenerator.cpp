/**
 * @file
 */

#include "TreeGenerator.h"

#include <unordered_map>
#include <functional>

namespace voxel {
namespace tree {

AttractionPoint::AttractionPoint(const glm::vec3& position) :
		_position(position) {
}

Branch::Branch(Branch* parent, const glm::vec3& position, const glm::vec3& growDirection, float size) :
		_parent(parent), _position(position), _growDirection(growDirection), _originalGrowDirection(growDirection), _size(size) {
	if (_parent) {
		_parent->_children.push_back(this);
	}
}

void Branch::reset() {
	_attractionPointInfluence = 0;
	_growDirection = _originalGrowDirection;
}

Tree::Tree(const glm::ivec3& position, int trunkHeight, int branchLength,
	int crownWidth, int crownHeight, int crownDepth, float branchSize, int seed) :
		_position(position), _attractionPointCount(crownDepth * 10), _crownWidth(crownWidth),
		_crownDepth(crownDepth), _crownHeight(crownHeight), _trunkHeight(trunkHeight),
		_minDistance2(36), _maxDistance2(100),
		_branchLength(branchLength), _branchSize(branchSize), _random(seed) {
	_root = new Branch(nullptr, _position, glm::up, _branchSize);
	_branches.insert(std::make_pair(_root->_position, _root));

	fillAttractionPoints();
	generateBranches(_branches, glm::up, _trunkHeight, _branchLength);
}

Tree::~Tree() {
	for (auto &e : _branches) {
		delete e.second;
	}
	_root = nullptr;
	_branches.clear();
	_attractionPoints.clear();
}

void Tree::fillAttractionPoints() {
	const int radius = _crownWidth / 2;
	const glm::ivec3 mins(_position.x - (_crownWidth / 2), _position.y + _trunkHeight, _position.z - (_crownDepth / 2));
	const glm::ivec3 maxs(mins.x + _crownWidth, mins.y + _crownHeight, mins.z + _crownDepth);
	const int radiusSquare = radius * radius;
	const glm::vec3 center((mins + maxs) / 2);
	for (int i = 0; i < _attractionPointCount; ++i) {
		const glm::vec3 location(
				_random.random(mins.x, maxs.x),
				_random.random(mins.y, maxs.y),
				_random.random(mins.z, maxs.z));
		if (glm::distance2(location, center) < radiusSquare) {
			_attractionPoints.emplace_back(AttractionPoint(location));
		}
	}
}

void Tree::generateBranches(Branches& branches, const glm::vec3& direction, float maxSize, float branchLength) const {
	float branchSize = _branchSize;
	const float deviation = 0.5f;
	const float random1 = _random.randomBinomial(deviation);
	const glm::vec3 d1 = direction + random1;
	const glm::vec3& branchPos1 = _position + d1 * branchLength;
	Branch* current = new Branch(_root, branchPos1, d1, branchSize);
	branches.insert(std::make_pair(current->_position, current));

	// grow until the max distance between root and branch is reached
	const float size2 = maxSize * maxSize;
	while (glm::distance2(current->_position, _root->_position) < size2) {
		const float random2 = _random.randomBinomial(deviation);
		const glm::vec3 d2 = direction + random2;
		const glm::vec3& branchPos2 = current->_position + d2 * branchLength;
		Branch *branch = new Branch(current, branchPos2, d2, branchSize);
		branches.insert(std::make_pair(branch->_position, branch));
		current = branch;
		branchSize *= _trunkSizeFactor;
		branchLength *= _branchSizeFactor;
	}
}

bool Tree::grow() {
	if (_doneGrowing) {
		return false;
	}

	// If no attraction points left, we are done
	if (_attractionPoints.empty()) {
		_doneGrowing = true;
		return false;
	}

	// process the attraction points
	for (auto pi = _attractionPoints.begin(); pi != _attractionPoints.end();) {
		bool attractionPointRemoved = false;
		AttractionPoint& attractionPoint = *pi;

		attractionPoint._closestBranch = nullptr;

		// Find the nearest branch for this attraction point
		for (auto bi = _branches.begin(); bi != _branches.end(); ++bi) {
			Branch* branch = bi->second;
			const float length2 = (float) glm::round(glm::distance2(branch->_position, attractionPoint._position));

			// Min attraction point distance reached, we remove it
			if (length2 <= _minDistance2) {
				pi = _attractionPoints.erase(pi);
				attractionPointRemoved = true;
				break;
			} else if (length2 <= _maxDistance2) {
				// branch in range, determine if it is the nearest
				if (attractionPoint._closestBranch == nullptr) {
					attractionPoint._closestBranch = branch;
				} else if (glm::distance2(attractionPoint._closestBranch->_position, attractionPoint._position) > length2) {
					attractionPoint._closestBranch = branch;
				}
			}
		}

		// if the attraction point was removed, skip
		if (attractionPointRemoved) {
			continue;
		}

		++pi;

		// Set the grow parameters on all the closest branches that are in range
		if (attractionPoint._closestBranch == nullptr) {
			continue;
		}
		const glm::vec3& dir = glm::normalize(attractionPoint._position - attractionPoint._closestBranch->_position);
		// add to grow direction of branch
		attractionPoint._closestBranch->_growDirection += dir;
		++attractionPoint._closestBranch->_attractionPointInfluence;
	}

	// Generate the new branches
	std::vector<Branch*> newBranches;
	for (auto& e : _branches) {
		Branch* branch = e.second;
		// if at least one attraction point is affecting the branch
		if (branch->_attractionPointInfluence <= 0) {
			continue;
		}
		const glm::vec3& avgDirection = branch->_growDirection / (float)branch->_attractionPointInfluence;
		const glm::vec3& branchPos = branch->_position + avgDirection * (float)_branchLength;
		Branch *newBranch = new Branch(branch, branchPos, avgDirection, branch->_size * _branchSizeFactor);
		newBranches.push_back(newBranch);
		branch->reset();
	}

	if (newBranches.empty()) {
		_doneGrowing = true;
		return false;
	}

	// Add the new branches to the tree
	bool branchAdded = false;
	for (Branch* branch : newBranches) {
		// Check if branch already exists. These cases seem to
		// happen when attraction point is in specific areas
		auto i = _branches.find(branch->_position);
		if (i == _branches.end()) {
			_branches.insert(std::make_pair(branch->_position, branch));
			branchAdded = true;
		}
	}

	// if no branches were added - we are done
	// this handles issues where attraction points equal out each other,
	// making branches grow without ever reaching the attraction point
	if (!branchAdded) {
		_doneGrowing = true;
		return false;
	}

	return true;
}

}
}
