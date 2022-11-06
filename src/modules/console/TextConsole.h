/**
 * @file
 */

#pragma once

#include "util/Console.h"
#include "console/TTY.h"
#include "core/Var.h"

namespace console {

/**
 * Text console for input and output. Supports curses or tty input/output with several modes
 */
class TextConsole : public util::Console {
private:
	using Super = util::Console;
	core::VarPtr _cursesVar;
	console::TTY _input;
	bool _enableCurses = false;
	bool _cursesActive = false;
	int _abortPressCount = -1;

	uv_loop_t _loop;
	void handleTTYInput();
protected:
	void initCurses();
	void shutdownCurses();

	void drawString(int x, int y, const glm::ivec4& color, int, const char* str, int len) override;
	int lineHeight() override;
	glm::ivec2 stringSize(const char* s, int length) override;
	void afterRender(const math::Rect<int> &rect) override;
	void beforeRender(const math::Rect<int> &rect) override;
public:
	TextConsole();
	virtual ~TextConsole();

	bool init() override;
	void construct() override;
	void update(double deltaFrameSeconds) override;
	void shutdown() override;
};

}
