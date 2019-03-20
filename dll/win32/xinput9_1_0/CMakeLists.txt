
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(xinput9_1_0.dll xinput9_1_0.spec)

list(APPEND SOURCE
    xinput9_1_0_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/xinput9_1_0.def)

add_library(xinput9_1_0 MODULE ${SOURCE} version.rc)
set_module_type(xinput9_1_0 win32dll)
add_importlibs(xinput9_1_0 msvcrt kernel32 xinput1_3)
add_cd_file(TARGET xinput9_1_0 DESTINATION reactos/system32 FOR all)
