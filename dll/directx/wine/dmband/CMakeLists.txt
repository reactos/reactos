
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dmusic)

spec2def(dmband.dll dmband.spec)

list(APPEND SOURCE
    bandtrack.c
    dmband_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/band.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/dmobject.c)

add_library(dmband MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmband.def)

add_idl_reg_scripts(dmband registry dmband.idl)

set_module_type(dmband win32dll)
target_link_libraries(dmband wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dmband ole32 advapi32 ucrtbase msvcrt kernel32 ntdll)
add_dependencies(dmband wineheaders)
add_cd_file(TARGET dmband DESTINATION reactos/system32 FOR all)
set_wine_module(dmband)
