cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(minecraft VERSION 1.0.0 LANGUAGES CXX)

set(TARGET "minecraft")
set(src "../src")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall)

if (WIN32)
  set(platform_sources ${src}/platform/win32.cpp)
elseif (UNIX)
  set(platform_sources ${src}/platform/linux.cpp)
endif ()

set(sources
  ${platform_sources}
  ${src}/main.cpp ${src}/util.cpp ${src}/shaders.cpp
  ${src}/world.cpp
  ${src}/image.cpp
  ${src}/texture.cpp
  third-party/OpenSimplexNoise/OpenSimplexNoise/OpenSimplexNoise.cpp
  third-party/imgui/imgui.cpp
  third-party/imgui/imgui_draw.cpp
  third-party/imgui/imgui_widgets.cpp
  third-party/imgui/imgui_tables.cpp
  third-party/imgui/backends/imgui_impl_opengl3.cpp
  third-party/imgui/backends/imgui_impl_glfw.cpp
)

if (DISABLE_CURSOR STREQUAL "1")
    add_definitions(-DDISABLE_CURSOR)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
  add_compile_options(-Wall -O0 -g3 -ggdb -gdwarf-3)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O3)
endif ()

include_directories(./third-party)

include_directories(./third-party/imgui/backends)
include_directories(./third-party/imgui)

add_executable(${TARGET} ${sources})

# target_link_libraries(${TARGET} ${CMAKE_DL_LIBS})

find_package(Threads)
target_link_libraries(${TARGET} ${CMAKE_THREAD_LIBS_INIT})

find_package(fmt)
target_link_libraries(${TARGET} fmt::fmt)

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(${TARGET} ${GLEW_LIBRARIES})

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(${TARGET} glfw)

find_package(OpenGL REQUIRED)
target_link_libraries(${TARGET} OpenGL::GL ${CMAKE_DL_LIBS})

# Performance debug (gperftools)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
link_directories(./third-party/gperftools)
target_link_libraries(${TARGET} tcmalloc profiler)

