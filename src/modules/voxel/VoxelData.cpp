/**
 * @file
 */

#include "VoxelData.h"

namespace voxel {

VoxelData::VoxelData(const voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(new voxel::RawVolume(*v)), palette(new palette::Palette(*p)) {
}

VoxelData::VoxelData(const voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(new voxel::RawVolume(*v)), palette(new palette::Palette(p)) {
}

VoxelData::VoxelData(voxel::RawVolume *v, const palette::Palette *p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(v), palette(new palette::Palette(*p)) {
}

VoxelData::VoxelData(voxel::RawVolume *v, const palette::Palette &p, bool disposeAfterUse)
	: _disposeAfterUse(disposeAfterUse), volume(v), palette(new palette::Palette(p)) {
}

VoxelData::VoxelData(const VoxelData &v)
	: _disposeAfterUse(true), volume(new voxel::RawVolume(*v.volume)), palette(new palette::Palette(*v.palette)) {
}

VoxelData::VoxelData(VoxelData &&v) : _disposeAfterUse(v._disposeAfterUse), volume(v.volume), palette(v.palette) {
	v.volume = nullptr;
	v.palette = nullptr;
}

VoxelData::~VoxelData() {
	if (_disposeAfterUse) {
		delete volume;
	}
	delete palette;
}

bool VoxelData::dispose() const {
	return _disposeAfterUse;
}

VoxelData &VoxelData::operator=(const VoxelData &v) {
	if (this == &v) {
		return *this;
	}
	if (_disposeAfterUse) {
		delete volume;
	}
	delete palette;

	palette = new palette::Palette(*v.palette);
	if (v._disposeAfterUse) {
		volume = new voxel::RawVolume(*v.volume);
	} else {
		volume = v.volume;
	}
	_disposeAfterUse = v._disposeAfterUse;
	return *this;
}

VoxelData &VoxelData::operator=(VoxelData &&v) {
	if (this == &v) {
		return *this;
	}
	if (_disposeAfterUse) {
		delete volume;
	}
	delete palette;
	volume = v.volume;
	palette = v.palette;
	_disposeAfterUse = v._disposeAfterUse;
	v.volume = nullptr;
	v.palette = nullptr;
	return *this;
}

} // namespace voxel
