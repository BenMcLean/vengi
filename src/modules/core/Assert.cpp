/**
 * @file
 */

#include "Assert.h"
#include "Log.h"
#include <SDL_stdinc.h>

#ifdef HAVE_BACKWARD
#include <backward.h>
#endif

void core_stacktrace() {
#ifdef HAVE_BACKWARD
	backward::StackTrace st;
	st.load_here(32);
	backward::TraceResolver tr;
	tr.load_stacktrace(st);
	for (size_t __stacktrace_i = 0; __stacktrace_i < st.size(); ++__stacktrace_i) {
		const backward::ResolvedTrace& trace = tr.resolve(st[__stacktrace_i]);
		Log::error("#%i %s %s [%p]", int(__stacktrace_i), trace.object_filename.c_str(), trace.object_function.c_str(), trace.addr);
	}
#endif
}

SDL_AssertState core_assert_impl_message(SDL_AssertData &sdl_assert_data, char *buf, int bufSize,
										 const char *function, const char *file, int line, const char *format, ...) {
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(buf, bufSize - 1, format, args);
	va_end(args);
	sdl_assert_data.condition = buf; /* also let it work for following calls */
	const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, function, file, line);
	if (sdl_assert_state == SDL_ASSERTION_RETRY) {
		return sdl_assert_state;
	}
	if (sdl_assert_state == SDL_ASSERTION_BREAK) {
		SDL_TriggerBreakpoint();
	} else if (sdl_assert_data.trigger_count <= 1) {
		core_stacktrace();
	}
	return sdl_assert_state;
}
