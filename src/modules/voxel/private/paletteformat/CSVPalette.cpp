/**
 * @file
 */

#include "CSVPalette.h"
#include "core/Log.h"

namespace voxel {

bool CSVPalette::load(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette) {
	char line[2048];
	int colorCount = 0;

	while (stream.readLine(sizeof(line), line)) {
		int r, g, b;
		if (SDL_sscanf(line, "%i, %i, %i", &r, &g, &b) != 3) {
			Log::error("Failed to parse line '%s'", line);
			continue;
		}
		if (colorCount >= PaletteMaxColors) {
			Log::warn("Not all colors were loaded");
			break;
		}
		palette.setColor(colorCount, core::RGBA(r, g, b));
		++colorCount;
	}
	palette.setSize(colorCount);
	return colorCount > 0;
}

bool CSVPalette::save(const voxel::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	for (size_t i = 0; i < palette.size(); ++i) {
		const core::RGBA &color = palette.color(i);
		if (!stream.writeStringFormat(false, "%i, %i, %i\n", color.r, color.g, color.b)) {
			Log::error("Failed to write color line");
			return false;
		}
	}
	return true;
}

} // namespace voxel
