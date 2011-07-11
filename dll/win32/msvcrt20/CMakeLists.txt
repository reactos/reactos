
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-DCRTDLL)

spec2def(msvcrt20.dll msvcrt20.spec)

list(APPEND SOURCE
    msvcrt20.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt20.def)

add_library(msvcrt20 SHARED ${SOURCE})
set_entrypoint(msvcrt20 0)

target_link_libraries(msvcrt20 wine)

add_importlibs(msvcrt20 msvcrt kernel32 ntdll)
add_dependencies(msvcrt20 psdk)
add_cd_file(TARGET msvcrt20 DESTINATION reactos/system32 FOR all)
