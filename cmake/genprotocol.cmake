function(protocol_include_dir TARGET DEPENDENCY)
	set(GEN_DIR ${GENERATE_DIR}/protocol/${DEPENDENCY}/)
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
endfunction()

function(generate_protocol TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	set(GEN_DIR ${GENERATE_DIR}/protocol/${TARGET}/)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	set(_headers)
	foreach (_file ${files})
		get_filename_component(_basefilename ${_file} NAME_WE)
		set(HEADER "${_basefilename}_generated.h")
		set(DEFINITION ${_file})
		list(APPEND _headers ${GEN_DIR}${HEADER})
		if (CMAKE_CROSS_COMPILING)
			message(STATUS "Looking for native tool in ${NATIVE_BUILD_DIR}")
			find_program(FLATC_EXECUTABLE NAMES vengi-flatc PATHS ${NATIVE_BUILD_DIR}/flatc)
			add_custom_command(
				OUTPUT ${GEN_DIR}${HEADER}
				COMMAND ${FLATC_EXECUTABLE} -c --scoped-enums -o ${GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/protocolq/${DEFINITION}
				DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocol/${DEFINITION}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMENT "Generating source code for ${DEFINITION}"
			)
		else()
			add_custom_command(
				OUTPUT ${GEN_DIR}${HEADER}
				COMMAND flatc -c --scoped-enums -o ${GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/protocol/${DEFINITION}
				DEPENDS flatc ${CMAKE_CURRENT_SOURCE_DIR}/protocol/${DEFINITION}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				COMMENT "Generating source code for ${DEFINITION}"
			)
		endif()
		engine_mark_as_generated(${GEN_DIR}/${HEADER})
	endforeach()

	add_custom_target(GenerateNetworkMessages${TARGET}
		DEPENDS ${_headers}
		COMMENT "Generate network messages in ${GEN_DIR}"
	)
	add_dependencies(${TARGET} GenerateNetworkMessages${TARGET})
	add_dependencies(codegen GenerateNetworkMessages${TARGET})
endfunction()
