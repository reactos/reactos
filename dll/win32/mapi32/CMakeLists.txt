
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

set_module_type(mapi32 win32dll)

target_link_libraries(mapi32
    ${CMAKE_CURRENT_BINARY_DIR}/mapi32.def
    wine
    uuid)

add_importlibs(mapi32 shlwapi shell32 user32 advapi32 kernel32 ntdll)
add_dependencies(mapi32 mapi32_def)
add_livecd_target(mapi32 reactos/system32)
