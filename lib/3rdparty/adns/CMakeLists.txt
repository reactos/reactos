include_directories(./src)
include_directories(./adns_win32)

add_definitions(-DADNS_JGAA_WIN32)

file(GLOB_RECURSE SOURCE "/src/*.c" "./adns_win32/*.c")

add_library(adns ${SOURCE})