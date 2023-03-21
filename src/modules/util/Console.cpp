/**
 * @file
 */

#include "Console.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include "command/Command.h"
#include "core/collection/Set.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Tokenizer.h"
#include "command/CommandHandler.h"
#include "VarUtil.h"
#include "util/CustomButtonNames.h"
#include <SDL.h>

namespace util {

namespace {

static const glm::ivec4 colors[MAX_COLORS] = {
	glm::ivec4(255, 255, 255, 255),
	glm::ivec4(0, 0, 0, 255),
	glm::ivec4(127, 127, 127, 255),
	glm::ivec4(0, 0, 255, 255),
	glm::ivec4(0, 255, 0, 255),
	glm::ivec4(255, 255, 0, 255),
	glm::ivec4(255, 0, 0, 255),
};
static_assert(lengthof(colors) == MAX_COLORS, "Color count doesn't match");

static const ConsoleColor priorityColors[SDL_NUM_LOG_PRIORITIES] = {
	GRAY,
	GRAY,
	GREEN,
	WHITE,
	YELLOW,
	RED,
	RED
};
static_assert(lengthof(priorityColors) == SDL_NUM_LOG_PRIORITIES, "Priority count doesn't match");
}

Console::Console() :
		_mainThread(SDL_ThreadID()) {
}

Console::~Console() {
}

core::String Console::getColor(ConsoleColor color) {
	core_assert(color >= 0 && color <= (int)SDL_arraysize(colors));
	core::String s;
	s += _colorMark;
	s += core::string::toString((int)color);
	return s;
}

bool Console::isColor(const char *cstr) {
	static const char maxColor = MAX_COLORS + '0';
	return cstr[0] == _colorMark && cstr[1] >= '0' && cstr[1] <= maxColor;
}

void Console::skipColor(const char **cstr) {
	static_assert((int)MAX_COLORS < 10, "max colors must not exceed one ascii char for encoding");
	*cstr += 2;
}

void Console::construct() {
	SDL_LogGetOutputFunction((SDL_LogOutputFunction*)&_logFunction, &_logUserData);
	SDL_LogSetOutputFunction((SDL_LogOutputFunction)logConsole, this);
	command::Command::registerCommand("toggleconsole", [&] (const command::CmdArgs& args) { toggle(); }).setHelp("Toggle the built-in console");
	command::Command::registerCommand("clear", [&] (const command::CmdArgs& args) { clear(); }).setHelp("Clear the text from the built-in console");
	command::Command::registerCommand("history", [&] (const command::CmdArgs& args) { printHistory(); }).setHelp("Print the command history");
}

bool Console::init() {
	const io::FilesystemPtr& fs = io::filesystem();
	const core::String& content = fs->load("%s", _historyFilename);
	core::string::splitString(content, _history, "\n");
	_historyPos = _history.size();
	Log::debug("Loaded %i history entries", _historyPos);

	return true;
}

void Console::shutdown() {
	core::String content;
	for (const core::String& s : _history) {
		content += s;
		content += '\n';
	}

	const io::FilesystemPtr& fs = io::filesystem();
	if (!fs->write(_historyFilename, content)) {
		Log::warn("Failed to write the history");
	} else {
		Log::debug("Wrote the history");
	}
	clear();

	command::Command::unregisterCommand("toggleconsole");
	command::Command::unregisterCommand("clear");
	command::Command::unregisterCommand("history");
	SDL_LogSetOutputFunction((SDL_LogOutputFunction)_logFunction, _logUserData);
}

void Console::printHistory() {
	for (const core::String& s : _history) {
		Log::info("%s", s.c_str());
	}
}

bool Console::onKeyPress(int32_t key, int16_t modifier) {
	if (!_consoleActive) {
		return false;
	}

	if (modifier & KMOD_ALT) {
		if (key == SDLK_BACKSPACE) {
			cursorDeleteWord();
		} else if (key == SDLK_LEFT) {
			cursorWordLeft();
		} else if (key == SDLK_RIGHT) {
			cursorWordRight();
		}
		return true;
	}

	if (modifier & KMOD_CONTROL) {
		if (key == SDLK_TAB || key == SDLK_d) {
			toggle();
		} else if (key == SDLK_a || key == SDLK_b) {
			_cursorPos = 0;
		} else if (key == SDLK_e) {
			_cursorPos = (int)_commandLine.size();
		} else if (key == SDLK_c) {
			_messages.push_back(_consolePrompt + _commandLine);
			clearCommandLine();
		} else if (key == SDLK_l) {
			clear();
		} else if (key == SDLK_w) {
			cursorDeleteWord();
		} else if (key == SDLK_v) {
			insertClipboard();
		} else if (key == SDLK_LEFT) {
			cursorWordLeft();
		} else if (key == SDLK_RIGHT) {
			cursorWordRight();
		} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
			++_fontSize;
		} else if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
			--_fontSize;
		}
		return true;
	}

	if (modifier & KMOD_SHIFT) {
		if (key == SDLK_HOME) {
			_scrollPos = (int)_messages.size() - _maxLines + 1;
		} else if (key == SDLK_END) {
			_scrollPos = 0;
		} else if (key == SDLK_PAGEUP) {
			scrollPageUp();
		} else if (key == SDLK_PAGEDOWN) {
			scrollPageDown();
		}
		return true;
	}

	switch (key) {
	case SDLK_ESCAPE:
		toggle();
		return true;
	case SDLK_HOME:
		_cursorPos = 0;
		return true;
	case SDLK_END:
		_cursorPos = (int)_commandLine.size();
		return true;
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		executeCommandLine();
		return true;
	case SDLK_BACKSPACE:
		cursorDelete();
		return true;
	case SDLK_DELETE:
		cursorDelete(false);
		return true;
	case SDLK_INSERT:
		_overwrite ^= true;
		return true;
	case SDLK_LEFT:
		cursorLeft();
		return true;
	case SDLK_RIGHT:
		cursorRight();
		return true;
	case SDLK_UP:
		cursorUp();
		return true;
	case SDLK_DOWN:
		cursorDown();
		return true;
	case SDLK_PAGEUP:
		scrollPageUp();
		return true;
	case SDLK_PAGEDOWN:
		scrollPageDown();
		return true;
	case SDLK_TAB:
		autoComplete();
		return true;
	}

	return true;
}

void Console::executeCommandLine() {
	_messages.push_back(_consolePrompt + _commandLine);
	_scrollPos = 0;
	if (_commandLine.empty()) {
		return;
	}
	_history.push_back(_commandLine);
	_historyPos = _history.size();

	command::executeCommands(_commandLine);
	clearCommandLine();
}

bool Console::onMouseButtonPress(int32_t x, int32_t y, uint8_t button) {
	if (!_consoleActive) {
		return false;
	}

	if (button != SDL_BUTTON_MIDDLE) {
		return false;
	}

	return insertClipboard();
}

bool Console::insertClipboard() {
	if (!SDL_HasClipboardText()) {
		return false;
	}

	char *str = SDL_GetClipboardText();
	if (str == nullptr) {
		return false;
	}

	insertText(str);
	SDL_free(str);
	return true;
}

bool Console::onMouseWheel(int32_t x, int32_t y) {
	if (!_consoleActive) {
		return false;
	}

	if (y > 0) {
		scrollUp();
	} else {
		scrollDown();
	}

	return true;
}

void Console::insertText(const core::String& text) {
	if (text.empty()) {
		return;
	}
	const SDL_Keymod state = SDL_GetModState();
	if (state & (KMOD_CONTROL | KMOD_ALT)) {
		return;
	}
	if (_overwrite && _cursorPos < int(_commandLine.size())) {
		cursorDelete();
	}
	_commandLine.insert(_cursorPos, text.c_str());
	_cursorPos += (int)text.size();
}

bool Console::onTextInput(const core::String& text) {
	if (!_consoleActive) {
		return false;
	}

	insertText(text);

	return true;
}

void Console::cursorLeft() {
	if (_cursorPos > 0) {
		_cursorPos--;
	}
}

void Console::cursorRight() {
	const int size = (int)_commandLine.size();
	if (_cursorPos < size) {
		_cursorPos++;
	}
}

void Console::cursorWordLeft() {
	auto prevWordEnd = _commandLine.find_last_of(" ", core_max(0, _cursorPos - 1));
	if (core::String::npos == prevWordEnd) {
		_cursorPos = 0;
		return;
	}
	_cursorPos = (int)prevWordEnd;
}

void Console::cursorWordRight() {
	const int spaceOffset = _commandLine[_cursorPos] == ' ' ? 1 : 0;
	const core::String& partialCommandLine = _commandLine.substr(_cursorPos + spaceOffset);
	const size_t nextWordEnd = partialCommandLine.find_first_of(" ");
	if (core::String::npos == nextWordEnd) {
		_cursorPos = (int)_commandLine.size();
		return;
	}
	_cursorPos = core_min(_commandLine.size(), _cursorPos + nextWordEnd + spaceOffset);
}

void Console::cursorUp() {
	if (_historyPos <= 0) {
		return;
	}

	--_historyPos;
	_commandLine = _history[_historyPos];
	_cursorPos = (int)_commandLine.size();
}

void Console::cursorDown() {
	++_historyPos;

	const size_t entries = _history.size();
	if (_historyPos >= entries) {
		_historyPos = entries;
		clearCommandLine();
		return;
	}
	_commandLine = _history[_historyPos];
	_cursorPos = (int)_commandLine.size();
}

void Console::scrollUp(const int lines) {
	const int scrollableLines = (int)_messages.size() - _maxLines;
	if (scrollableLines <= 0) {
		return;
	}
	if (_scrollPos <= scrollableLines) {
		_scrollPos += core_min(lines, scrollableLines - _scrollPos + 1);
	}
}

void Console::scrollDown(const int lines) {
	if (_scrollPos <= 0) {
		return;
	}
	_scrollPos = core_max(_scrollPos - lines, 0);
}

void Console::scrollPageUp() {
	// scroll one page minus one line minus prompt
	scrollUp(_maxLines - 2);
}


void Console::scrollPageDown() {
	// scroll one page minus one line minus prompt
	scrollDown(_maxLines - 2);
}

void Console::autoComplete() {
	// TODO: handle the cursor position properly
	core::DynamicArray<core::String> matches;
	const core::DynamicArray<core::String> allCommands = core::Tokenizer(_commandLine, ";").tokens();
	const core::String& lastCmd = allCommands.empty() ? "" : allCommands.back();
	const core::DynamicArray<core::String> strings = core::Tokenizer(lastCmd, " ").tokens();
	core::String baseSearchString = "";
	bool parameter = _commandLine[_cursorPos] == ' ' || strings.size() > 1;
	if (parameter) {
		const command::Command* cmd = command::Command::getCommand(strings.front());
		if (cmd != nullptr) {
			if (strings.back() == strings.front()) {
				cmd->complete("", matches);
			} else {
				cmd->complete(strings.back(), matches);
			}
		}
	} else {
		// try to complete the already existing command
		baseSearchString = strings.empty() ? "" : strings.back();
		command::Command::visitSorted([&] (const command::Command& cmd) {
			if (strings.empty() || strings.size() == 1) {
				// match the command name itself
				if (core::string::matches(cmd.name(), baseSearchString + "*")) {
					matches.push_back(cmd.name());
				}
			} else {
				// match parameters for the command
				cmd.complete(strings.back(), matches);
			}
		});
		if (!strings.empty()) {
			// try to complete the last string as it can be used as a parameter to the command
			baseSearchString = strings.back();
		}
		baseSearchString += '*';
		util::visitVarSorted([&] (const core::VarPtr& var) {
			if (core::string::matches(var->name(), baseSearchString)) {
				matches.push_back(var->name());
			}
		}, 0u);
	}

	if (matches.empty()) {
		return;
	}

	core::Set<core::String, 11, core::StringHash> uniqueMatches;
	uniqueMatches.insert(matches.begin(), matches.end());
	matches.clear();
	for (auto i = uniqueMatches.begin(); i != uniqueMatches.end(); ++i) {
		matches.push_back(i->key);
	}
	matches.sort(core::Greater<core::String>());

	if (matches.size() == 1) {
		if (strings.size() <= 1) {
			_commandLine = matches.front() + " ";
		} else {
			const size_t cmdLineSize = _commandLine.size();
			const size_t cmdEraseIndex = cmdLineSize - strings.back().size();
			_commandLine.erase(cmdEraseIndex, strings.back().size());
			_commandLine.insert(cmdEraseIndex, matches.front().c_str());
		}
	} else {
		_messages.push_back(_consolePrompt + _commandLine);
		int pos = 0;
		const core::String first = matches.front();
		for (char c : first) {
			bool allMatch = true;
			for (const core::String& match : matches) {
				if (match[pos] != c) {
					allMatch = false;
					break;
				}
			}
			if (!allMatch) {
				break;
			}
			++pos;
		}
		if (pos > 0) {
			replaceLastParameter(matches.front().substr(0, pos));
		}
		for (const core::String& match : matches) {
			Log::info("%s", match.c_str());
		}
	}
	_cursorPos = (int)_commandLine.size();
}

void Console::replaceLastParameter(const core::String& param) {
	auto iter = _commandLine.rfind(' ');
	if (iter == core::String::npos) {
		_commandLine = param;
		return;
	}

	_commandLine.erase(iter + 1);
	_commandLine.append(param.c_str());
}

void Console::cursorDelete(bool moveCursor) {
	if (_commandLine.empty()) {
		return;
	}
	if (moveCursor) {
		if (_cursorPos <= 0) {
			return;
		}
		cursorLeft();
	}
	_commandLine.erase(_cursorPos, 1);
}

void Console::cursorDeleteWord() {
	if (_commandLine.empty()) {
		return;
	}
	if (0 >= _cursorPos) {
		return;
	}
	const int spaceOffset = _commandLine[_cursorPos - 1] == ' ' ? 1 : 0;
	const size_t prevWordStart = _commandLine.find_last_of(" ", _cursorPos - spaceOffset - 1);
	if (core::String::npos == prevWordStart) {
		_commandLine.erase(0, _cursorPos);
		_cursorPos = 0;
		return;
	}
	_commandLine.erase(prevWordStart + 1, _cursorPos - prevWordStart - 1);
	_cursorPos = (int)prevWordStart + 1;
}

core::String Console::removeAnsiColors(const char* message) {
	core::String out;
	out.reserve(SDL_strlen(message) + 1);
	for (const char *c = message; *c != '\0'; ++c) {
		// https://en.wikipedia.org/wiki/ANSI_escape_code
		if (*c >= 030 && *c < 037 && *(c + 1) == '[') {
			c += 2;
			while (*c != 'm' && *c != '\0') {
				++c;
			}
			continue;
		}
		out += *c;
	}
	return out;
}

void Console::logConsole(void *userdata, int category, int priority, const char *message) {
	if (priority < 0 || priority >= SDL_NUM_LOG_PRIORITIES) {
		return;
	}
	if (priority < SDL_LogGetPriority(category)) {
		return;
	}
	Console* console = (Console*)userdata;
	if (SDL_ThreadID() != console->_mainThread) {
		core_assert(message);
		console->_messageQueue.emplace(category, priority, message);
		return;
	}
	console->addLogLine(category, priority, message);
}

void Console::addLogLine(int category, int priority, const char *message) {
	const core::String& cleaned = removeAnsiColors(message);
	const bool hasColor = isColor(cleaned.c_str());
	if (hasColor) {
		_messages.emplace_back(cleaned);
		skipColor(&message);
	} else {
		const core::String& color = getColor(priorityColors[priority]);
		_messages.emplace_back(color + cleaned);
	}
	if (_useOriginalLogFunction) {
		((SDL_LogOutputFunction)_logFunction)(_logUserData, category, (SDL_LogPriority)priority, message);
	}
}

bool Console::toggle() {
	_consoleActive ^= true;
	return _consoleActive;
}

void Console::update(double /*deltaFrameSeconds*/) {
	core_assert(_mainThread == SDL_ThreadID());
	LogLine msg;
	while (_messageQueue.pop(msg)) {
		core_assert(msg.message);
		addLogLine(msg.category, msg.priority, msg.message);
	}
}

void Console::clear() {
	clearCommandLine();
	_messages.clear();
	_scrollPos = 0;
}

inline void Console::clearCommandLine() {
	_cursorPos = 0;
	_commandLine.clear();
}

void Console::drawString(int x, int y, const core::String& str, int len) {
	const char *cstr = str.c_str();
	glm::ivec4 color = colors[WHITE];
	int colorIndex = -1;
	if (isColor(cstr)) {
		skipColor(&cstr);
		len -= 2;
		colorIndex = str[1] - '0';
		if (colorIndex >= 0 && colorIndex < (int)SDL_arraysize(colors)) {
			color = colors[colorIndex];
		}
	}
	drawString(x, y, &color[0], colorIndex, cstr, len);
}

void Console::render(const math::Rect<int> &rect, double deltaFrameSeconds) {
	_frame += deltaFrameSeconds;
	if (_frame > 0.25) {
		_frame = 0.0;
		_cursorBlink ^= true;
	}

	if (!_consoleActive) {
		return;
	}
	core_trace_scoped(ConsoleRender);

	beforeRender(rect);

	const int lineH = lineHeight();
	_maxLines = (rect.getMaxZ() - rect.getMinZ()) / lineH;
	if (_maxLines <= 0) {
		afterRender(rect);
		return;
	}
	const int maxY = (int)_messages.size() * lineH;
	const glm::ivec2& commandLineSize = stringSize(_commandLine.c_str(), (int)_commandLine.size());
	const int startY = core_min(rect.getMinZ() + rect.getMaxZ() - commandLineSize.y - 4, maxY);
	if (!_messages.empty()) {
		auto i = _messages.end();
		--i;
		core::prev(i, _scrollPos);
		for (int y = startY; ; --i) {
			if (y < rect.getMinZ()) {
				break;
			}
			const glm::ivec2& size = stringSize(i->c_str(), (int)i->size());
			y -= size.y;
			drawString(_consoleMarginLeft, y, *i, (int)i->size());
			if (i == _messages.begin()) {
				break;
			}
		}
	}

	drawString(_consoleMarginLeft, startY, _consolePrompt, (int)_consolePrompt.size());
	drawString(_consoleMarginLeft + _consoleMarginLeftBehindPrompt, startY, _commandLine, (int)_commandLine.size());
	if (_cursorBlink) {
		const glm::ivec2& l = stringSize(_commandLine.c_str(), _cursorPos);
		drawString(_consoleMarginLeft + _consoleMarginLeftBehindPrompt + l.x, startY, _consoleCursor, (int)_consoleCursor.size());
	}

	afterRender(rect);
}

}
