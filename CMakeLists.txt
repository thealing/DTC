cmake_minimum_required(VERSION 3.20)

project("dtc")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)

	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

	add_compile_options(/GF /GS- /sdl-)

	add_link_options(/MANIFEST:NO)

	if(CMAKE_BUILD_TYPE STREQUAL "Release")

		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

	endif()

endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/source")

add_compile_definitions(_HAS_EXCEPTIONS=0)

add_compile_definitions(UNICODE _UNICODE)

add_compile_definitions(NOMINMAX)

add_executable("${PROJECT_NAME}" "Source.cpp")

set_target_properties("${PROJECT_NAME}" PROPERTIES OUTPUT_NAME "${PROJECT_NAME}" PDB_NAME "${PROJECT_NAME}.exe" PREFIX "")

set(TARGET_DEMO "demo")

set(DEMO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/demo/main.dtl")

set(DEMO_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

set(DEMO_TEMP_FILE "${DEMO_OUTPUT_DIR}/demo/main.dtl.i")

set(DEMO_BUILD_FILE "${DEMO_OUTPUT_DIR}/build.c")

set_property(DIRECTORY PROPERTY ADDITIONAL_CLEAN_FILES "${DEMO_OUTPUT_DIR}")

file(GLOB_RECURSE DEMO_DTL_DEPENDENCIES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/demo/*.dtl")

get_filename_component(DEMO_TEMP_DIR "${DEMO_TEMP_FILE}" DIRECTORY)

add_custom_command(OUTPUT "${DEMO_TEMP_FILE}" COMMAND "${CMAKE_COMMAND}" -E make_directory "${DEMO_TEMP_DIR}" DEPENDS "${DEMO_DTL_DEPENDENCIES}")

if(MSVC)

	add_custom_command(OUTPUT "${DEMO_TEMP_FILE}" APPEND COMMAND "${CMAKE_C_COMPILER}" /P /Fi"${DEMO_TEMP_FILE}" "${DEMO_SOURCE}")

else()

	add_custom_command(OUTPUT "${DEMO_TEMP_FILE}" APPEND COMMAND "${CMAKE_C_COMPILER}" -E -x c "${DEMO_SOURCE}" -o "${DEMO_TEMP_FILE}.tmp")

	add_custom_command(OUTPUT "${DEMO_TEMP_FILE}" APPEND COMMAND "${CMAKE_COMMAND}" -DINPUT="${DEMO_TEMP_FILE}.tmp" -DOUTPUT="${DEMO_TEMP_FILE}" -P "${CMAKE_SOURCE_DIR}/gcc_reld.cmake")

	add_custom_command(OUTPUT "${DEMO_TEMP_FILE}" APPEND COMMAND "${CMAKE_COMMAND}" -E remove "${DEMO_TEMP_FILE}.tmp")

endif()

set(TARGET_DEMO_BUILD_COMMAND "${PROJECT_NAME}" "-l" "-e" "${DEMO_BUILD_FILE}" "${DEMO_TEMP_FILE}")

add_custom_command(OUTPUT "${DEMO_BUILD_FILE}" COMMAND ${TARGET_DEMO_BUILD_COMMAND} DEPENDS "${PROJECT_NAME}" "${DEMO_TEMP_FILE}")

add_executable("${TARGET_DEMO}" "${DEMO_BUILD_FILE}")
