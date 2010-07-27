
include_directories(.)
file(GLOB_RECURSE SOURCE "*.c")
add_library(vfatlib ${SOURCE})
add_dependencies(vfatlib psdk)
