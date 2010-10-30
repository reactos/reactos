
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(avifil32.dll avifil32.spec)

add_library(avifil32 SHARED
    acmstream.c
    api.c
    avifile.c
    editstream.c
    extrachunk.c
    factory.c
    getframe.c
    icmstream.c
    regsvr.c
    tmpfile.c
    wavfile.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/avifil32.def)

set_module_type(avifil32 win32dll)

target_link_libraries(avifil32
    uuid
    wine)

add_importlibs(avifil32
    msvcrt
    msacm32
    msvfw32
    winmm
    ole32
    user32
    advapi32
    kernel32
    ntdll)


add_cab_target(avifil32 1)
