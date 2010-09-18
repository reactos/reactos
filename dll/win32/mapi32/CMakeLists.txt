


add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mapi32 ${CMAKE_CURRENT_SOURCE_DIR}/mapi32.spec ${CMAKE_CURRENT_BINARY_DIR}/mapi32.def)

add_library(mapi32 SHARED
    imalloc.c
    mapi32_main.c
    prop.c
    sendmail.c
    util.c
    version.rc)

set_target_properties(mapi32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(mapi32
    ${CMAKE_CURRENT_BINARY_DIR}/mapi32.def
    wine
    -lshlwapi
    -lshell32
    -luuid
    mingw_common
    -luser32
    -ladvapi32)

add_dependencies(mapi32 mapi32_def psdk)
