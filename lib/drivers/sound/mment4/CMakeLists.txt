
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DDEBUG_NT4=1)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

file(GLOB_RECURSE SOURCE "*.c")

add_library(mment4 ${SOURCE})
add_dependencies(mment4 psdk)
