


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mciseq ${CMAKE_CURRENT_SOURCE_DIR}/mciseq.spec ${CMAKE_CURRENT_BINARY_DIR}/mciseq.def)

add_library(mciseq SHARED mcimidi.c)

set_target_properties(mciseq PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(mciseq
    ${CMAKE_CURRENT_BINARY_DIR}/mciseq.def
    wine
    -lwinmm
    mingw_common
    -luser32)

add_dependencies(mciseq mciseq_def psdk)
