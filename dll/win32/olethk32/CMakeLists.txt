
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(olethk32 ${CMAKE_CURRENT_SOURCE_DIR}/olethk32.spec ${CMAKE_CURRENT_BINARY_DIR}/olethk32.def)

add_library(olethk32 SHARED main.c version.rc)

set_module_type(olethk32 win32dll)

target_link_libraries(olethk32
    ${CMAKE_CURRENT_BINARY_DIR}/olethk32.def
    wine)

add_importlibs(olethk32 kernel32 ntdll)
add_dependencies(olethk32 olethk32_def psdk)
add_cab_target(olethk32 1)
