cmake_minimum_required(VERSION 2.8)
project(resampler LANGUAGES C)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
SET(CMAKE_BUILD_TYPE "Release")

include_directories(include)
add_executable(resampler main.c)

target_link_libraries(resampler -lm)