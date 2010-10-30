
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msisys ${CMAKE_CURRENT_SOURCE_DIR}/msisys.ocx.spec ${CMAKE_CURRENT_BINARY_DIR}/msisys.def)

add_library(msisys SHARED msisys.c)

set_module_type(msisys win32ocx)

target_link_libraries(msisys
    ${CMAKE_CURRENT_BINARY_DIR}/msisys.def
    uuid
    wine)

add_importlibs(msisys kernel32 ntdll)

