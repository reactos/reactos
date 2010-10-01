
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xinput1_2 ${CMAKE_CURRENT_SOURCE_DIR}/xinput1_2.spec ${CMAKE_CURRENT_BINARY_DIR}/xinput1_2.def)

add_library(xinput1_2 SHARED xinput1_2_main.c version.rc)

set_module_type(xinput1_2 win32dll)
target_link_libraries(xinput1_2 ${CMAKE_CURRENT_BINARY_DIR}/xinput1_2.def)
add_dependencies(xinput1_2 xinput1_2_def psdk)
