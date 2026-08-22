
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dmusic)

spec2def(dmloader.dll dmloader.spec)

list(APPEND SOURCE
    container.c
    debug.c
    dmloader_main.c
    loader.c
    loaderstream.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/dmobject.c)

add_library(dmloader MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmloader.def)

add_idl_reg_scripts(dmloader registry dmloader.idl)

set_module_type(dmloader win32dll)
target_link_libraries(dmloader wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dmloader ole32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(dmloader wineheaders)
add_cd_file(TARGET dmloader DESTINATION reactos/system32 FOR all)
set_wine_module(dmloader)
