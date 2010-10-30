
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cabinet ${CMAKE_CURRENT_SOURCE_DIR}/cabinet.spec ${CMAKE_CURRENT_BINARY_DIR}/cabinet.def)

list(APPEND SOURCE
    cabinet_main.c
    fci.c
    fdi.c
    cabinet.rc)

add_library(cabinet SHARED ${SOURCE})

set_entrypoint(cabinet 0)

target_link_libraries(cabinet
    ${CMAKE_CURRENT_BINARY_DIR}/cabinet.def
    wine)

add_importlibs(cabinet kernel32 ntdll)
add_dependencies(cabinet cabinet_def psdk buildno_header)
add_cab_target(cabinet 1)