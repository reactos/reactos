
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    xinput1_3_main.c)

add_library(xinput1_3 MODULE ${SOURCE} version.rc)
spec2def(xinput1_3 xinput1_3.dll xinput1_3.spec ADD_IMPORTLIB)

set_module_type(xinput1_3 win32dll)
target_link_libraries(xinput1_3 wine)
add_importlibs(xinput1_3 msvcrt kernel32 ntdll)
add_cd_file(TARGET xinput1_3 DESTINATION reactos/system32 FOR all)
