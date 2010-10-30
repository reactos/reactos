
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(inetmib1.dll inetmib1.spec)

add_library(inetmib1 SHARED main.c ${CMAKE_CURRENT_BINARY_DIR}/inetmib1.def)

set_module_type(inetmib1 win32dll)

target_link_libraries(inetmib1 wine)

add_importlibs(inetmib1
    msvcrt
    snmpapi
    iphlpapi
    kernel32
    ntdll)


add_cab_target(inetmib1 1)
