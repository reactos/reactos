
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-DCRTDLL)

spec2def(msvcrt40.dll msvcrt40.spec)

list(APPEND SOURCE
    msvcrt40.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def)

add_library(msvcrt40 SHARED ${SOURCE})

set_module_type(msvcrt40 win32dll)

target_link_libraries(msvcrt40 wine)

add_importlibs(msvcrt40 msvcrt kernel32 ntdll)

add_cd_file(TARGET msvcrt40 DESTINATION reactos/system32 FOR all)
