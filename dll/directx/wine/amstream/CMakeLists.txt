
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600)

spec2def(amstream.dll amstream.spec)

list(APPEND SOURCE
    audiodata.c
    audiostream.c
    ddrawstream.c
    filter.c
    main.c
    multimedia.c)

add_library(amstream MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/amstream.def)

add_idl_reg_scripts(amstream registry amstream_classes.idl)

set_module_type(amstream win32dll)
target_link_libraries(amstream wine wine_dll_register wine_dll_canunload strmbase strmiids uuid)
add_importlibs(amstream ole32 advapi32 ddraw user32 kernel32_vista msvcrt kernel32 ntdll)
add_dependencies(amstream wineheaders)
add_cd_file(TARGET amstream DESTINATION reactos/system32 FOR all)
set_wine_module(amstream)
