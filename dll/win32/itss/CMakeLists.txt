
add_definitions(-D__WINESRC__ -D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(itss ${CMAKE_CURRENT_SOURCE_DIR}/itss.spec ${CMAKE_CURRENT_BINARY_DIR}/itss.def)

add_library(itss SHARED
    chm_lib.c
    lzx.c
    itss.c
    moniker.c
    protocol.c
    storage.c
    rsrc.rc)

set_module_type(itss win32dll)

target_link_libraries(itss
    ${CMAKE_CURRENT_BINARY_DIR}/itss.def
    uuid
    wine)

add_importlibs(itss
    urlmon
    shlwapi
    ole32
    kernel32
    ntdll)

add_dependencies(itss itss_def wineheaders)
add_livecd_target(itss reactos/system32)
