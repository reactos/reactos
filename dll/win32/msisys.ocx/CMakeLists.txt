
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msisys.ocx msisys.ocx.spec)

add_library(msisys SHARED msisys.c
    ${CMAKE_CURRENT_BINARY_DIR}/msisys.def)

set_module_type(msisys win32ocx)

target_link_libraries(msisys
    uuid
    wine)

add_importlibs(msisys kernel32 ntdll)

