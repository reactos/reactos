
include_directories(BEFORE include/freebsd/src/sys)
include_directories(include/freebsd)
include_directories(include/freebsd/sys/include)
include_directories(include/freebsd/dev/include)
include_directories(include/freebsd/net/include)

include_directories(include)

add_definitions(-D__NTDRIVER__)
add_definitions(-DKERNEL)
add_definitions(-DOSKIT)

file(GLOB_RECURSE SOURCE "oskittcp/*.c")

add_library(oskittcp ${SOURCE})
add_dependencies(oskittcp bugcodes)

