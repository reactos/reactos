
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(olethk32.dll olethk32.spec)

add_library(olethk32 SHARED main.c version.rc ${CMAKE_CURRENT_BINARY_DIR}/olethk32.def)

set_module_type(olethk32 win32dll)

target_link_libraries(olethk32 wine)

add_importlibs(olethk32 msvcrt kernel32 ntdll)
add_dependencies(olethk32 psdk)
add_cab_target(olethk32 1)
