
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(hhctrl.ocx hhctrl.ocx.spec)

add_library(hhctrl SHARED
    chm.c
    content.c
    help.c
    hhctrl.c
    index.c
    regsvr.c
    search.c
    stream.c
    webbrowser.c
    hhctrl.rc    
    ${CMAKE_CURRENT_BINARY_DIR}/hhctrl.def)

set_module_type(hhctrl win32ocx)

target_link_libraries(hhctrl
    uuid
    wine)

add_importlibs(hhctrl
    advapi32
    comctl32
    shlwapi
    ole32
    oleaut32
    user32
    gdi32
    kernel32
    ntdll)


