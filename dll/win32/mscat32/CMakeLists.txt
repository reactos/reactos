
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mscat32 ${CMAKE_CURRENT_SOURCE_DIR}/mscat32.spec ${CMAKE_CURRENT_BINARY_DIR}/mscat32.def)

add_library(mscat32 SHARED main.c)

set_module_type(mscat32 win32dll)

target_link_libraries(mscat32
    ${CMAKE_CURRENT_BINARY_DIR}/mscat32.def
    wine)

add_importlibs(mscat32 wintrust kernel32 ntdll)
add_dependencies(mscat32 mscat32_def)
add_livecd_target(mscat32 reactos/system32)
