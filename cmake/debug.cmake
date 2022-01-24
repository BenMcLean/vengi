find_program(GDB_EXECUTABLE gdb)
find_program(LLDB_EXECUTABLE lldb)
if (GDB_EXECUTABLE)
	set(DEBUGGER ${GDB_EXECUTABLE} CACHE STRING "Which debugger should be used")
elseif (LLDB_EXECUTABLE)
	set(DEBUGGER ${LLDB_EXECUTABLE} CACHE STRING "Which debugger should be used")
else()
	set(DEBUGGER "unknown" CACHE STRING "Which debugger should be used")
	message(STATUS "No debugger (gdb or lldb) was found")
endif()
set_property(CACHE DEBUGGER PROPERTY STRINGS gdb lldb)

function(engine_add_debuggger TARGET)
	if (${DEBUGGER} MATCHES "gdb")
		add_custom_target(${TARGET}-debug)
		add_custom_command(TARGET ${TARGET}-debug
			COMMAND ${GDB_EXECUTABLE} -ex run --args $<TARGET_FILE:${TARGET}>
			COMMENT "Starting debugger session for ${TARGET}"
			USES_TERMINAL
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	elseif (${DEBUGGER} MATCHES "lldb")
		add_custom_target(${TARGET}-debug)
		add_custom_command(TARGET ${TARGET}-debug
			COMMAND CG_CONTEXT_SHOW_BACKTRACE=1 ${LLDB_EXECUTABLE} -b -o run $<TARGET_FILE:${TARGET}>
			COMMENT "Starting debugger session for ${TARGET}"
			USES_TERMINAL
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endfunction()
