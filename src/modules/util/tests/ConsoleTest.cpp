/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "util/Console.h"
#include "core/Var.h"
#include "command/Command.h"
#include <SDL_log.h>

namespace util {

class ConsoleTest: public app::AbstractTest {
};

class TestConsole : public util::Console {
protected:
	void drawString(ConsoleColor color, const char* str, int len) override {}
};

TEST_F(ConsoleTest, testAutoCompleteCvar) {
	const core::String cvar1 = "abcdef_console";
	const core::String cvar2 = "test";
	const core::String cvarComplete = cvar1 + cvar2;
	core::Var::get(cvarComplete, "1");
	TestConsole c;
	SDL_LogSetOutputFunction(nullptr, nullptr);
	ASSERT_EQ(cvar1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cvarComplete + " ", c.commandLine());
}

TEST_F(ConsoleTest, testAutoCompleteCommand) {
	const core::String cmd1 = "abcdef_console";
	const core::String cmd2 = "test";
	const core::String cmdComplete = cmd1 + cmd2;
	command::Command::registerCommand(cmdComplete.c_str(), [] (const command::CmdArgs& args) {});
	TestConsole c;
	SDL_LogSetOutputFunction(nullptr, nullptr);
	ASSERT_EQ(cmd1, c.commandLine());
	c.autoComplete();
	ASSERT_EQ(cmdComplete + " ", c.commandLine());
}

}
