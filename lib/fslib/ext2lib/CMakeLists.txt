
file(GLOB_RECURSE SOURCE "*.c")
add_library(ext2lib ${SOURCE})
add_dependencies(ext2lib psdk)