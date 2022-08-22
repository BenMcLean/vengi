/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"

namespace command {

extern int complete(const io::FilesystemPtr& filesystem, core::String dir, const core::String& match, core::DynamicArray<core::String>& matches, const char* pattern);

inline auto fileCompleter(const io::FilesystemPtr& filesystem, const core::String& lastDirectory, const char* pattern = "*") {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(filesystem, lastDirectory, str, matches, pattern);
	};
}

inline auto fileCompleter(const io::FilesystemPtr& filesystem, const core::VarPtr& lastDirectory, const char* pattern = "*") {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(filesystem, lastDirectory->strVal(), str, matches, pattern);
	};
}

inline auto fileCompleter(const io::FilesystemPtr& filesystem, const core::VarPtr& lastDirectory, const io::FormatDescription* format) {
	const core::String &pattern = io::convertToAllFilePattern(format);
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(filesystem, lastDirectory->strVal(), str, matches, pattern.c_str());
	};
}

}
