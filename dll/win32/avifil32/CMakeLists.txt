
add_definitions(
    -D__WINESRC__
    -D_DLL -D__USE_CRTIMP)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

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

target_link_libraries(avifil32 uuid wine)

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

add_importlib_target(avifil32.spec)
add_cd_file(TARGET avifil32 DESTINATION reactos/system32 FOR all)
