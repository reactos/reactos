
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(itircl ${CMAKE_CURRENT_SOURCE_DIR}/itircl.spec ${CMAKE_CURRENT_BINARY_DIR}/itircl.def)

add_library(itircl SHARED itircl_main.c)

set_module_type(itircl win32dll)

target_link_libraries(itircl
    ${CMAKE_CURRENT_BINARY_DIR}/itircl.def
    wine)

add_importlibs(itircl kernel32 ntdll)
add_dependencies(itircl itircl_def psdk)
add_livecd_target(itircl reactos/system32)
