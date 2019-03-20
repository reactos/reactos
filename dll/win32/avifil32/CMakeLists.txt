
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__ -DENTRY_PREFIX=avifil32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(avifil32.dll avifil32.spec ADD_IMPORTLIB)
add_rpcproxy_files(avifil32.idl)

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
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    precomp.h)

add_library(avifil32 MODULE
    ${SOURCE}
    guid.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/avifil32_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/avifil32.def)

set_module_type(avifil32 win32dll)
target_link_libraries(avifil32 wine ${PSEH_LIB})
add_importlibs(avifil32 msacm32 msvfw32 winmm ole32 user32 advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(avifil32 precomp.h SOURCE)
add_cd_file(TARGET avifil32 DESTINATION reactos/system32 FOR all)
