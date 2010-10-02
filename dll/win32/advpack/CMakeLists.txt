
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

list(APPEND SOURCE
    advpack.c
    files.c
    install.c
    reg.c)

spec2def(advpack ${CMAKE_CURRENT_SOURCE_DIR}/advpack.spec ${CMAKE_CURRENT_BINARY_DIR}/advpack.def)

add_library(advpack SHARED ${SOURCE})

set_module_type(advpack win32dll)

target_link_libraries(advpack
    ${CMAKE_CURRENT_BINARY_DIR}/advpack.def
    wine)

add_importlibs(advpack ole32 setupapi version advapi32 kernel32 ntdll)
add_dependencies(advpack advpack_def)
