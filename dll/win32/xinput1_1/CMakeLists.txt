
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xinput1_1 ${CMAKE_CURRENT_SOURCE_DIR}/xinput1_1.spec ${CMAKE_CURRENT_BINARY_DIR}/xinput1_1.def)

add_library(xinput1_1 SHARED xinput1_1_main.c version.rc)

set_module_type(xinput1_1 win32dll)

target_link_libraries(xinput1_1 ${CMAKE_CURRENT_BINARY_DIR}/xinput1_1.def)
add_importlibs(xinput1_1 kernel32)
add_dependencies(xinput1_1 xinput1_1_def)
