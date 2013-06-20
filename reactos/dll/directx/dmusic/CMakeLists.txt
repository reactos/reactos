
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dmusic.dll dmusic.spec)

list(APPEND SOURCE
    buffer.c
    clock.c
    collection.c
    dmusic.c
    dmusic_main.c
    download.c
    instrument.c
    port.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmusic.def)

add_library(dmusic SHARED ${SOURCE})
set_module_type(dmusic win32dll UNICODE)
target_link_libraries(dmusic dxguid uuid wine)
add_importlibs(dmusic ole32 advapi32 winmm msvcrt kernel32 ntdll)
add_dependencies(dmusic psdk)
add_pch(dmusic dmusic_private.h)
add_cd_file(TARGET dmusic DESTINATION reactos/system32 FOR all)
