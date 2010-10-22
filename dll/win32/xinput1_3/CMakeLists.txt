
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(xinput1_3 ${CMAKE_CURRENT_SOURCE_DIR}/xinput1_3.spec ${CMAKE_CURRENT_BINARY_DIR}/xinput1_3.def)

add_library(xinput1_3 SHARED xinput1_3_main.c version.rc)

set_module_type(xinput1_3 win32dll)

target_link_libraries(xinput1_3
    ${CMAKE_CURRENT_BINARY_DIR}/xinput1_3.def
    wine)

add_importlibs(xinput1_3 msvcrt kernel32 ntdll)

add_dependencies(xinput1_3 xinput1_3_def)
add_livecd_target(xinput1_3 reactos/system32)
