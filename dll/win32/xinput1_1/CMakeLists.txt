
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(xinput1_1.dll xinput1_1.spec)

list(APPEND SOURCE
    xinput1_1_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/xinput1_1.def)

add_library(xinput1_1 MODULE ${SOURCE} version.rc)
set_module_type(xinput1_1 win32dll)
add_importlibs(xinput1_1 xinput1_3 msvcrt kernel32)
add_cd_file(TARGET xinput1_1 DESTINATION reactos/system32 FOR all)
