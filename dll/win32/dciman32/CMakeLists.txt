
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dciman32 ${CMAKE_CURRENT_SOURCE_DIR}/dciman32.spec ${CMAKE_CURRENT_BINARY_DIR}/dciman32.def)

add_library(dciman32 SHARED dciman_main.c)

set_module_type(dciman32 win32dll)

target_link_libraries(dciman32
    ${CMAKE_CURRENT_BINARY_DIR}/dciman32.def
    wine)

add_importlibs(dciman32 kernel32 ntdll)
add_dependencies(dciman32 dciman32_def)
add_livecd_target(dciman32 reactos/system32)
