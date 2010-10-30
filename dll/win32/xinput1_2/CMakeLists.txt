
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xinput1_2.dll xinput1_2.spec)

add_library(xinput1_2 SHARED xinput1_2_main.c version.rc ${CMAKE_CURRENT_BINARY_DIR}/xinput1_2.def)

add_importlibs(xinput1_2 kernel32)

set_module_type(xinput1_2 win32dll)

add_cab_target(xinput1_2 1)
