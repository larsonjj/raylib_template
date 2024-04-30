cmake_minimum_required(VERSION 3.22)
include(FetchContent)
include(AddGitSubmodule.cmake)

project(raylib_template
	VERSION 0.1.0
	DESCRIPTION "Raylib template using SDL2 and Box2d"
	HOMEPAGE_URL "https://github.com/larsonjj/raylib_template"
	LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_COMPILE_WARNING_AS_ERROR OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Define PROJECT_INCLUDE to be the path to the include directory of the project
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src")
set(BUILD_SHARED_LIBS FALSE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES OFF CACHE BOOL "" FORCE) # don't build the supplied example games
set(PLATFORM "SDL" CACHE STRING "" FORCE) # build Raylib using SDL2
set(OPENGL_VERSION "ES 2.0" CACHE STRING "" FORCE) # build Raylib for OpenGL ES 2.0

if (APPLE)
    set(ANGLE_DIR "${CMAKE_CURRENT_LIST_DIR}/external/angle/macos")
endif()
if (WIN32)
    set(ANGLE_DIR "${CMAKE_CURRENT_LIST_DIR}/external/angle/win64")
    set(ANGLE_INCLUDE_DIR "${ANGLE_DIR}/include")
    set(ANGLE_BIN_DIR "${ANGLE_DIR}/bin")
    set(ANGLE_LIB_DIR "${ANGLE_DIR}/lib")
endif()

FetchContent_Declare(
	box2d
	GIT_REPOSITORY https://github.com/erincatto/box2c.git
	GIT_TAG 0c30b825863d1d3d31dcfe0f8005751f2508f5e2
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(box2d)

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG 8b06473a11484ddc8e27484f879837bdf54d450e
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        OVERRIDE_FIND_PACKAGE # Force Raylib to find and use the SDL3 we just fetched
)
FetchContent_MakeAvailable(SDL3)

add_git_submodule(external/raylib)

add_executable(${PROJECT_NAME})

find_library(OPENGL_LIBRARY OpenGL)
if (APPLE)
    # Link required system frameworks for macOS
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(CORE_VIDEO_LIBRARY CoreVideo)
endif()

if (APPLE)
    # Define PROJECT_SOURCES as a list of all source files
    file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
    target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_HINT_OPENGL_ES_DRIVER=0)
    target_link_libraries(${PROJECT_NAME} PRIVATE box2d raylib SDL3::SDL3 SDL3::SDL3-static ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY} ${CORE_VIDEO_LIBRARY})
    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
    # Copy the MacOS Angle libraries to the build directory
    file(COPY "${ANGLE_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}")
elseif (WIN32)
    # Define PROJECT_SOURCES as a list of all source files
    file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c" "${ANGLE_INCLUDE_DIR}/EGL/*.h" "${ANGLE_INCLUDE_DIR}/GLES2/*.h" "${ANGLE_INCLUDE_DIR}/GLES3/*.h")

    target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_HINT_OPENGL_ES_DRIVER=0 SDL_GL_CONTEXT_MAJOR_VERSION=2 SDL_GL_CONTEXT_MINOR_VERSION=0 SDL_GL_CONTEXT_PROFILE_MASK=SDL_GL_CONTEXT_PROFILE_ES SDL_HINT_VIDEO_WIN_D3DCOMPILER="d3dcompiler_47.dll")
    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE} ${ANGLE_INCLUDE_DIR} ${ANGLE_LIB_DIR})

    find_library(ANGLE_GLESv2_DLL libGLESv2.dll.lib PATHS ${ANGLE_LIB_DIR})
    if(NOT ANGLE_GLESv2_DLL)
        message(FATAL_ERROR "ANGLE_GLESv2_DLL library not found")
    else()
        message(STATUS "Found ANGLE_GLESv2_DLL: ${ANGLE_GLESv2_DLL}")
    endif()

    find_library(ANGLE_EGL_DLL libEGL.dll.lib PATHS ${ANGLE_LIB_DIR})
    if(NOT ANGLE_EGL_DLL)
        message(FATAL_ERROR "ANGLE_EGL_DLL library not found")
    else()
        message(STATUS "Found ANGLE_EGL_DLL: ${ANGLE_EGL_DLL}")
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE box2d raylib SDL3::SDL3 SDL3::SDL3-static ${ANGLE_GLESv2_DLL} ${ANGLE_EGL_DLL})

    # Copy the Win64 Angle libraries to the build directory
    file(COPY "${ANGLE_BIN_DIR}/" DESTINATION "${CMAKE_BINARY_DIR}")
else()
    # Define PROJECT_SOURCES as a list of all source files
    file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
    target_link_libraries(${PROJECT_NAME} PRIVATE box2d raylib SDL3::SDL3 SDL3::SDL3-static)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
endif()

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

# Define static assets to be copied to the build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets/" DESTINATION "${CMAKE_BINARY_DIR}/assets")

target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
