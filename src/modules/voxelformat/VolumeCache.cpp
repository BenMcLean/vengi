/**
 * @file
 */

#include "VolumeCache.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/Format.h"
#include "io/Filesystem.h"
#include "app/App.h"
#include "command/Command.h"
#include "core/Log.h"

namespace voxelformat {

VolumeCache::~VolumeCache() {
	core_assert_msg(_volumes.empty(), "VolumeCache wasn't shut down properly");
}

voxel::RawVolume* VolumeCache::loadVolume(const core::String &filename) {
	{
		core::ScopedLock lock(_mutex);
		auto i = _volumes.find(filename);
		if (i != _volumes.end()) {
			return i->second;
		}
	}
	Log::debug("Loading volume from %s", filename.c_str());
	const io::FilesystemPtr& fs = io::filesystem();

	io::FilePtr file;
	for (const char **ext = SUPPORTED_VOXEL_FORMATS_LOAD_LIST; *ext; ++ext) {
		file = fs->open(core::string::format("%s.%s", filename.c_str(), *ext));
		if (file->exists()) {
			break;
		}
	}
	if (!file->exists()) {
		Log::debug("Failed to load %s for any of the supported format extensions", filename.c_str());
		return nullptr;
	}
	voxel::VoxelVolumes volumes;
	if (!voxelformat::loadVolumeFormat(file, volumes)) {
		Log::error("Failed to load %s", file->name().c_str());
		voxelformat::clearVolumes(volumes);
		core::ScopedLock lock(_mutex);
		_volumes.put(filename, nullptr);
		return nullptr;
	}
	voxel::RawVolume* v = volumes.merge();
	voxelformat::clearVolumes(volumes);

	core::ScopedLock lock(_mutex);
	_volumes.put(filename, v);
	return v;
}

bool VolumeCache::removeVolume(const char* fullPath) {
	const core::String filename = fullPath;
	core::ScopedLock lock(_mutex);
	auto i = _volumes.find(filename);
	if (i != _volumes.end()) {
		_volumes.erase(i);
		return true;
	}
	return false;
}

bool VolumeCache::deleteVolume(const char* fullPath) {
	const core::String filename = fullPath;
	core::ScopedLock lock(_mutex);
	auto i = _volumes.find(filename);
	if (i != _volumes.end()) {
		delete i->value;
		_volumes.erase(i);
		return true;
	}
	return false;
}

void VolumeCache::construct() {
	command::Command::registerCommand("volumecachelist", [&] (const command::CmdArgs& argv) {
		Log::info("Cache content");
		core::ScopedLock lock(_mutex);
		for (const auto& e : _volumes) {
			Log::info(" * %s", e->key.c_str());
		}
	});
	command::Command::registerCommand("volumecacheclear", [&] (const command::CmdArgs& argv) {
		core::ScopedLock lock(_mutex);
		for (const auto & e : _volumes) {
			delete e->value;
		}
		_volumes.clear();
	});
}

bool VolumeCache::init() {
	return true;
}

void VolumeCache::shutdown() {
	core::ScopedLock lock(_mutex);
	for (const auto & e : _volumes) {
		delete e->value;
	}
	_volumes.clear();
}

}
