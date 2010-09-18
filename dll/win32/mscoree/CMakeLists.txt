


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mscoree ${CMAKE_CURRENT_SOURCE_DIR}/mscoree.spec ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def)

add_library(mscoree SHARED corruntimehost.c mscoree_main.c)

set_target_properties(mscoree PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mscoree
    ${CMAKE_CURRENT_BINARY_DIR}/mscoree.def
    wine
    mingw_common
    -ladvapi32
    -lshell32
    -luuid)

add_dependencies(mscoree mscoree_def psdk)
