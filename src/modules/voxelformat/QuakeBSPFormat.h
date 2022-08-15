/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "image/Image.h"

namespace voxelformat {

/**
 * @brief Quake bsp format
 *
 * @ingroup Formats
 */
class QuakeBSPFormat : public MeshFormat {
private:
	// ----------------------------------------------
	// internal bsp (ufoai) structures
	struct BspLump {
		uint32_t offset = 0;
		uint32_t len = 0;
	};
	static_assert(sizeof(BspLump) == 8, "Unexpected size of BspLump");

	struct BspHeader {
		uint32_t magic = 0;
		uint32_t version = 0;
		BspLump lumps[30];
	};

	struct BspTexture {
		glm::vec4 st[2] {}; // st - xyz+offset
		uint32_t surfaceFlags = 0;
		uint32_t value = 0;
		char name[32] = "";
	};
	static_assert(sizeof(BspTexture) == 72, "Unexpected size of BspTexture");

	struct BspModel {
		glm::vec3 mins {0.0f};
		glm::vec3 maxs {0.0f};
		glm::vec3 position {0.0f};
		int32_t node = 0;
		int32_t faceId = 0;
		int32_t faceCount = 0;
	};
	static_assert(sizeof(BspModel) == 48, "Unexpected size of BspModel");

	using BspVertex = glm::vec3;
	static_assert(sizeof(BspVertex) == 12, "Unexpected size of BspVertex");

	struct BspFace {
		uint16_t planeId = 0;
		int16_t side = 0;

		int32_t edgeId = 0;
		int16_t edgeCount = 0;
		int16_t textureId = 0;

		int32_t lightofsDay = 0;
		int32_t lightofsNight = 0;
	};
	static_assert(sizeof(BspFace) == 20, "Unexpected size of BspFace");

	struct BspEdge {
		int16_t vertexIndices[2] {0, 0}; // negative means counter clock wise
	};
	static_assert(sizeof(BspEdge) == 4, "Unexpected size of BspEdge");

	// ----------------------------------------------
	// structures used for loading the relevant parts

	struct Face {
		int32_t edgeId = 0;
		int16_t edgeCount = 0;
		int16_t textureId = 0;
		int32_t index = -1;
	};

	struct Texture : public BspTexture {
		image::ImagePtr image;
	};

	bool voxelize(const core::DynamicArray<Texture> &textures, const core::DynamicArray<Face> &faces,
				  const core::DynamicArray<BspEdge> &edges, const core::DynamicArray<int32_t> &surfEdges,
				  const core::DynamicArray<BspVertex> &vertices, SceneGraph &sceneGraph);

	int32_t validateLump(const BspLump &lump, size_t elementSize) const;
	bool loadUFOAlienInvasionBsp(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, const BspHeader &header);
	bool loadUFOAlienInvasionTextures(const core::String &filename, io::SeekableReadStream& stream, const BspHeader &header, core::DynamicArray<Texture> &textures, core::StringMap<image::ImagePtr> &textureMap);
	bool loadUFOAlienInvasionFaces(io::SeekableReadStream& stream, const BspHeader &header, core::DynamicArray<Face> &faces);
	bool loadUFOAlienInvasionEdges(io::SeekableReadStream& stream, const BspHeader &header, core::DynamicArray<BspEdge> &edges, core::DynamicArray<int32_t> &surfEdges);
	bool loadUFOAlienInvasionVertices(io::SeekableReadStream& stream, const BspHeader &header, core::DynamicArray<BspVertex> &vertices);

public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) override;
	bool saveMeshes(const core::Map<int, int> &, const SceneGraph &, const Meshes &meshes, const core::String &filename,
					io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad, bool withColor,
					bool withTexCoords) override {
		return false;
	}
};

} // namespace voxelformat
