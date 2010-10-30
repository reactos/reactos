
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dciman32.dll dciman32.spec)

add_library(dciman32 SHARED dciman_main.c ${CMAKE_CURRENT_BINARY_DIR}/dciman32.def)

set_module_type(dciman32 win32dll)

target_link_libraries(dciman32 wine)

add_importlibs(dciman32 kernel32 ntdll)

add_cab_target(dciman32 1)
