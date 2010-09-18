


add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mpr ${CMAKE_CURRENT_SOURCE_DIR}/mpr.spec ${CMAKE_CURRENT_BINARY_DIR}/mpr.def)

add_library(mpr SHARED
    auth.c
    mpr_main.c
    multinet.c
    nps.c
    pwcache.c
    wnet.c
    mpr.rc)

set_target_properties(mpr PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mpr
    ${CMAKE_CURRENT_BINARY_DIR}/mpr.def
    wine
    mingw_common
    -ladvapi32
    -luser32)

add_dependencies(mpr mpr_def psdk)
