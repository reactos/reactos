
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c)

add_library(snmpapi MODULE ${SOURCE} snmpapi.rc)
spec2def(snmpapi snmpapi.dll snmpapi.spec ADD_IMPORTLIB)

set_module_type(snmpapi win32dll)
target_link_libraries(snmpapi wine)
add_importlibs(snmpapi msvcrt kernel32_vista kernel32 ntdll)
add_cd_file(TARGET snmpapi DESTINATION reactos/system32 FOR all)
