include_directories(.)
include_directories(./lib)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/expat)

add_definitions(-DHAVE_EXPAT_CONFIG_H)
file(GLOB_RECURSE SOURCE "lib/*.c")
LIST(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lib/xmltok_impl.c ${CMAKE_CURRENT_SOURCE_DIR}/lib/xmltok_ns.c)

add_library(expat ${SOURCE})