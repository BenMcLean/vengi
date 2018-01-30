/**
 * @file
 */

#include "core/Trace.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/command/Command.h"

#if USE_EMTRACE
#include <emscripten/trace.h>
#endif

namespace core {

namespace {

static TraceCallback* _callback = nullptr;

}

Trace::Trace(uint16_t port) {
#if USE_EMTRACE
	emscripten_trace_configure(core::string::format("http://localhost:%i/", (int)port).c_str(), "Engine");
#endif
	traceThread("MainThread");
}

Trace::~Trace() {
#if USE_EMTRACE
	emscripten_trace_close();
#endif
}

TraceScoped::TraceScoped(const char* name, const char *msg) {
	traceBegin(name);
	traceMessage(msg);
}

TraceScoped::~TraceScoped() {
	traceEnd();
}

TraceGLScoped::TraceGLScoped(const char* name, const char *msg) {
	traceGLBegin(name);
	traceMessage(msg);
}

TraceGLScoped::~TraceGLScoped() {
	traceGLEnd();
}

TraceCallback* traceSet(TraceCallback* callback) {
	TraceCallback* old = _callback;
	_callback = callback;
	return old;
}

void traceInit() {
#if USE_EMTRACE
	Log::info("emtrace active");
#endif
}

void traceGLInit() {
}

void traceShutdown() {
}

void traceGLShutdown() {
}

void traceBeginFrame() {
#if USE_EMTRACE
	emscripten_trace_record_frame_start();
#else
	if (_callback != nullptr) {
		_callback->traceBeginFrame();
	} else {
		traceBegin("Frame");
	}

#endif
}

void traceEndFrame() {
#if USE_EMTRACE
	emscripten_trace_record_frame_end();
#else
	if (_callback != nullptr) {
		_callback->traceEndFrame();
	} else {
		traceEnd();
	}
#endif
}

void traceBegin(const char* name) {
#if USE_EMTRACE
	emscripten_trace_enter_context(name);
#else
	if (_callback != nullptr) {
		_callback->traceBegin(name);
	}
#endif
}

void traceEnd() {
#if USE_EMTRACE
	emscripten_trace_exit_context();
#else
	if (_callback != nullptr) {
		_callback->traceEnd();
	}
#endif
}

void traceGLBegin(const char* name) {
	traceBegin(name);
}

void traceGLEnd() {
	traceEnd();
}

void traceMessage(const char* message) {
	if (message == nullptr) {
		return;
	}
	Log::trace("%s", message);
}

void traceThread(const	 char* name) {
	traceMessage(name);
}

}
