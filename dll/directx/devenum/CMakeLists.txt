
set_unicode()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(devenum ${CMAKE_CURRENT_SOURCE_DIR}/devenum.spec ${CMAKE_CURRENT_BINARY_DIR}/devenum.def)

add_library(devenum SHARED
    createdevenum.c
    devenum_main.c
    factory.c
    mediacatenum.c
    parsedisplayname.c
    devenum.rc)

set_module_type(devenum win32dll)

target_link_libraries(devenum
    ${CMAKE_CURRENT_BINARY_DIR}/devenum.def
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
    kernel32
    ntdll)

add_dependencies(devenum devenum_def)
add_livecd_target(devenum reactos/system32)
