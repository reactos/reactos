
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xinput9_1_0 ${CMAKE_CURRENT_SOURCE_DIR}/xinput9_1_0.spec ${CMAKE_CURRENT_BINARY_DIR}/xinput9_1_0.def)

add_library(xinput9_1_0 SHARED xinput9_1_0_main.c version.rc)

set_module_type(xinput9_1_0 win32dll)
target_link_libraries(xinput9_1_0 ${CMAKE_CURRENT_BINARY_DIR}/xinput9_1_0.def)
add_dependencies(xinput9_1_0 xinput9_1_0_def psdk)
