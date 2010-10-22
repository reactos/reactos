
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-DCRTDLL)

spec2def(msvcrt40 ${CMAKE_CURRENT_SOURCE_DIR}/msvcrt40.spec ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def)

add_library(msvcrt40 SHARED msvcrt40.c)

set_module_type(msvcrt40 win32dll)

target_link_libraries(msvcrt40
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt40.def
    wine)

add_importlibs(msvcrt40 msvcrt kernel32 ntdll)
add_dependencies(msvcrt40 msvcrt40_def)
add_livecd_target(msvcrt40 reactos/system32)
