/**
 * @file
 */

#include "VolumeCache.h"
#include "animation/chr/CharacterSkeleton.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/StringUtil.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/Format.h"

namespace voxedit {
namespace anim {

bool VolumeCache::load(const core::String& fullPath, size_t volumeIndex, voxel::VoxelVolumes& volumes) {
	Log::info("Loading volume from %s into the cache", fullPath.c_str());
	const io::FilesystemPtr& fs = io::filesystem();

	io::FilePtr file;
	for (const char **ext = voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD_LIST; *ext; ++ext) {
		file = fs->open(core::string::format("%s.%s", fullPath.c_str(), *ext));
		if (file->exists()) {
			break;
		}
	}
	if (!file->exists()) {
		Log::error("Failed to load %s for any of the supported format extensions", fullPath.c_str());
		return false;
	}
	voxel::VoxelVolumes localVolumes;
	// TODO: use the cache luke
	io::FileStream stream(file.get());
	if (!voxelformat::loadFormat(file->name(), stream, localVolumes)) {
		Log::error("Failed to load %s", file->name().c_str());
		return false;
	}
	if ((int)localVolumes.size() != 1) {
		localVolumes.clear();
		Log::error("More than one volume/layer found in %s", file->name().c_str());
		return false;
	}
	volumes[volumeIndex] = core::move(localVolumes[0]);
	return true;
}

bool VolumeCache::getVolumes(const animation::AnimationSettings& settings, voxel::VoxelVolumes& volumes) {
	volumes.resize(animation::AnimationSettings::MAX_ENTRIES);

	for (size_t i = 0; i < animation::AnimationSettings::MAX_ENTRIES; ++i) {
		if (settings.paths[i].empty()) {
			continue;
		}
		const core::String& fullPath = settings.fullPath(i);
		if (!load(fullPath, i, volumes)) {
			Log::error("Failed to load %s", settings.paths[i].c_str());
			return false;
		}
	}
	for (int i = 0; i < (int)animation::AnimationSettings::MAX_ENTRIES; ++i) {
		if (volumes[i].volume() == nullptr) {
			continue;
		}
		volumes[i].setName(settings.meshType(i));
	}
	return true;
}

}
}
