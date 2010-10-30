
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(inetmib1 ${CMAKE_CURRENT_SOURCE_DIR}/inetmib1.spec ${CMAKE_CURRENT_BINARY_DIR}/inetmib1.def)

add_library(inetmib1 SHARED main.c)

set_module_type(inetmib1 win32dll)

target_link_libraries(inetmib1
    ${CMAKE_CURRENT_BINARY_DIR}/inetmib1.def
    wine)

add_importlibs(inetmib1
    msvcrt
    snmpapi
    iphlpapi
    kernel32
    ntdll)

add_dependencies(inetmib1 inetmib1_def)
add_cab_target(inetmib1 1)
