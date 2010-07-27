
include_directories(.)
file(GLOB_RECURSE SOURCE "*.c")
add_library(vfatxlib ${SOURCE})
