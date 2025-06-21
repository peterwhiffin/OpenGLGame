cmake_minimum_required(VERSION 3.10)
project(OpenGLGame CXX)

set(CMAKE_CXX_STANDARD 17)

# Gather source files only (no headers!)
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/utils/*.c" "src/utils/*.cpp" "src/utils/soloud/*.cpp" "src/utils/soloud/*.c")

add_executable(dummy ${SOURCES})

# Add glad's include directory so compiler can find glad.h
target_include_directories(dummy PRIVATE "${CMAKE_SOURCE_DIR}/include" "../JoltPhysics-5.3.0" "../OpenAL-soft/include/AL" "../miniaudio-0.11.22" "../soloud20200207/include")
target_compile_definitions(dummy PRIVATE JPH_DEBUG_RENDERER PETES_EDITOR WITH_MINIAUDIO)
