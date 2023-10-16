/**
 * @file
 */

#include "App.h"
#include <SDL_cpuinfo.h>
#include "app/AppCommand.h"
#include "core/Var.h"
#include "core/concurrent/ThreadPool.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "io/Filesystem.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Tokenizer.h"
#include "core/concurrent/Concurrency.h"
#include "util/VarUtil.h"
#include <SDL.h>
#include "engine-config.h"
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <signal.h>
#include <cfenv>

// osx delayed loading of a NSDocument derived file type
static core::String g_loadingDocument;
extern "C" void setLoadingDocument(const char *path) {
	g_loadingDocument = path;
}
const core::String &loadingDocument() {
	return g_loadingDocument;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#if defined(__WIN32__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__LINUX__) || defined(__MACOSX__) || defined(__EMSCRIPTEN__)
#include <sys/utsname.h>
#endif

namespace app {

#ifdef __EMSCRIPTEN__
void App::runFrameEmscripten() {
	if (AppState::InvalidAppState == _staticInstance->_curState) {
		emscripten_cancel_main_loop();
		return;
	}
	_staticInstance->onFrame();
}
#endif

static void catch_function(int signo) {
	core_stacktrace();
	abort();
}

static void graceful_shutdown(int signo) {
	App::getInstance()->requestQuit();
}

static void loop_debug_log(int signo) {
	const core::VarPtr &log = core::Var::getSafe(cfg::CoreLogLevel);
	int current = log->intVal();
	current--;
	if (current < SDL_LOG_PRIORITY_VERBOSE)
		current = SDL_LOG_PRIORITY_CRITICAL;
	log->setVal(current);
	Log::init();
}

App* App::_staticInstance;

App* App::getInstance() {
	core_assert(_staticInstance != nullptr);
	return _staticInstance;
}

App::App(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		_filesystem(filesystem), _threadPool(core::make_shared<core::ThreadPool>(threadPoolSize, "Core")),
		_timeProvider(timeProvider) {
#ifdef FE_TONEAREST
	std::fesetround(FE_TONEAREST);
#endif

#if defined(__WIN32__)
	_osName = "Windows";
#elif defined(__MACOSX__)
	_osName = "MacOSX";
#elif defined(__LINUX__)
	_osName = "Linux";
#elif defined(__EMSCRIPTEN__)
	_osName = "Emscripten";
#endif

#if defined(__WIN32__)
	OSVERSIONINFOA osInfo;
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	::GetVersionExA(&osInfo);
	_osVersion = core::string::format("%i.%i.%i", (int)osInfo.dwMajorVersion, (int)osInfo.dwMinorVersion, (int)osInfo.dwBuildNumber);
#elif defined(__LINUX__) || defined(__MACOSX__) || defined(__EMSCRIPTEN__)
	struct utsname details;
	if (uname(&details) == 0) {
		_osVersion = core::string::format("%s %s", details.release, details.version);
	}
#endif

	if  (_osName.empty()) {
		_osName = "unknown";
	}
	if  (_osVersion.empty()) {
		_osVersion = "undetected";
	}

	core_assert_init();
	signal(SIGSEGV, catch_function);
	_initialLogLevel = SDL_LOG_PRIORITY_INFO;
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, (SDL_LogPriority)_initialLogLevel);
	_timeProvider->updateTickTime();
	_staticInstance = this;
	signal(SIGINT, graceful_shutdown);
	signal(42, loop_debug_log);
}

App::~App() {
	Log::shutdown();
	_threadPool = core::ThreadPoolPtr();
}

void App::init(const core::String& organisation, const core::String& appname) {
	_organisation = organisation;
	_appname = appname;
}

void App::setArgs(int argc, char *argv[]) {
	_argc = argc;
	_argv = argv;
}

int App::startMainLoop(int argc, char *argv[]) {
	setArgs(argc, argv);
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(runFrameEmscripten, 0, 1);
#else
	while (AppState::InvalidAppState != _curState) {
		onFrame();
	}
#endif
	return _exitCode;
}

void App::addBlocker(AppState blockedState) {
	_blockers[(int)blockedState] = true;
}

void App::remBlocker(AppState blockedState) {
	_blockers[(int)blockedState] = false;
}

void App::onFrame() {
	core_trace_begin_frame("Main");
	if (_nextState != AppState::InvalidAppState && _nextState != _curState) {
		if (_blockers[(int)_nextState]) {
			if (AppState::Blocked != _curState) {
				_curState = AppState::Blocked;
			}
		} else {
			_curState = _nextState;
			_nextState = AppState::InvalidAppState;
		}
	}

	_timeProvider->updateTickTime();
	if (AppState::Blocked == _curState) {
		SDL_Delay(1);
		_deltaFrameSeconds = 0.001;
	} else {
		const double now = _timeProvider->tickSeconds();
		_deltaFrameSeconds = now - _nowSeconds;
		_nowSeconds = now;

		switch (_curState) {
		case AppState::Construct: {
			core_trace_scoped(AppOnConstruct);
			_nextState = onConstruct();
			Log::debug("AppState::Construct done");
			break;
		}
		case AppState::Init: {
			core_trace_scoped(AppOnInit);
			Log::debug("AppState::BeforeInit");
			onBeforeInit();
			Log::debug("AppState::Init");
			_nextState = onInit();
			Log::debug("AppState::AfterInit");
			onAfterInit();
			Log::debug("AppState::Init done");
			_nextFrameSeconds = now;
			break;
		}
		case AppState::InitFailure: {
			core_trace_scoped(AppOnCleanup);
			if (_exitCode == 0) {
				_exitCode = 1;
			}
			_nextState = onCleanup();
			Log::debug("AppState::InitFailure done");
			break;
		}
		case AppState::Running: {
			core_trace_scoped(AppOnRunning);
			{
				core_trace_scoped(AppOnBeforeRunning);
				onBeforeRunning();
			}
			const AppState state = onRunning();
			if (_nextState != AppState::Cleanup && _nextState != AppState::Destroy) {
				_nextState = state;
			}
			if (AppState::Running == _nextState) {
				core_trace_scoped(AppOnAfterRunning);
				onAfterRunning();
			}
			const double framesPerSecondsCap = _framesPerSecondsCap->floatVal();
			if (framesPerSecondsCap >= 1.0) {
				if (_nextFrameSeconds > now) {
					const double delay = _nextFrameSeconds - now;
					_nextFrameSeconds = now + 1.0 / framesPerSecondsCap;
					if (delay > 0.0) {
						const uint32_t milliDelay = (uint32_t)(delay * 1000.0);
						SDL_Delay(milliDelay);
					}
				} else {
					_nextFrameSeconds = now + 1.0 / framesPerSecondsCap;
				}
			}
			break;
		}
		case AppState::Cleanup: {
			core_trace_scoped(AppOnCleanup);
			_nextState = onCleanup();
			Log::debug("AppState::Cleanup done");
			break;
		}
		case AppState::Destroy: {
			core_trace_scoped(AppOnDestroy);
			_nextState = onDestroy();
			_curState = AppState::InvalidAppState;
			Log::debug("AppState::Destroy done");
			break;
		}
		default:
			break;
		}
	}
	onAfterFrame();
	core_trace_end_frame("Main");
}

AppState App::onConstruct() {
	core::VarPtr logVar = core::Var::get(cfg::CoreLogLevel, _initialLogLevel);
	logVar->setHelp("The lower the value, the more you see. 1 is the highest log level, 5 is just fatal errors.");
	// this ensures that we are sleeping 1 millisecond if there is enough room for it
	_framesPerSecondsCap = core::Var::get(cfg::CoreMaxFPS, "1000.0");
	registerArg("--loglevel").setShort("-l").setDescription("Change log level from 1 (trace) to 6 (only critical)");
	const core::String& logLevelVal = getArgVal("--loglevel");
	if (!logLevelVal.empty()) {
		logVar->setVal(logLevelVal);
	}
	core::Var::get(cfg::CoreSysLog, _syslog ? "true" : "false", "Log to the system log", core::Var::boolValidator);

	Log::init();

	command::Command::registerCommand("set", [] (const command::CmdArgs& args) {
		if (args.size() < 2) {
			Log::info("usage: set <name> <value>");
			return;
		}
		core::Var::get(args[0], "")->setVal(core::string::join(args.begin() + 1, args.end(), " "));
	}).setHelp("Set a variable value");

	command::Command::registerCommand("quit", [&] (const command::CmdArgs& args) {requestQuit();}).setHelp("Quit the application");

#ifdef DEBUG
	command::Command::registerCommand("assert", [&] (const command::CmdArgs& args) {
		core_assert_msg(false, "assert triggered");
	}).setHelp("Trigger an assert");
#endif

	AppCommand::init(_timeProvider);

	for (int i = 0; i < _argc; ++i) {
		if (_argv[i][0] != '-' || (_argv[i][0] != '\0' && _argv[i][1] == '-')) {
			continue;
		}

		const core::String command = &_argv[i][1];
		if (command != "set") {
			continue;
		}
		if (i + 2 < _argc) {
			core::String var = _argv[i + 1];
			const char *value = _argv[i + 2];
			i += 2;
			core::Var::get(var, value, core::CV_FROMCOMMANDLINE);
			Log::debug("Set %s to %s", var.c_str(), value);
		}
	}

	Log::init();

	Log::debug("%s: " PROJECT_VERSION, _appname.c_str());

	for (int i = 0; i < _argc; ++i) {
		Log::debug("argv[%i] = %s", i, _argv[i]);
	}

	if (_coredump) {
#ifdef HAVE_SYS_RESOURCE_H
		struct rlimit core_limits;
		core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &core_limits);
		Log::debug("activate core dumps");
#else
		Log::debug("can't activate core dumps");
#endif
	}

	if (!_filesystem->init(_organisation, _appname)) {
		Log::warn("Failed to initialize the filesystem");
	}

	const io::FilesystemPtr& fs = io::filesystem();
	const core::String& logfilePath = fs->writePath("log.txt");
	Log::init(logfilePath.c_str());

	return AppState::Init;
}

void App::onBeforeInit() {
}

AppState App::onInit() {
	Log::debug("Initialize sdl");
	SDL_Init(SDL_INIT_TIMER|SDL_INIT_EVENTS);
	Log::debug("Initialize the threadpool");
	_threadPool->init();

	Log::debug("Initialize the cvars");
	const io::FilePtr& varsFile = _filesystem->open(_appname + ".vars");
	const core::String &content = varsFile->load();
	core::Tokenizer t(content);
	while (t.hasNext()) {
		const core::String& name = t.next();
		if (name.empty()) {
			Log::warn("%s contains invalid configuration name", varsFile->name().c_str());
			break;
		}
		if (!t.hasNext()) {
			Log::warn("%s contains invalid configuration value for %s", varsFile->name().c_str(), name.c_str());
			break;
		}
		const core::String& value = t.next();
		if (!t.hasNext()) {
			break;
		}
		const core::String& flags = t.next();
		uint32_t flagsMaskFromFile = core::CV_FROMFILE;
		for (char c : flags) {
			if (c == 'R') {
				flagsMaskFromFile |= core::CV_READONLY;
				Log::debug("read only flag for %s", name.c_str());
			} else if (c == 'S') {
				flagsMaskFromFile |= core::CV_SHADER;
				Log::debug("shader flag for %s", name.c_str());
			} else if (c == 'X') {
				flagsMaskFromFile |= core::CV_SECRET;
				Log::debug("secret flag for %s", name.c_str());
			}
		}
		const core::VarPtr& old = core::Var::get(name);
		int32_t flagsMask;
		if (old) {
			flagsMask = (int32_t)(flagsMaskFromFile | old->getFlags());
		} else {
			flagsMask = (int32_t)(flagsMaskFromFile);
		}

		flagsMask &= ~(core::CV_FROMCOMMANDLINE | core::CV_FROMENV);

		core::Var::get(name, value.c_str(), flagsMask);
	}

	Log::debug("Initialize the log system");
	Log::init();
	_logLevelVar = core::Var::getSafe(cfg::CoreLogLevel);
	_syslogVar = core::Var::getSafe(cfg::CoreSysLog);

	core::Var::needsSaving();
	core::Var::visit([&] (const core::VarPtr& var) {
		var->markClean();
	});

	for (int i = 0; i < _argc; ++i) {
		if (SDL_strcmp(_argv[i], "--help") == 0 || SDL_strcmp(_argv[i], "-h") == 0) {
			usage();
			return AppState::Destroy;
		}
		if (SDL_strcmp(_argv[i], "--version") == 0 || SDL_strcmp(_argv[i], "-v") == 0) {
			Log::info("%s " PROJECT_VERSION, _appname.c_str());
			return AppState::Destroy;
		}
	}

	_availableMemoryMiB = SDL_GetSystemRAM();

	core_trace_init();

	return AppState::Running;
}

void App::onAfterInit() {
	Log::debug("handle %i command line arguments", _argc);
	for (int i = 0; i < _argc; ++i) {
		// every command is started with a '-'
		if (_argv[i][0] != '-' || (_argv[i][0] != '\0' &&_argv[i][1] == '-')) {
			continue;
		}

		const core::String command = &_argv[i][1];
		if (command == "set") {
			// already handled
			continue;
		}
		if (command::Command::getCommand(command) == nullptr) {
			continue;
		}
		core::String args;
		args.reserve(256);
		for (++i; i < _argc;) {
			if (_argv[i][0] == '-') {
				--i;
				break;
			}
			args.append(_argv[i++]);
			args.append(" ");
		}
		Log::debug("Execute %s with %i arguments", command.c_str(), (int)args.size());
		command::executeCommands(command + " " + args);
	}
	const core::String& autoexecCommands = filesystem()->load("autoexec.cfg");
	if (!autoexecCommands.empty()) {
		Log::debug("execute autoexec.cfg");
		command::Command::execute(autoexecCommands);
	} else {
		Log::debug("skip autoexec.cfg");
	}

	const core::String& autoexecAppCommands = filesystem()->load("%s-autoexec.cfg", _appname.c_str());
	if (!autoexecAppCommands.empty()) {
		Log::debug("execute %s-autoexec.cfg", _appname.c_str());
		command::Command::execute(autoexecAppCommands);
	}

	// we might have changed the loglevel from the commandline
	if (_logLevelVar->isDirty() || _syslogVar->isDirty()) {
		Log::init();
		_logLevelVar->markClean();
		_syslogVar->markClean();
	}
}

bool App::hasEnoughMemory(size_t bytes) const {
	if (_availableMemoryMiB <= 0) {
		// assume we have enough memory if the system does not report the available memory
		return true;
	}
	const uint32_t s = 1024 * 1024;
	return _availableMemoryMiB * (size_t)s >= bytes;
}

void App::usage() const {
	const core::VarPtr& logLevel = core::Var::get(cfg::CoreLogLevel, "");
	logLevel->setVal((int)Log::Level::Info);
	Log::init();
	Log::info("Version " PROJECT_VERSION);
	Log::info("Usage: %s [--help] [--version] [-set configvar value] [-commandname] %s", _appname.c_str(), _additionalUsage.c_str());
	Log::info("------------");

	int maxWidthLong = 0;
	int maxWidthShort = 0;
	for (const Argument& a : _arguments) {
		maxWidthLong = core_max(maxWidthLong, (int)a.longArg().size());
		maxWidthShort = core_max(maxWidthShort, (int)a.shortArg().size());
	};
	int maxWidthOnlyLong = maxWidthLong + maxWidthShort + 3;
	for (const Argument& a : _arguments) {
		const core::String defaultVal = a.defaultValue().empty() ? "" : core::string::format(" (default: %s)", a.defaultValue().c_str());
		if (a.shortArg().empty()) {
			Log::info("%-*s - %s %s", maxWidthOnlyLong,
				a.longArg().c_str(), a.description().c_str(), defaultVal.c_str());
		} else {
			Log::info("%-*s | %-*s - %s %s", maxWidthLong,
				a.longArg().c_str(), maxWidthShort, a.shortArg().c_str(), a.description().c_str(), defaultVal.c_str());
		}
	}

	int maxWidth = 0;
	core::Var::visit([&] (const core::VarPtr& v) {
		maxWidth = core_max(maxWidth, (int)v->name().size());
	});
	command::Command::visit([&] (const command::Command& c) {
		maxWidth = core_max(maxWidth, (int)SDL_strlen(c.name()));
	});

	Log::info("------------");
	Log::info("Config variables:");
	util::visitVarSorted([=] (const core::VarPtr& v) {
		const uint32_t flags = v->getFlags();
		core::String flagsStr = "     ";
		const char *value = v->strVal().c_str();
		if ((flags & core::CV_READONLY) != 0) {
			flagsStr[0]  = 'R';
		}
		if ((flags & core::CV_NOPERSIST) != 0) {
			flagsStr[1]  = 'N';
		}
		if ((flags & core::CV_SHADER) != 0) {
			flagsStr[2]  = 'S';
		}
		if ((flags & core::CV_SECRET) != 0) {
			flagsStr[3]  = 'X';
			value = "***secret***";
		}
		if (v->isDirty()) {
			flagsStr[4]  = 'D';
		}
		Log::info("   %-*s %s %s", maxWidth, v->name().c_str(), flagsStr.c_str(), value);
		if (v->help() != nullptr) {
			Log::info("   -- %s", v->help());
		}
	}, 0u);
	Log::info("Flags:");
	Log::info("   %-*s Readonly  can't get modified at runtime - only at startup", maxWidth, "R");
	Log::info("   %-*s Nopersist value won't get persisted in the cfg file", maxWidth, "N");
	Log::info("   %-*s Shader    changing the value would result in a recompilation of the shaders", maxWidth, "S");
	Log::info("   %-*s Dirty     the config variable is dirty, means that the initial value was changed", maxWidth, "D");
	Log::info("   %-*s Secret    the value of the config variable won't be shown in the logs", maxWidth, "X");

	Log::info("------------");
	Log::info("Commands:");
	command::Command::visitSorted([=] (const command::Command& c) {
		Log::info("   %-*s %s", maxWidth, c.name(), c.help());
	});
	Log::info("------------");
	Log::info("Search paths:");
	const io::Paths& paths = _filesystem->paths();
	for (const core::String& path : paths) {
		Log::info(" * %s", path.c_str());
	}
	Log::info("------------");
	Log::info("Config variables can either be set via autoexec.cfg, %s.vars, environment or commandline parameter.", _appname.c_str());
	Log::info("The highest order is the command line. If you specify it on the command line, every other method");
	Log::info("will not be used. If the engine finds the cvar name in your environment variables, this one will");
	Log::info("take precendence over the one the is found in the configuration file. Next is the configuration");
	Log::info("file - this one will take precendence over the default settings that are specified in the code.");
	Log::info("The environment variable can be either lower case or upper case. For example it will work if you");
	Log::info("have CL_GAMMA or cl_gamma exported. The lower case variant has the higher priority.");
	Log::info("Examples:");
	Log::info("export the variable CORE_LOGLEVEL with the value 1 to override previous values.");
	Log::info("%s -set core_loglevel 1.", _appname.c_str());
}

void App::onAfterRunning() {
}

void App::onBeforeRunning() {
}

AppState App::onRunning() {
	if (_logLevelVar->isDirty() || _syslogVar->isDirty()) {
		Log::init();
		_logLevelVar->markClean();
		_syslogVar->markClean();
	}

	command::Command::update(_deltaFrameSeconds);

	if (!_failedToSaveConfiguration && core::Var::needsSaving()) {
		if (!saveConfiguration()) {
			_failedToSaveConfiguration = true;
			Log::warn("Failed to save configuration");
		}
	}

	return AppState::Cleanup;
}

bool App::hasArg(const core::String& arg) const {
	for (int i = 1; i < _argc; ++i) {
		if (arg == _argv[i]) {
			return true;
		}
	}
	for (const Argument& a : _arguments) {
		if (a.longArg() == arg || a.shortArg() == arg) {
			for (int i = 1; i < _argc; ++i) {
				if (a.longArg() == _argv[i] || a.shortArg() == _argv[i]) {
					return true;
				}
			}
			break;
		}
	}
	return false;
}

core::String App::getArgVal(const core::String& arg, const core::String& defaultVal, int* argi) {
	int start = argi == nullptr ? 1 : core_max(1, *argi);
	for (int i = start; i < _argc; ++i) {
		if (arg != _argv[i]) {
			continue;
		}
		if (i + 1 < _argc) {
			if (argi != nullptr) {
				*argi = i + 1;
			}
			return _argv[i + 1];
		}
	}
	for (const Argument& a : _arguments) {
		if (a.longArg() != arg && a.shortArg() != arg) {
			continue;
		}
		for (int i = start; i < _argc; ++i) {
			if (a.longArg() != _argv[i] && a.shortArg() != _argv[i]) {
				continue;
			}
			if (i + 1 < _argc) {
				if (argi != nullptr) {
					*argi = i + 1;
				}
				return _argv[i + 1];
			}
		}
		if (!a.mandatory()) {
			if (!defaultVal.empty()) {
				return defaultVal;
			}
			return a.defaultValue();
		}
		if (defaultVal.empty() && a.defaultValue().empty()) {
			usage();
			requestQuit();
		}
		if (!defaultVal.empty()) {
			return defaultVal;
		}
		return a.defaultValue();
	}
	return "";
}

App::Argument& App::registerArg(const core::String& arg) {
	const App::Argument argument(arg);
	_arguments.push_back(argument);
	return _arguments.back();
}

bool App::saveConfiguration() {
	if (_organisation.empty() || _appname.empty()) {
		Log::debug("don't save the config variables because organisation or appname is missing");
		return false;
	}
	const core::String filename = _appname + ".vars";
	Log::debug("save the config variables to '%s'", filename.c_str());
	core::String ss;
	ss.reserve(16384);
	util::visitVarSorted([&](const core::VarPtr& var) {
		if ((var->getFlags() & core::CV_NOPERSIST) != 0u) {
			return;
		}
		const uint32_t flags = var->getFlags();
		core::String flagsStr;
		const char *value = var->strVal().c_str();
		if ((flags & core::CV_READONLY) == core::CV_READONLY) {
			flagsStr.append("R");
		}
		if ((flags & core::CV_SHADER) == core::CV_SHADER) {
			flagsStr.append("S");
		}
		if ((flags & core::CV_SECRET) == core::CV_SECRET) {
			flagsStr.append("X");
		}
		ss += "\"";
		ss += var->name();
		ss += "\" \"";
		ss += value;
		ss += "\" \"";
		ss += flagsStr;
		ss += "\"\n";
	}, 0u);
	return _filesystem->write(filename, ss);
}

AppState App::onCleanup() {
	if (_suspendRequested) {
		addBlocker(AppState::Init);
		return AppState::Init;
	}

	saveConfiguration();

	_threadPool->shutdown();

	command::Command::shutdown();
	core::Var::shutdown();

	const SDL_AssertData *item = SDL_GetAssertionReport();
	while (item != nullptr) {
		Log::warn("'%s', %s (%s:%d), triggered %u times, always ignore: %s.\n",
				item->condition, item->function, item->filename, item->linenum,
				item->trigger_count, item->always_ignore != 0 ? "yes" : "no");
		item = item->next;
	}
	SDL_ResetAssertionReport();

	_filesystem->shutdown();

	core_trace_shutdown();

	SDL_Quit();

	return AppState::Destroy;
}

AppState App::onDestroy() {
	SDL_Quit();
	return AppState::InvalidAppState;
}

void App::readyForInit() {
	remBlocker(AppState::Init);
}

bool App::allowedToQuit() {
	return true;
}

void App::requestQuit() {
	if (AppState::Running == _curState) {
		if (allowedToQuit()) {
			_nextState = AppState::Cleanup;
		}
	} else {
		_nextState = AppState::Destroy;
	}
}

void App::requestSuspend() {
	_nextState = AppState::Cleanup;
	_suspendRequested = true;
}

const core::String& App::currentWorkingDir() const {
	return _filesystem->basePath();
}

core::ThreadPool& App::threadPool() {
	return *_threadPool.get();
}

}
