
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xinput1_1.dll xinput1_1.spec)

add_library(xinput1_1 SHARED xinput1_1_main.c version.rc ${CMAKE_CURRENT_BINARY_DIR}/xinput1_1.def)

set_module_type(xinput1_1 win32dll)

add_importlibs(xinput1_1 kernel32)

add_cab_target(xinput1_1 1)
