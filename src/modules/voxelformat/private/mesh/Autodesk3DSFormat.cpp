/**
 * @file
 */

#include "Autodesk3DSFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/TexturedTri.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Failed to read 3ds " CORE_STRINGIFY(read));                                                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Failed to read 3ds " CORE_STRINGIFY(read));                                                        \
		return false;                                                                                                  \
	}

namespace priv {

enum ChunkIds {
	CHUNK_ID_VERSION = 0x0002,

	CHUNK_ID_NODE = 0x3D3D,
	CHUNK_ID_NODE_PIVOT = 0xB013,
	CHUNK_ID_NODE_SCALE = 0x0100,
	CHUNK_ID_NODE_ID = 0xB030,
	CHUNK_ID_NODE_VERSION = 0x3D3E,
	CHUNK_ID_NODE_MATRIX = 0x4160,
	CHUNK_ID_NODE_ROTATION = 0xB021,
	CHUNK_ID_NODE_BOUNDING_BOX = 0xB014,
	CHUNK_ID_NODE_CHILD = 0x4000,
	CHUNK_ID_NODE_HEADER = 0xB010,
	CHUNK_ID_NODE_INSTANCE_NAME = 0xB011,
	CHUNK_ID_NODE_MATERIAL = 0xAFFF,
	CHUNK_ID_NODE_OBJECT_MESH = 0x4100,

	CHUNK_ID_MESH_VERTICES = 0x4110,
	CHUNK_ID_MESH_FACES = 0x4120,
	CHUNK_ID_MESH_NORMALS = 0x4152,
	CHUNK_ID_MESH_UV = 0x4140,

	CHUNK_ID_MATERIAL_NAME = 0xA000,
	CHUNK_ID_MATERIAL_AMBIENT = 0xA010,
	CHUNK_ID_MATERIAL_DIFFUSE = 0xA020,
	CHUNK_ID_MATERIAL_SPECULAR = 0xA030,
	CHUNK_ID_MATERIAL_SHININESS = 0xA040,
	CHUNK_ID_MATERIAL_SHININESS_2 = 0xA041,
	CHUNK_ID_MATERIAL_TRANSPARENCY = 0xA050,
	CHUNK_ID_MATERIAL_FALLTHROUGH = 0xA052,
	CHUNK_ID_MATERIAL_FALLIN = 0xA08A,

	CHUNK_ID_MATERIAL_BLUR = 0xA053,
	CHUNK_ID_MATERIAL_TWO_SIDED = 0xA081,
	CHUNK_ID_MATERIAL_DIFFUSE_TEXTURE = 0xA200,
	CHUNK_ID_MATERIAL_SPECULAR_TEXTURE = 0xA204,
	CHUNK_ID_MATERIAL_OPACITY_TEXTURE = 0xA210,
	CHUNK_ID_MATERIAL_REFLECTION_TEXTURE = 0xA220,
	CHUNK_ID_MATERIAL_BUMP_TEXTURE = 0xA230,
	CHUNK_ID_MATERIAL_SELF_ILLUMINATION = 0xA084,
	CHUNK_ID_MATERIAL_WIREFRAME_SIZE = 0xA087,
	CHUNK_ID_MATERIAL_SHADING = 0xA100,

	CHUNK_ID_TEXTURE_MAP_NAME = 0xA300,
	CHUNK_ID_TEXTURE_MAP_TILING = 0xA351,
	CHUNK_ID_TEXTURE_MAP_TEXBLUR = 0xA353,
	CHUNK_ID_TEXTURE_MAP_USCALE = 0xA354,
	CHUNK_ID_TEXTURE_MAP_VSCALE = 0xA356,
	CHUNK_ID_TEXTURE_MAP_UOFFSET = 0xA358,
	CHUNK_ID_TEXTURE_MAP_VOFFSET = 0xA35A,
	CHUNK_ID_KEYFRAME_HEADER = 0xB00A,

	CHUNK_ID_FACE_MATERIAL_GROUP = 0x4130,
	CHUNK_ID_FACE_SMOOTH_GROUP = 0x4150,

	CHUNK_ID_KEYFRAMES = 0xB000,

	// generic data types
	CHUNK_ID_DATA_COLOR_FLOAT = 0x0010,
	CHUNK_ID_DATA_COLOR_BYTE = 0x0011,
	CHUNK_ID_DATA_LINEAR_COLOR_BYTE = 0x0012,
	CHUNK_ID_DATA_LINEAR_COLOR_FLOAT = 0x0013,
	CHUNK_ID_DATA_PERCENT = 0x0030,
	CHUNK_ID_DATA_PERCENT_FLOAT = 0x0031,
};

#define TO_STR(x)                                                                                                      \
	{ x, CORE_STRINGIFY(x) }

static const struct {
	uint16_t chunkId;
	const char *name;
} Names[] = {TO_STR(CHUNK_ID_VERSION),
			 TO_STR(CHUNK_ID_VERSION),
			 TO_STR(CHUNK_ID_NODE_VERSION),
			 TO_STR(CHUNK_ID_NODE_PIVOT),
			 TO_STR(CHUNK_ID_NODE_ID),
			 TO_STR(CHUNK_ID_DATA_COLOR_FLOAT),
			 TO_STR(CHUNK_ID_DATA_COLOR_BYTE),
			 TO_STR(CHUNK_ID_DATA_LINEAR_COLOR_BYTE),
			 TO_STR(CHUNK_ID_DATA_LINEAR_COLOR_FLOAT),
			 TO_STR(CHUNK_ID_NODE_SCALE),
			 TO_STR(CHUNK_ID_DATA_PERCENT),
			 TO_STR(CHUNK_ID_DATA_PERCENT_FLOAT),
			 TO_STR(CHUNK_ID_NODE),
			 TO_STR(CHUNK_ID_NODE_MATERIAL),
			 TO_STR(CHUNK_ID_MATERIAL_SPECULAR_TEXTURE),
			 TO_STR(CHUNK_ID_MATERIAL_OPACITY_TEXTURE),
			 TO_STR(CHUNK_ID_MATERIAL_REFLECTION_TEXTURE),
			 TO_STR(CHUNK_ID_MATERIAL_BUMP_TEXTURE),
			 TO_STR(CHUNK_ID_NODE_CHILD),
			 TO_STR(CHUNK_ID_MESH_VERTICES),
			 TO_STR(CHUNK_ID_MESH_FACES),
			 TO_STR(CHUNK_ID_MATERIAL_NAME),
			 TO_STR(CHUNK_ID_MATERIAL_AMBIENT),
			 TO_STR(CHUNK_ID_MATERIAL_DIFFUSE),
			 TO_STR(CHUNK_ID_MATERIAL_SPECULAR),
			 TO_STR(CHUNK_ID_MATERIAL_SHININESS),
			 TO_STR(CHUNK_ID_MATERIAL_SHININESS_2),
			 TO_STR(CHUNK_ID_MATERIAL_TRANSPARENCY),
			 TO_STR(CHUNK_ID_MATERIAL_FALLTHROUGH),
			 TO_STR(CHUNK_ID_MATERIAL_FALLIN),
			 TO_STR(CHUNK_ID_MATERIAL_BLUR),
			 TO_STR(CHUNK_ID_MATERIAL_TWO_SIDED),
			 TO_STR(CHUNK_ID_NODE_OBJECT_MESH),
			 TO_STR(CHUNK_ID_MATERIAL_DIFFUSE_TEXTURE),
			 TO_STR(CHUNK_ID_TEXTURE_MAP_NAME),
			 TO_STR(CHUNK_ID_FACE_MATERIAL_GROUP),
			 TO_STR(CHUNK_ID_MESH_UV),
			 TO_STR(CHUNK_ID_TEXTURE_MAP_UOFFSET),
			 TO_STR(CHUNK_ID_TEXTURE_MAP_VOFFSET),
			 TO_STR(CHUNK_ID_NODE_MATRIX),
			 TO_STR(CHUNK_ID_TEXTURE_MAP_TILING),
			 TO_STR(CHUNK_ID_TEXTURE_MAP_TEXBLUR),
			 TO_STR(CHUNK_ID_TEXTURE_MAP_USCALE),
			 TO_STR(CHUNK_ID_TEXTURE_MAP_VSCALE),
			 TO_STR(CHUNK_ID_KEYFRAME_HEADER),
			 TO_STR(CHUNK_ID_NODE_HEADER),
			 TO_STR(CHUNK_ID_NODE_INSTANCE_NAME),
			 TO_STR(CHUNK_ID_NODE_BOUNDING_BOX),
			 TO_STR(CHUNK_ID_NODE_ROTATION),
			 TO_STR(CHUNK_ID_KEYFRAMES),
			 TO_STR(CHUNK_ID_MATERIAL_SHADING),
			 TO_STR(CHUNK_ID_MATERIAL_SELF_ILLUMINATION),
			 TO_STR(CHUNK_ID_MATERIAL_WIREFRAME_SIZE),
			 TO_STR(CHUNK_ID_FACE_SMOOTH_GROUP),
			 TO_STR(CHUNK_ID_MESH_NORMALS)};

static const char *chunkToString(uint16_t chunkId) {
	for (const auto &name : Names) {
		if (name.chunkId == chunkId) {
			return name.name;
		}
	}
	return "Unknown";
}

} // namespace priv

Autodesk3DSFormat::ScopedChunk::ScopedChunk(io::SeekableReadStream *stream) : _stream(stream) {
	_chunkPos = stream->pos();
	stream->readUInt16(chunk.id);
	stream->readUInt32(chunk.length);
	Log::debug("Found chunk %s with size %d", priv::chunkToString(chunk.id), chunk.length);
}

Autodesk3DSFormat::ScopedChunk::~ScopedChunk() {
	int64_t expectedPos = _chunkPos + chunk.length;
	if (_stream->pos() != expectedPos) {
		Log::error("3ds chunk %s has unexpected size of %i - expected was %i", priv::chunkToString(chunk.id),
				   (int)(_stream->pos() - _chunkPos), (int)chunk.length);
		_stream->seek(expectedPos);
	}
}

bool Autodesk3DSFormat::readMeshFaces(io::SeekableReadStream *stream, Chunk3ds &parent, Mesh3ds &mesh) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	uint16_t faceCount;
	wrap(stream->readUInt16(faceCount))
	mesh.faces.resize(faceCount);
	for (uint16_t i = 0; i < faceCount; ++i) {
		Face3ds &face = mesh.faces[i];
		wrap(stream->readUInt16(face.indices.x))
		wrap(stream->readUInt16(face.indices.y))
		wrap(stream->readUInt16(face.indices.z))
		wrap(stream->readUInt16(face.flags))
	}

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_FACE_SMOOTH_GROUP: {
			uint16_t count = mesh.faces.size();
			for (uint16_t i = 0; i < count; ++i) {
				int32_t group;
				wrap(stream->readInt32(group))
				mesh.faces[i].smoothingGroup = group;
			}
			break;
		}
		case priv::CHUNK_ID_FACE_MATERIAL_GROUP: {
			core::String material;
			wrapBool(stream->readString(64, material, true))
			uint16_t count;
			wrap(stream->readUInt16(count))
			Log::debug("count: %d", count);
			for (uint16_t i = 0; i < count; ++i) {
				uint16_t faceIndex;
				wrap(stream->readUInt16(faceIndex))
				if (faceIndex >= mesh.faces.size()) {
					Log::error("Invalid face index %d/%d", faceIndex, (int)mesh.faces.size());
					return false;
				}
				mesh.faces[faceIndex].material = material;
			}
			break;
		}
		// TODO: face normals? 0x4154
		default:
			skipUnknown(stream, scoped.chunk, "Face");
			break;
		}
	}
	return true;
}

bool Autodesk3DSFormat::readMesh(io::SeekableReadStream *stream, Chunk3ds &parent, Mesh3ds &mesh) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_MESH_VERTICES: {
			uint16_t count;
			wrap(stream->readUInt16(count))
			mesh.vertices.resize(count);
			for (uint16_t i = 0; i < count; ++i) {
				wrap(stream->readFloat(mesh.vertices[i].x))
				wrap(stream->readFloat(mesh.vertices[i].y))
				wrap(stream->readFloat(mesh.vertices[i].z))
			}
			break;
		}
		case priv::CHUNK_ID_MESH_FACES: {
			wrapBool(readMeshFaces(stream, scoped.chunk, mesh))
			break;
		}
		case priv::CHUNK_ID_MESH_UV: {
			uint16_t count;
			wrap(stream->readUInt16(count))
			mesh.texcoords.resize(count);
			for (uint16_t i = 0; i < count; ++i) {
				wrap(stream->readFloat(mesh.texcoords[i].x))
				wrap(stream->readFloat(mesh.texcoords[i].y))
			}
			break;
		}
		case priv::CHUNK_ID_MESH_NORMALS: {
			uint16_t count;
			wrap(stream->readUInt16(count))
			mesh.normals.resize(count);
			for (uint16_t i = 0; i < count; ++i) {
				wrap(stream->readFloat(mesh.normals[i].x))
				wrap(stream->readFloat(mesh.normals[i].y))
				wrap(stream->readFloat(mesh.normals[i].z))
			}
			break;
		}
		case priv::CHUNK_ID_NODE_MATRIX: {
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 3; ++j) {
					wrap(stream->readFloat(mesh.matrix[i][j]))
				}
			}
			break;
		}
		default:
			skipUnknown(stream, scoped.chunk, "Mesh");
			break;
		}
	}
	return true;
}

bool Autodesk3DSFormat::readDataColor(io::SeekableReadStream *stream, Chunk3ds &parent, core::RGBA &color) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_DATA_COLOR_BYTE:
		case priv::CHUNK_ID_DATA_LINEAR_COLOR_BYTE: {
			uint8_t r, g, b;
			wrap(stream->readUInt8(r))
			wrap(stream->readUInt8(g))
			wrap(stream->readUInt8(b))
			color.r = r;
			color.g = g;
			color.b = b;
			color.a = 255;
			break;
		}
		default:
			skipUnknown(stream, scoped.chunk, "Color");
			break;
		}
	}
	return true;
}

bool Autodesk3DSFormat::readDataFactor(io::SeekableReadStream *stream, Chunk3ds &parent, float &factor) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_DATA_PERCENT: {
			int16_t percent;
			wrap(stream->readInt16(percent))
			factor = (float)percent / 100.0f;
			Log::debug("factor: %d", percent);
			break;
		}
		case priv::CHUNK_ID_DATA_PERCENT_FLOAT: {
			wrap(stream->readFloat(factor))
			Log::debug("factor: %f", factor);
			break;
		}
		default:
			skipUnknown(stream, scoped.chunk, "Factor");
			break;
		}
	}
	return true;
}

bool Autodesk3DSFormat::readMaterialTexture(const io::ArchivePtr &archive, io::SeekableReadStream *stream,
											Chunk3ds &parent, MaterialTexture3ds &texture) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_DATA_PERCENT:
			stream->skip(2);
			break;
		case priv::CHUNK_ID_TEXTURE_MAP_NAME: {
			wrapBool(stream->readString(64, texture.name, true))
			Log::debug("texture name: %s", texture.name.c_str());
			texture.name = lookupTexture("", texture.name);
			texture.texture = image::loadImage(texture.name);
			if (!texture.texture || !texture.texture->isLoaded()) {
				Log::warn("Failed to load texture %s", texture.name.c_str());
			}
			break;
		}
		case priv::CHUNK_ID_TEXTURE_MAP_TILING: {
			wrap(stream->readInt16(texture.tiling))
			Log::debug("tiling: %d", texture.tiling);
			break;
		}
		case priv::CHUNK_ID_TEXTURE_MAP_TEXBLUR: {
			wrap(stream->readFloat(texture.blur))
			Log::debug("blur: %f", texture.blur);
			break;
		}
		case priv::CHUNK_ID_TEXTURE_MAP_USCALE: {
			wrap(stream->readFloat(texture.scaleU))
			Log::debug("Texture map scale u: %f", texture.scaleU);
			break;
		}
		case priv::CHUNK_ID_TEXTURE_MAP_VSCALE: {
			wrap(stream->readFloat(texture.scaleV))
			Log::debug("Texture map scale v: %f", texture.scaleV);
			break;
		}
		case priv::CHUNK_ID_TEXTURE_MAP_UOFFSET: {
			wrap(stream->readFloat(texture.offsetU))
			Log::debug("Texture map offset u: %f", texture.offsetU);
			break;
		}
		case priv::CHUNK_ID_TEXTURE_MAP_VOFFSET: {
			wrap(stream->readFloat(texture.offsetV))
			Log::debug("Texture map offset v: %f", texture.offsetV);
			break;
		}
		default:
			skipUnknown(stream, scoped.chunk, "Texture");
			break;
		}
	}
	return true;
}

bool Autodesk3DSFormat::readMaterial(const io::ArchivePtr &archive, io::SeekableReadStream *stream, Chunk3ds &parent,
									 Material3ds &material) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_MATERIAL_NAME: {
			wrapBool(stream->readString(64, material.name, true))
			Log::debug("material name: %s", material.name.c_str());
			break;
		}
		case priv::CHUNK_ID_MATERIAL_DIFFUSE: {
			wrapBool(readDataColor(stream, scoped.chunk, material.diffuseColor))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_AMBIENT: {
			wrapBool(readDataColor(stream, scoped.chunk, material.ambientColor))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_SPECULAR: {
			wrapBool(readDataColor(stream, scoped.chunk, material.specularColor))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_SHININESS: {
			wrapBool(readDataFactor(stream, scoped.chunk, material.shininess))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_SHININESS_2: {
			wrapBool(readDataFactor(stream, scoped.chunk, material.shininess2))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_TRANSPARENCY: {
			wrapBool(readDataFactor(stream, scoped.chunk, material.transparency))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_SELF_ILLUMINATION:
			// fallthrough
		case priv::CHUNK_ID_MATERIAL_FALLIN:
			// fallthrough
		case priv::CHUNK_ID_MATERIAL_FALLTHROUGH: {
			float value;
			wrapBool(readDataFactor(stream, scoped.chunk, value))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_WIREFRAME_SIZE: {
			float value;
			wrap(stream->readFloat(value))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_BLUR: {
			wrapBool(readDataFactor(stream, scoped.chunk, material.blur))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_DIFFUSE_TEXTURE: {
			wrapBool(readMaterialTexture(archive, stream, scoped.chunk, material.diffuse))
			break;
		}
		case priv::CHUNK_ID_MATERIAL_SHADING: {
			uint16_t shading;
			wrap(stream->readUInt16(shading))
			Log::debug("shading: %d", shading);
			break;
		}
		default:
			skipUnknown(stream, scoped.chunk, "Material");
			break;
		}
	}
	return true;
}

bool Autodesk3DSFormat::readNodeChildren(io::SeekableReadStream *stream, Chunk3ds &parent, Node3ds &node) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	core::String name;
	wrapBool(stream->readString(64, name, true))

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_NODE_OBJECT_MESH: {
			Mesh3ds mesh;
			mesh.name = name;
			wrapBool(readMesh(stream, scoped.chunk, mesh))
			node.meshes.emplace_back(mesh);
			break;
		}
		// TODO: camera
		default:
			skipUnknown(stream, scoped.chunk, "Child");
			break;
		}
	}
	return true;
}

bool Autodesk3DSFormat::readNode(const io::ArchivePtr &archive, io::SeekableReadStream *stream, Chunk3ds &parent,
								 Node3ds &node) const {
	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + parent.length - 6;

	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);
		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_NODE_HEADER: {
			wrapBool(stream->readString(64, node.name, true))
			Log::debug("node name: %s", node.name.c_str());
			wrap(stream->readUInt16(node.flags1))
			wrap(stream->readUInt16(node.flags2))
			wrap(stream->readInt16(node.parentId))
			break;
		}
		case priv::CHUNK_ID_NODE_VERSION: {
			wrap(stream->readUInt32(node.meshVersion))
			Log::debug("node version: %d", node.meshVersion);
			break;
		}
		case priv::CHUNK_ID_NODE_SCALE: {
			wrap(stream->readFloat(node.scale))
			Log::debug("scale: %f", node.scale);
			break;
		}
		case priv::CHUNK_ID_NODE_MATERIAL: {
			Material3ds material;
			wrapBool(readMaterial(archive, stream, scoped.chunk, material))
			if (material.name.empty()) {
				Log::error("Material without name");
				return false;
			}
			Log::debug("Add material with name: '%s'", material.name.c_str());
			if (node.materials.find(material.name) != node.materials.end()) {
				Log::error("Material with name '%s' already exists", material.name.c_str());
				return false;
			}
			node.materials.put(material.name, material);
			break;
		}
		case priv::CHUNK_ID_NODE_PIVOT: {
			wrap(stream->readFloat(node.pivot.x))
			wrap(stream->readFloat(node.pivot.y))
			wrap(stream->readFloat(node.pivot.z))
			Log::debug("pivot: %f %f %f", node.pivot.x, node.pivot.y, node.pivot.z);
			break;
		}
		case priv::CHUNK_ID_NODE_ID: {
			wrap(stream->readInt16(node.id))
			Log::debug("node id: %d", node.id);
			break;
		}
		case priv::CHUNK_ID_NODE_CHILD: {
			wrapBool(readNodeChildren(stream, scoped.chunk, node))
			break;
		}
		case priv::CHUNK_ID_NODE_INSTANCE_NAME: {
			wrapBool(stream->readString(64, node.instanceName, true))
			Log::debug("instance name: %s", node.instanceName.c_str());
			break;
		}
		case priv::CHUNK_ID_NODE_BOUNDING_BOX: {
			wrap(stream->readFloat(node.min.x))
			wrap(stream->readFloat(node.min.y))
			wrap(stream->readFloat(node.min.z))
			wrap(stream->readFloat(node.max.x))
			wrap(stream->readFloat(node.max.y))
			wrap(stream->readFloat(node.max.z))
			Log::debug("bounding box: min(%f %f %f), max(%f %f %f)", node.min.x, node.min.y, node.min.z, node.max.x,
					   node.max.y, node.max.z);
			break;
		}
#if 0
		case priv::CHUNK_ID_NODE_ROTATION: {
			Log::debug("rotation");
			uint16_t flags;
			wrap(stream->readUInt16(flags))
			stream->skip(8);
			uint16_t keys;
			wrap(stream->readUInt16(keys))
			for (uint16_t i = 0; i < keys; ++i) {
				uint16_t frame;
				wrap(stream->readUInt16(frame))
				stream->skip(4);
				float x, y, z, rot;
				wrap(stream->readFloat(rot))
				wrap(stream->readFloat(x))
				wrap(stream->readFloat(y))
				wrap(stream->readFloat(z))
			}
			break;
	}
#endif
		default:
			skipUnknown(stream, scoped.chunk, "Node");
			break;
		}
	}
	return true;
}

void Autodesk3DSFormat::skipUnknown(io::SeekableReadStream *stream, const Chunk3ds &chunk, const char *section) const {
	Log::debug("%s: Unknown 3ds chunk 0X%04x (%s) of size %d", section, chunk.id, priv::chunkToString(chunk.id),
			   chunk.length);
	stream->skip(chunk.length - 6);
}

bool Autodesk3DSFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
									   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	uint16_t magic;
	wrap(stream->readUInt16(magic))
	if (magic != 0x4D4D) { // this is not a real magic byte - but the id for the main chunk
		Log::error("Invalid 3ds magic");
		return false;
	}

	uint32_t numberOfBytes;
	wrap(stream->readUInt32(numberOfBytes))

	const int64_t currentPos = stream->pos();
	const int64_t endOfChunk = currentPos + numberOfBytes - 6;

	core::DynamicArray<Node3ds> nodes;
	while (stream->pos() < endOfChunk) {
		ScopedChunk scoped(stream);

		switch (scoped.chunk.id) {
		case priv::CHUNK_ID_VERSION: {
			uint32_t version;
			wrap(stream->readUInt32(version))
			Log::debug("version: %d", version);
			break;
		}
		case priv::CHUNK_ID_NODE: {
			Node3ds node;
			wrapBool(readNode(archive, stream, scoped.chunk, node))
			nodes.emplace_back(node);
			break;
		}
		default:
			skipUnknown(stream, scoped.chunk, "Main");
			break;
		}
	}

	// 3dsmax is using z-up axis - so let's correct this
	const glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	const glm::vec3 scale = getInputScale();
	Log::debug("Import %i nodes", (int)nodes.size());
	for (const Node3ds &node : nodes) {
		Log::debug("Import %i meshes for node %s", (int)node.meshes.size(), node.name.c_str());
		for (const Mesh3ds &mesh : node.meshes) {
			TriCollection tris;
			for (const Face3ds &face : mesh.faces) {
				TexturedTri tri;
				for (int i = 0; i < 3; ++i) {
					tri.vertices[i] = rotationMatrix * glm::vec4(mesh.vertices[face.indices[i]] * scale, 1.0f);
				}
				tri.uv[0] = mesh.texcoords[face.indices[0]];
				tri.uv[1] = mesh.texcoords[face.indices[1]];
				tri.uv[2] = mesh.texcoords[face.indices[2]];
				auto matIter = node.materials.find(face.material);
				if (matIter == node.materials.end()) {
					Log::error("Material '%s' not found (%i total)", face.material.c_str(), (int)node.materials.size());
					for (const auto &mat : node.materials) {
						Log::error("Material: '%s'", mat->first.c_str());
					}
					return false;
				} else {
					Material3ds &material = matIter->value;
					tri.color[0] = tri.color[1] = tri.color[2] = material.diffuseColor;
					tri.texture = material.diffuse.texture;
				}
				tris.emplace_back(tri);
			}
			// TODO: FORMAT: node parent
			// TODO: MATERIAL use the material from the 3ds file
			Log::debug("Mesh %s has %i tris", mesh.name.c_str(), (int)tris.size());
			const int nodeId = voxelizeNode(mesh.name, sceneGraph, tris);
			if (nodeId == InvalidNodeId) {
				return false;
			}
		}
	}

	return true;
}

#undef wrapBool
#undef wrap

} // namespace voxelformat