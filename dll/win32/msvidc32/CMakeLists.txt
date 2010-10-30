
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

spec2def(msvidc32 ${CMAKE_CURRENT_SOURCE_DIR}/msvidc32.spec ${CMAKE_CURRENT_BINARY_DIR}/msvidc32.def)

add_library(msvidc32 SHARED msvideo1.c rsrc.rc)

set_module_type(msvidc32 win32dll)

target_link_libraries(msvidc32
    ${CMAKE_CURRENT_BINARY_DIR}/msvidc32.def
    wine)

add_importlibs(msvidc32 user32 kernel32 ntdll)
add_dependencies(msvidc32 msvidc32_def)
add_cab_target(msvidc32 1)
