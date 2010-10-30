
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(itircl.dll itircl.spec)

add_library(itircl SHARED itircl_main.c ${CMAKE_CURRENT_BINARY_DIR}/itircl.def)

set_module_type(itircl win32dll)

target_link_libraries(itircl wine)

add_importlibs(itircl kernel32 ntdll)
add_dependencies(itircl psdk)
add_cab_target(itircl 1)
