
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DNDEBUG=1)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

file(GLOB_RECURSE SOURCE "*.c")

add_library(mmebuddy ${SOURCE})
add_dependencies(mmebuddy psdk)
