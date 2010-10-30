
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-DCRTDLL)

spec2def(msvcrt20.dll msvcrt20.spec)

add_library(msvcrt20 SHARED msvcrt20.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20.def)

set_entrypoint(msvcrt20 0)

target_link_libraries(msvcrt20 wine)

add_importlibs(msvcrt20 msvcrt kernel32 ntdll)
add_dependencies(msvcrt20 psdk buildno_header)
add_cab_target(msvcrt20 1)
