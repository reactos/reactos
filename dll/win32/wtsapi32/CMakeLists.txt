
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(wtsapi32 ${CMAKE_CURRENT_SOURCE_DIR}/wtsapi32.spec ${CMAKE_CURRENT_BINARY_DIR}/wtsapi32.def)

add_library(wtsapi32 SHARED wtsapi32.c)

set_module_type(wtsapi32 win32dll)

target_link_libraries(wtsapi32
    ${CMAKE_CURRENT_BINARY_DIR}/wtsapi32.def
    wine)

add_importlibs(wtsapi32 advapi32 kernel32 ntdll)
add_dependencies(wtsapi32 wtsapi32_def)
add_cab_target(wtsapi32 1)
