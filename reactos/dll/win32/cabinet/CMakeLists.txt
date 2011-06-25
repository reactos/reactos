
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(cabinet.dll cabinet.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cabinet_main.c
    fci.c
    fdi.c
    stubs.c
    cabinet.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cabinet.def)

add_library(cabinet SHARED ${SOURCE})

set_entrypoint(cabinet 0)

target_link_libraries(cabinet wine)

add_importlibs(cabinet kernel32 ntdll)
add_importlib_target(cabinet.spec)

add_dependencies(cabinet psdk)
add_cd_file(TARGET cabinet DESTINATION reactos/system32 FOR all)
