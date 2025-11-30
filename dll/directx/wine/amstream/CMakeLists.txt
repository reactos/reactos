
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(amstream.dll amstream.spec)

list(APPEND SOURCE
    audiodata.c
    audiostream.c
    ddrawstream.c
    filter.c
    guids.c
    main.c
    multimedia.c)

add_library(amstream MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/amstream.def)

set_module_type(amstream win32dll)
target_link_libraries(amstream wine_dll_register strmbase strmiids uuid wine)
add_importlibs(amstream ole32 ddraw user32 msvcrt kernel32_vista kernel32 ntdll)
#add_pch(amstream precomp.h SOURCE)
add_cd_file(TARGET amstream DESTINATION reactos/system32 FOR all)
