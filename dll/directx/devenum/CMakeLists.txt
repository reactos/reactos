

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(devenum.dll devenum.spec)

add_library(devenum SHARED
    createdevenum.c
    devenum_main.c
    factory.c
    mediacatenum.c
    parsedisplayname.c
    devenum.rc
    ${CMAKE_CURRENT_BINARY_DIR}/devenum.def)

set_module_type(devenum win32dll UNICODE)

target_link_libraries(devenum
    strmiids
    uuid
    wine)

add_importlibs(devenum
    advapi32
    ole32
    oleaut32
    winmm
    user32
    avicap32
    msvcrt
    kernel32
    ntdll)

add_pch(devenum devenum_private.h)
add_cd_file(TARGET devenum DESTINATION reactos/system32 FOR all)
