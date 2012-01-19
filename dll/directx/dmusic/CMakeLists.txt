

add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dmusic.dll dmusic.spec)

add_library(dmusic SHARED
    buffer.c
    clock.c
    collection.c
    dmusic.c
    dmusic_main.c
    download.c
    downloadedinstrument.c
    instrument.c
    port.c
    regsvr.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmusic.def)

set_module_type(dmusic win32dll UNICODE ENTRYPOINT 0)

target_link_libraries(dmusic
    dxguid
    uuid
    wine)

add_importlibs(dmusic
    user32
    advapi32
    ole32
    dsound
    kernel32
    ntdll)

add_dependencies(dmusic psdk)
add_pch(dmusic dmusic_private.h)
add_cd_file(TARGET dmusic DESTINATION reactos/system32 FOR all)
