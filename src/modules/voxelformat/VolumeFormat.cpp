/**
 * @file
 */

#include "VolumeFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "voxelformat/PLYFormat.h"
#include "voxelformat/VoxFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"
#include "voxelformat/VXLFormat.h"
#include "voxelformat/CubFormat.h"
#include "voxelformat/BinVoxFormat.h"
#include "voxelformat/KVXFormat.h"
#include "voxelformat/KV6Format.h"
#include "voxelformat/AoSVXLFormat.h"
#include "voxelformat/CSMFormat.h"
#include "voxelformat/OBJFormat.h"

namespace voxelformat {

// this is the list of supported voxel volume formats that are have importers implemented
const io::FormatDescription SUPPORTED_VOXEL_FORMATS_LOAD[] = {
	{"MagicaVoxel", "vox"},
	{"Qubicle", "qbt"},
	{"Qubicle", "qb"},
	{"Sandbox VoxEdit", "vxm"},
	{"Sandbox VoxEdit", "vxr"},
	{"BinVox", "binvox"},
	{"CubeWorld", "cub"},
	{"Build engine", "kvx"},
	{"Ace of Spades", "kv6"},
	{"Tiberian Sun", "vxl"},
	{"Qubicle", "qef"},
	{"Chronovox", "csm"},
	{"Nicks Voxel Model", "nvm"},
	{nullptr, nullptr}
};
// this is the list of internal formats that are supported engine-wide (the format we save our own models in)
const char *SUPPORTED_VOXEL_FORMATS_LOAD_LIST[] = { "qb", "vox", nullptr };
// this is the list of supported voxel or mesh formats that have exporters implemented
const io::FormatDescription SUPPORTED_VOXEL_FORMATS_SAVE[] = {
	{"MagicaVoxel", "vox"},
	{"Qubicle", "qbt"},
	{"Qubicle", "qb"},
	{"Sandbox VoxEdit", "vxm"},
	{"BinVox", "binvox"},
	{"CubeWorld", "cub"},
	{"Build engine", "kvx"},
	{"Tiberian Sun", "vxl"},
	{"Qubicle", "qef"},
	{"WaveFront OBJ", "obj"},
	{"Polygon File Format", "ply"},
	{nullptr, nullptr}
};

static uint32_t loadMagic(const io::FilePtr& file) {
	io::FileStream stream(file.get());
	uint32_t magicWord = 0u;
	stream.readInt(magicWord);
	return magicWord;
}

bool loadVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& newVolumes) {
	if (!filePtr->exists()) {
		Log::error("Failed to load model file %s. Doesn't exist.", filePtr->name().c_str());
		return false;
	}

	const uint32_t magic = loadMagic(filePtr);

	core_trace_scoped(LoadVolumeFormat);
	const core::String& ext = filePtr->extension();
	if (ext == "qb") {
		voxel::QBFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vox" || magic == FourCC('V','O','X',' ')) {
		voxel::VoxFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "qbt" || magic == FourCC('Q','B',' ','2')) {
		voxel::QBTFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "kvx") {
		voxel::KVXFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "kv6" || magic == FourCC('K','v','x','l')) {
		voxel::KV6Format f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "cub") {
		voxel::CubFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vxm" || magic == FourCC('V','X','M','A')
			|| magic == FourCC('V','X','M','9') || magic == FourCC('V','X','M','8')
			|| magic == FourCC('V','X','M','7') || magic == FourCC('V','X','M','6')
			|| magic == FourCC('V','X','M','5') || magic == FourCC('V','X','M','4')) {
		voxel::VXMFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vxr"
			|| magic == FourCC('V','X','R','7') || magic == FourCC('V','X','R','6')
			|| magic == FourCC('V','X','R','5') || magic == FourCC('V','X','R','4')
			|| magic == FourCC('V','X','R','3') || magic == FourCC('V','X','R','2')
			|| magic == FourCC('V','X','R','1')) {
		voxel::VXRFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vxl" && magic == FourCC('V','o','x','e')) {
		voxel::VXLFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "vxl") {
		voxel::AoSVXLFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "csm" || magic == FourCC('.','C','S','M')
			|| ext == "nvm" || magic == FourCC('.','N','V','M')) {
		voxel::CSMFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "binvox" || magic == FourCC('#','b','i','n')) {
		voxel::BinVoxFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else if (ext == "qef" || magic == FourCC('Q','u','b','i')) {
		voxel::QEFFormat f;
		if (!f.loadGroups(filePtr, newVolumes)) {
			voxelformat::clearVolumes(newVolumes);
		}
	} else {
		Log::error("Failed to load model file %s - unsupported file format for extension '%s'",
				filePtr->name().c_str(), ext.c_str());
		return false;
	}
	if (newVolumes.empty()) {
		Log::error("Failed to load model file %s. Broken file.", filePtr->name().c_str());
		return false;
	}
	Log::info("Load model file %s with %i layers", filePtr->name().c_str(), (int)newVolumes.size());
	return true;
}

bool saveFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes) {
	if (isMeshFormat(filePtr->name())) {
		return saveMeshFormat(filePtr, volumes);
	}
	return saveVolumeFormat(filePtr, volumes);
}

bool saveVolumeFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes) {
	if (volumes.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filePtr->name().c_str());
		return false;
	}

	const core::String& ext = filePtr->extension();
	if (ext == "qb") {
		voxel::QBFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "qbt") {
		voxel::QBTFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "qef") {
		voxel::QEFFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "cub") {
		voxel::CubFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "vxl") {
		voxel::VXLFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "binvox") {
		voxel::BinVoxFormat f;
		return f.saveGroups(volumes, filePtr);
	}
	Log::warn("Failed to save file with unknown type: %s - saving as qb instead", ext.c_str());
	voxel::QBFormat f;
	return f.saveGroups(volumes, filePtr);
}

bool saveMeshFormat(const io::FilePtr& filePtr, voxel::VoxelVolumes& volumes) {
	if (volumes.empty()) {
		Log::error("Failed to save model file %s - no volumes given", filePtr->name().c_str());
		return false;
	}

	const core::String& ext = filePtr->extension();
	if (ext == "obj") {
		voxel::OBJFormat f;
		return f.saveGroups(volumes, filePtr);
	} else if (ext == "ply") {
		voxel::PLYFormat f;
		return f.saveGroups(volumes, filePtr);
	}
	Log::error("Failed to save model file %s - unknown extension '%s' given", filePtr->name().c_str(), ext.c_str());
	return false;
}


bool isMeshFormat(const core::String& filename) {
	const core::String& ext = core::string::extractExtension(filename);
	if (ext == "obj" || ext == "ply") {
		return true;
	}
	return false;
}

void clearVolumes(voxel::VoxelVolumes& volumes) {
	for (auto& v : volumes) {
		delete v.volume;
	}
	volumes.volumes.clear();
}

}
