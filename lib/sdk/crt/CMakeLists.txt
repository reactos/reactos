
include_directories(.)
include_directories(./include)

add_definitions(-D__MINGW_IMPORT=extern)
add_definitions(-DUSE_MSVCRT_PREFIX)
add_definitions(-D_MSVCRT_LIB_)
add_definitions(-D_MSVCRT_)
add_definitions(-D_MT)
add_definitions(-D_CRTBLD)

file(GLOB_RECURSE SOURCE "*.c")
LIST(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/stdio/findgen.c)
LIST(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/string/strtold.c)

add_library(crt ${SOURCE})

add_dependencies(crt psdk)