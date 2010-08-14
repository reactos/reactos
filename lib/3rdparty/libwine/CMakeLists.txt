include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

file(GLOB_RECURSE SOURCE "*.c")
list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/debug.c)

add_library(wine ${SOURCE})

add_library(wineldr loader.c)
