
add_definitions(-D__WINESRC__ -D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(itss.dll itss.spec)

add_library(itss SHARED
    chm_lib.c
    lzx.c
    itss.c
    moniker.c
    protocol.c
    storage.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/itss.def)

set_module_type(itss win32dll)

target_link_libraries(itss
    uuid
    wine)

add_importlibs(itss
    urlmon
    shlwapi
    ole32
    kernel32
    ntdll)

add_dependencies(itss wineheaders)
add_cab_target(itss 1)
