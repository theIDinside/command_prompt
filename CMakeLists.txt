cmake_minimum_required(VERSION 3.10)
project(cp)
# added precompiled header macro, written by author, which can be found here: https://github.com/larsch/cmake-precompiled-header
include(PrecompiledHeader.cmake)

set(SOURCES src/cmdprompt/CommandPrompt.cpp src/cmdprompt/CommandPrompt.h)
set(CMAKE_CXX_STANDARD 17)
set(BUFFER_LIBRARY src/cmdprompt/Buffer.cpp src/cmdprompt/Buffer.h)
add_library(cplib STATIC ${SOURCES})
add_precompiled_header(cplib ./src/cmdprompt/heavy.h SOURCE_CXX ./src/cmdprompt/heavy.cpp)
# add_library(buflib STATIC ${BUFFER_LIBRARY})
add_executable(command_prompt_test tests/main.cpp)
target_link_libraries(command_prompt_test cplib)