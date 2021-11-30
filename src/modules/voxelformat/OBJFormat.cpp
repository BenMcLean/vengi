/**
 * @file
 */

#include "OBJFormat.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "voxel/MaterialColor.h"
#include "voxel/VoxelVertex.h"
#include "voxel/Mesh.h"
#include "voxelformat/VoxelVolumes.h"
#include "engine-config.h"

namespace voxel {

void OBJFormat::writeMtlFile(const core::String& mtlName) const {
	const io::FilePtr& file = io::filesystem()->open(mtlName, io::FileMode::SysWrite);
	if (!file->validHandle()) {
		Log::error("Failed to create mtl file at %s", file->name().c_str());
		return;
	}
	io::FileStream stream(file);
	stream.writeStringFormat(false, "# version " PROJECT_VERSION " github.com/mgerhardy/engine\n");
	stream.writeStringFormat(false, "\n");
	stream.writeStringFormat(false, "newmtl palette\n");
	stream.writeStringFormat(false, "Ka 1.000000 1.000000 1.000000\n");
	stream.writeStringFormat(false, "Kd 1.000000 1.000000 1.000000\n");
	stream.writeStringFormat(false, "Ks 0.000000 0.000000 0.000000\n");
	stream.writeStringFormat(false, "Tr 1.000000\n");
	stream.writeStringFormat(false, "illum 1\n");
	stream.writeStringFormat(false, "Ns 0.000000\n");
	stream.writeStringFormat(false, "map_Kd palette-%s.png\n", voxel::getDefaultPaletteName());
}

bool OBJFormat::saveMeshes(const Meshes& meshes, const io::FilePtr &file, float scale, bool quad, bool withColor, bool withTexCoords) {
	io::FileStream stream(file);

	const MaterialColorArray& colors = getMaterialColors();

	// 1 x 256 is the texture format that we are using for our palette
	const float texcoord = 1.0f / (float)colors.size();
	// it is only 1 pixel high - sample the middle
	const float v1 = 0.5f;

	stream.writeStringFormat(false, "# version " PROJECT_VERSION " github.com/mgerhardy/engine\n");
	stream.writeStringFormat(false, "\n");
	stream.writeStringFormat(false, "g Model\n");

	Log::debug("Exporting %i layers", (int)meshes.size());

	int idxOffset = 0;
	int texcoordOffset = 0;
	for (const auto& meshExt : meshes) {
		const voxel::Mesh* mesh = meshExt.mesh;
		Log::debug("Exporting layer %s", meshExt.name.c_str());
		const int nv = mesh->getNoOfVertices();
		const int ni = mesh->getNoOfIndices();
		if (ni % 3 != 0) {
			Log::error("Unexpected indices amount");
			return false;
		}
		const glm::vec3 offset(mesh->getOffset());
		const voxel::VoxelVertex* vertices = mesh->getRawVertexData();
		const voxel::IndexType* indices = mesh->getRawIndexData();
		const char *objectName = meshExt.name.c_str();
		if (objectName[0] == '\0') {
			objectName = "Noname";
		}
		stream.writeStringFormat(false, "o %s\n", objectName);
		stream.writeStringFormat(false, "mtllib palette.mtl\n");
		stream.writeStringFormat(false, "usemtl palette\n");

		for (int i = 0; i < nv; ++i) {
			const voxel::VoxelVertex& v = vertices[i];
			stream.writeStringFormat(false, "v %.04f %.04f %.04f",
					(offset.x + (float)v.position.x) * scale, (offset.y + (float)v.position.y) * scale, (offset.z + (float)v.position.z) * scale);
			if (withColor) {
				const glm::vec4& color = colors[v.colorIndex];
				stream.writeStringFormat(false, " %.03f %.03f %.03f", color.r, color.g, color.b);
			}
			stream.writeStringFormat(false, "\n");
		}

		if (quad) {
			if (withTexCoords) {
				for (int i = 0; i < ni; i += 6) {
					const voxel::VoxelVertex& v = vertices[indices[i]];
					const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
				}
			}

			int uvi = texcoordOffset;
			for (int i = 0; i < ni; i += 6, uvi += 4) {
				const uint32_t one   = idxOffset + indices[i + 0] + 1;
				const uint32_t two   = idxOffset + indices[i + 1] + 1;
				const uint32_t three = idxOffset + indices[i + 2] + 1;
				const uint32_t four  = idxOffset + indices[i + 5] + 1;
				if (withTexCoords) {
					stream.writeStringFormat(false, "f %i/%i %i/%i %i/%i %i/%i\n",
						(int)one, uvi + 1, (int)two, uvi + 2, (int)three, uvi + 3, (int)four, uvi + 4);
				} else {
					stream.writeStringFormat(false, "f %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
				}
			}
			texcoordOffset += ni / 6 * 4;
		} else {
			if (withTexCoords) {
				for (int i = 0; i < ni; i += 3) {
					const voxel::VoxelVertex& v = vertices[indices[i]];
					const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
				}
			}

			for (int i = 0; i < ni; i += 3) {
				const uint32_t one   = idxOffset + indices[i + 0] + 1;
				const uint32_t two   = idxOffset + indices[i + 1] + 1;
				const uint32_t three = idxOffset + indices[i + 2] + 1;
				if (withTexCoords) {
					stream.writeStringFormat(false, "f %i/%i %i/%i %i/%i\n", (int)one,
							texcoordOffset + i + 1, (int)two, texcoordOffset + i + 2, (int)three, texcoordOffset + i + 3);
				} else {
					stream.writeStringFormat(false, "f %i %i %i\n", (int)one, (int)two, (int)three);
				}
			}
			texcoordOffset += ni;

		}
		idxOffset += nv;
	}

	core::String name = file->name();
	core::String mtlname = core::string::stripExtension(name);
	mtlname.append(".mtl");
	writeMtlFile(mtlname);

	return true;
}

}
