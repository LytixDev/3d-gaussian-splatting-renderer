#
# Specify minimum CMake version and project name
#
cmake_minimum_required(VERSION 3.6)
project (glowbox)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "") # nice for language servers

#
# Set python cmd, preferring python 3
#

find_program(PYTHON_3_LOCATION py)
if(NOT "${PYTHON_3_LOCATION}" STREQUAL "PYTHON_3_LOCATION-NOTFOUND") # windows shim
	execute_process(COMMAND py -3 --version RESULT_VARIABLE has_python_3)
	if("${has_python_3}" STREQUAL "0")
		set (PYTHON_CMD py -3)
	else()
		set (PYTHON_CMD py) # may use python 2
	endif()
else()
    find_program(PYTHON_3_LOCATION python3)
    if(NOT "${PYTHON_3_LOCATION}" STREQUAL "PYTHON_3_LOCATION-NOTFOUND") # windows shim
    	set (PYTHON_CMD python3)
    else()
    	set (PYTHON_CMD python)
    endif()
endif()
message("python used: ${PYTHON_CMD}")

#
# CMake setup
#
set (CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set (CMAKE_VERBOSE_MAKEFILE 0) # 1 should be used for debugging
set (CMAKE_SUPPRESS_REGENERATION TRUE) # Suppresses ZERO_CHECK
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
  if(NOT WIN32)
    set(GLAD_LIBRARIES dl)
  endif()
endif()

#
# GLFW options
#
option (GLFW_INSTALL OFF)
option (GLFW_BUILD_DOCS OFF)
option (GLFW_BUILD_EXAMPLES OFF)
option (GLFW_BUILD_TESTS OFF)
add_subdirectory (lib/glfw)


# SFML options
# We only need sound for this project, so everything else can be turned off
option (SFML_BUILD_GRAPHICS OFF)
option (SFML_BUILD_WINDOW OFF)
option (SFML_BUILD_NETWORK OFF)
add_subdirectory(lib/SFML)

#
# Add FMT
#
add_subdirectory (lib/fmt)

#
# GLAD
#
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/include)
    message("Generating glad library files, using ${PYTHON_CMD}")
	execute_process(
		COMMAND ${PYTHON_CMD} -m glad --profile core --out-path . --generator c --spec gl
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad
	)
	message("Finished generating glad library files")
endif()

#
# Set include paths
#
include_directories (src/
                     lib/glad/include/
                     lib/glfw/include/
                     lib/lodepng/
                     lib/glm/
                     lib/stb/
                     lib/arrrgh/
                     lib/SFML/include/
                     lib/imgui/)


#
# Add files
#
file (GLOB         VENDORS_SOURCES lib/glad/src/glad.c
								   lib/lodepng/lodepng.cpp
                                   lib/imgui/*.cpp)
file (GLOB_RECURSE PROJECT_HEADERS src/*.hpp
                                   src/*.h)
file (GLOB_RECURSE PROJECT_SOURCES src/*.cpp
                                   src/*.cxx
                                   src/*.cc
                                   src/*.c)
file (GLOB_RECURSE PROJECT_SHADERS res/shaders/*.comp
                                   res/shaders/*.frag
                                   res/shaders/*.geom
                                   res/shaders/*.vert)
file (GLOB         PROJECT_CONFIGS CMakeLists.txt
                                   README.rst
                                  .gitignore
                                  .gitmodules)

#
# Organizing files
#
source_group ("headers" FILES ${PROJECT_HEADERS})
source_group ("shaders" FILES ${PROJECT_SHADERS})
source_group ("sources" FILES ${PROJECT_SOURCES})
source_group ("libraries" FILES ${VENDORS_SOURCES})

#
# Set executable and target link libraries
#
add_definitions (-DGLFW_INCLUDE_NONE
                 -DPROJECT_SOURCE_DIR=\"${PROJECT_SOURCE_DIR}\")
add_executable (${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS}
                                ${PROJECT_SHADERS} ${PROJECT_CONFIGS}
                                ${VENDORS_SOURCES})
target_link_libraries (${PROJECT_NAME}
                       glfw
                       sfml-audio
                       fmt::fmt
                       ${GLFW_LIBRARIES}
                       ${GLAD_LIBRARIES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT glowbox)
