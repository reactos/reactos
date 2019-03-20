
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(itircl.dll itircl.spec)

list(APPEND SOURCE
    itircl_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/itircl_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/itircl.def)

add_library(itircl MODULE ${SOURCE})
set_module_type(itircl win32dll)
target_link_libraries(itircl wine)
add_importlibs(itircl msvcrt kernel32 ntdll)
add_cd_file(TARGET itircl DESTINATION reactos/system32 FOR all)
