
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(dciman32.dll dciman32.spec)

list(APPEND SOURCE
    dciman_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dciman32.def)

add_library(dciman32 SHARED ${SOURCE})

set_module_type(dciman32 win32dll)

target_link_libraries(dciman32 wine)

add_importlibs(dciman32 msvcrt kernel32 ntdll)

add_cab_target(dciman32 1)
