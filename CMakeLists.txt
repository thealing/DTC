cmake_minimum_required(VERSION 3.2)

project("dtc")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)

	string(REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

	string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	
	string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	
	string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

	add_compile_options(/GF /GS- /sdl-)

	add_link_options(/MANIFEST:NO)

endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/source")

add_compile_definitions(_HAS_EXCEPTIONS=0)

add_compile_definitions(UNICODE _UNICODE)

add_compile_definitions(NOMINMAX)

add_executable("${PROJECT_NAME}" "Source.cpp" "source/main/client.hpp" "source/core/parser.hpp" "source/core/char.hpp" "source/engine/types.hpp" "source/engine/value_type.hpp")

set_target_properties("${PROJECT_NAME}" PROPERTIES OUTPUT_NAME "${PROJECT_NAME}" PDB_NAME "${PROJECT_NAME}.exe" PREFIX "")
