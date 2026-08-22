
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../dmusic)

spec2def(dmstyle.dll dmstyle.spec)

list(APPEND SOURCE
    auditiontrack.c
    chordtrack.c
    commandtrack.c
    dmstyle_main.c
    dmutils.c
    motiftrack.c
    mutetrack.c
    style.c
    styletrack.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../dmusic/dmobject.c)

add_library(dmstyle MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmstyle.def)

add_idl_reg_scripts(dmstyle registry dmstyle.idl)

set_module_type(dmstyle win32dll)
target_link_libraries(dmstyle wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dmstyle ole32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(dmstyle wineheaders)
add_cd_file(TARGET dmstyle DESTINATION reactos/system32 FOR all)
set_wine_module(dmstyle)
