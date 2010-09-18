


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(sxs ${CMAKE_CURRENT_SOURCE_DIR}/sxs.spec ${CMAKE_CURRENT_BINARY_DIR}/sxs.def)

add_library(sxs SHARED sxs.c)

set_target_properties(sxs PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(sxs
    ${CMAKE_CURRENT_BINARY_DIR}/sxs.def
    wine
    mingw_common)

add_dependencies(sxs sxs_def psdk)
