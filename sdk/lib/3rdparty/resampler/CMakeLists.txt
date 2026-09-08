#cmake_minimum_required(VERSION 2.8)
project(resampler LANGUAGES C)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(CMAKE_BUILD_TYPE "Release")

include_directories(include)
#add_executable(resampler main.c)
add_library(resampler main.c)
set_target_properties(resampler PROPERTIES LINKER_LANGUAGE C)

add_dependencies(resampler xdk)
target_link_libraries(resampler -lm)

