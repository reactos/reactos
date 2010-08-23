
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
