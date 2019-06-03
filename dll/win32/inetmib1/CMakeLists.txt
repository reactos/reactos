
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c)

add_library(inetmib1 MODULE ${SOURCE})
spec2def(inetmib1 inetmib1.dll inetmib1.spec)

set_module_type(inetmib1 win32dll)
target_link_libraries(inetmib1 wine)
add_delay_importlibs(inetmib1 iphlpapi)
add_importlibs(inetmib1 snmpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET inetmib1 DESTINATION reactos/system32 FOR all)
