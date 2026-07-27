
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dmusic)

spec2def(dmcompos.dll dmcompos.spec)

list(APPEND SOURCE
    chordmap.c
    chordmaptrack.c
    composer.c
    dmcompos_main.c
    signposttrack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/dmobject.c)

add_library(dmcompos MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmcompos.def)

add_idl_reg_scripts(dmcompos registry dmcompos.idl)

set_module_type(dmcompos win32dll)
target_link_libraries(dmcompos wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dmcompos ole32 advapi32 ucrtbase msvcrt kernel32 ntdll)
add_dependencies(dmcompos wineheaders)
add_cd_file(TARGET dmcompos DESTINATION reactos/system32 FOR all)
set_wine_module(dmcompos)
