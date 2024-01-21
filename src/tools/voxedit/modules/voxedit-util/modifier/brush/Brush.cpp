/**
 * @file
 */

#include "Brush.h"

namespace voxedit {

void Brush::reset() {
	markDirty();
}

void Brush::update(const BrushContext &ctx, double nowSeconds) {
}

ModifierType Brush::modifierType(ModifierType type) const {
	return type;
}

/**
 * @brief Determine whether the brush should get rendered
 */
bool Brush::active() const {
	return true;
}

bool Brush::init() {
	return true;
}

void Brush::shutdown() {
}

} // namespace voxedit
