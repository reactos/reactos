
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(slbcsp ${CMAKE_CURRENT_SOURCE_DIR}/slbcsp.spec ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def)

add_library(slbcsp SHARED main.c)

set_module_type(slbcsp win32dll)

target_link_libraries(slbcsp
    ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def
    wine)

add_importlibs(slbcsp kernel32 ntdll)
add_dependencies(slbcsp slbcsp_def)
add_cab_target(slbcsp 1)
