
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/icu4ros/icu/source/common)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -UWINVER -DWINVER=0x600 -fno-exceptions -fno-rtti")

file(GLOB_RECURSE SOURCE "*.cpp")

add_library(normalize ${SOURCE})