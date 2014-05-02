
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(avifil32.dll avifil32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    acmstream.c
    api.c
    avifile.c
    editstream.c
    extrachunk.c
    factory.c
    getframe.c
    icmstream.c
    tmpfile.c
    wavfile.c
    avifile_private.h)

add_library(avifil32 SHARED
    ${SOURCE}
    guid.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/avifil32.def)

set_module_type(avifil32 win32dll)
target_link_libraries(avifil32 wine)
add_importlibs(avifil32 msacm32 msvfw32 winmm ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(avifil32 avifile_private.h SOURCE)
add_cd_file(TARGET avifil32 DESTINATION reactos/system32 FOR all)
