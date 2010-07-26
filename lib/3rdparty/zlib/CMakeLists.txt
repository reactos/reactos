
file(GLOB_RECURSE SOURCE "*.c")

LIST(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/minigzip.c)
LIST(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/example.c)

add_definitions(-DNO_VIZ)

if(CMAKE_CROSSCOMPILING)

add_library(zlib ${SOURCE})

else()

add_library(zlibhost ${SOURCE})

endif()

