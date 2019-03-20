
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(xinput1_2.dll xinput1_2.spec)

list(APPEND SOURCE
    xinput1_2_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/xinput1_2.def)

add_library(xinput1_2 MODULE ${SOURCE} version.rc)
set_module_type(xinput1_2 win32dll)
add_importlibs(xinput1_2 xinput1_3 msvcrt kernel32)
add_cd_file(TARGET xinput1_2 DESTINATION reactos/system32 FOR all)
