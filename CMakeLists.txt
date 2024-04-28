cmake_minimum_required(VERSION 3.22)
include(FetchContent)
include(AddGitSubmodule.cmake)

project(raylib_template
	VERSION 0.1.0
	DESCRIPTION "Raylib template using SDL2 and Box2d"
	HOMEPAGE_URL "https://github.com/larsonjj/raylib_template"
	LANGUAGES C
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(RAYLIB_INCLUDE_DIR external/raylib/src)
set(RAYLIB_LIBRARY_DIR external/raylib/src)
set(ANGLE_LIBRARY_DIR ${CMAKE_CURRENT_LIST_DIR})
# Define PROJECT_INCLUDE to be the path to the include directory of the project
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src")
set(BUILD_SHARED_LIBS FALSE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES OFF CACHE BOOL "" FORCE) # don't build the supplied example games
set(PLATFORM "SDL" CACHE STRING "" FORCE) # build Raylib using SDL2
set(OPENGL_VERSION "ES 2.0" CACHE STRING "" FORCE) # build Raylib for OpenGL ES 2.0
set(SDL_OPENGLES ON CACHE BOOL "" FORCE) # build SDL for OpenGL ES 2.0
set(SDL_OPENGL ON CACHE BOOL "" FORCE) # build SDL for OpenGL 3.3
set(SDL_VIDEO_DRIVER_COCOA ON CACHE BOOL "" FORCE) # build Angle for OpenGL ES 2.0 and SDL

FetchContent_Declare(
	box2d
	GIT_REPOSITORY https://github.com/erincatto/box2c.git
	GIT_TAG main
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(box2d)

FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        OVERRIDE_FIND_PACKAGE # Force Raylib to find and use the SDL2 we just fetched
)
FetchContent_MakeAvailable(SDL2)

add_git_submodule(external/raylib)

if (APPLE)
    # Link required system frameworks for macOS
    find_library(COCOA_LIBRARY Cocoa)
    find_library(OPENGL_LIBRARY OpenGL)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(CORE_VIDEO_LIBRARY CoreVideo)
    find_library(ANGLE_GLESv2_LIBRARY libGLESv2.dylib PATHS ${ANGLE_LIBRARY_DIR})
    find_library(ANGLE_EGL_LIBRARY libEGL.dylib PATHS ${ANGLE_LIBRARY_DIR})
endif()

# Define PROJECT_SOURCES as a list of all source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
# Define static assets to be copied to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION "${CMAKE_BINARY_DIR}/assets")
# file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/external/angle/macos/arm64/" DESTINATION "${CMAKE_BINARY_DIR}")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})

if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE box2d raylib SDL2::SDL2main SDL2::SDL2-static ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY} ${CORE_VIDEO_LIBRARY} ${ANGLE_GLESv2_LIBRARY} ${ANGLE_EGL_LIBRARY})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE box2d raylib SDL2::SDL2main SDL2::SDL2-static)
endif()

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
