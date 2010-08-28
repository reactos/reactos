
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

spec2def(netapi32 ${CMAKE_CURRENT_SOURCE_DIR}/netapi32.spec ${CMAKE_CURRENT_BINARY_DIR}/netapi32.def)

add_library(netapi32 SHARED
    access.c
    apibuf.c
    browsr.c
    ds.c
    local_group.c
    nbcmdqueue.c
    nbnamecache.c
    nbt.c
    netapi32.c
    netbios.c
    share.c
    wksta.c)

set_target_properties(netapi32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(netapi32
    ${CMAKE_CURRENT_BINARY_DIR}/netapi32.def
    wine
    -liphlpapi
    -lws2_32
    -ladvapi32
    mingw_common)

add_dependencies(netapi32 netapi32_def psdk)
