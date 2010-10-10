
set_unicode()

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msdmo ${CMAKE_CURRENT_SOURCE_DIR}/msdmo.spec ${CMAKE_CURRENT_BINARY_DIR}/msdmo.def)

add_library(msdmo SHARED
    dmoreg.c
    dmort.c
    rsrc.rc)

set_entrypoint(msdmo 0)

target_link_libraries(msdmo
    ${CMAKE_CURRENT_BINARY_DIR}/msdmo.def
    uuid
    wine)

add_importlibs(msdmo
    user32
    advapi32
    ole32
    kernel32
    ntdll)

add_dependencies(msdmo msdmo_def psdk buildno_header)
