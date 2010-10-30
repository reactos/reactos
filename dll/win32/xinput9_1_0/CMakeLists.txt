
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xinput9_1_0.dll xinput9_1_0.spec)

add_library(xinput9_1_0 SHARED xinput9_1_0_main.c version.rc ${CMAKE_CURRENT_BINARY_DIR}/xinput9_1_0.def)

add_importlibs(xinput9_1_0 kernel32)

set_module_type(xinput9_1_0 win32dll)

add_cab_target(xinput9_1_0 1)
