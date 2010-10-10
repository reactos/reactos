
set_unicode()

add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dmusic ${CMAKE_CURRENT_SOURCE_DIR}/dmusic.spec ${CMAKE_CURRENT_BINARY_DIR}/dmusic.def)

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
    version.rc)

set_entrypoint(dmusic 0)

target_link_libraries(dmusic
    ${CMAKE_CURRENT_BINARY_DIR}/dmusic.def
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

add_dependencies(dmusic dmusic_def psdk buildno_header)
