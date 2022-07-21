/**
 * @file
 */

#include "KeybindingParser.h"
#include "CustomButtonNames.h"
#include "command/Command.h"
#include "core/ArrayLength.h"
#include "core/Tokenizer.h"
#include "core/collection/DynamicArray.h"
#include "core/Log.h"
#include <SDL.h>

namespace util {

void KeybindingParser::parseKeyAndCommand(core::String key, const core::String& command) {
	int modifier = KMOD_NONE;
	core::TokenizerConfig cfg;
	core::Tokenizer tok(cfg, key, COMMAND_PRESSED);
	const core::DynamicArray<core::String>& line = tok.tokens();
	if (line.size() > 1) {
		for (const core::String& token : line) {
			const core::String& lower = token.toLower();
			if (lower == "shift") {
				modifier |= KMOD_SHIFT;
			} else if (lower == "left_shift") {
				modifier |= KMOD_LSHIFT;
			} else if (lower == "right_shift") {
				modifier |= KMOD_RSHIFT;
			} else if (lower == "alt") {
				modifier |= KMOD_ALT;
			} else if (lower == "left_alt") {
				modifier |= KMOD_LALT;
			} else if (lower == "right_alt") {
				modifier |= KMOD_RALT;
			} else if (lower == "ctrl") {
				modifier |= KMOD_CTRL;
			} else if (lower == "left_ctrl") {
				modifier |= KMOD_LCTRL;
			} else if (lower == "right_ctrl") {
				modifier |= KMOD_RCTRL;
			} else {
				key = token;
				if (key.empty()) {
					key = COMMAND_PRESSED;
				}
			}
		}
	}

	SDL_Keycode keyCode = SDLK_UNKNOWN;
	uint16_t count = 1u;
	for (int i = 0; i < lengthof(button::CUSTOMBUTTONMAPPING); ++i) {
		if (button::CUSTOMBUTTONMAPPING[i].name == key) {
			keyCode = button::CUSTOMBUTTONMAPPING[i].key;
			count = button::CUSTOMBUTTONMAPPING[i].count;
			break;
		}
	}
	if (keyCode == SDLK_UNKNOWN) {
		key = core::string::replaceAll(key, "_", " ");
		keyCode = SDL_GetKeyFromName(key.c_str());
	}
	if (keyCode == SDLK_UNKNOWN) {
		Log::warn("could not get a valid key code for %s (skip binding for %s)", key.c_str(), command.c_str());
		++_invalidBindings;
		return;
	}
	_bindings.insert(std::make_pair(keyCode, CommandModifierPair(command, modifier, count)));
}

KeybindingParser::KeybindingParser(const core::String& key, const core::String& binding) :
		core::Tokenizer(""), _invalidBindings(0) {
	parseKeyAndCommand(key, binding);
}

KeybindingParser::KeybindingParser(const core::String& bindings) :
		core::Tokenizer(bindings), _invalidBindings(0) {
	for (;;) {
		if (!hasNext()) {
			break;
		}
		core::String key = next();
		if (!hasNext()) {
			break;
		}
		const core::String command = next();
		parseKeyAndCommand(key, command);
	}
}

}
