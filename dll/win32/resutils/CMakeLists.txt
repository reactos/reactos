
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(resutils.dll resutils.spec)

add_library(resutils SHARED resutils.c ${CMAKE_CURRENT_BINARY_DIR}/resutils.def)

set_module_type(resutils win32dll)

target_link_libraries(resutils wine)

add_importlibs(resutils msvcrt kernel32 ntdll)

add_cab_target(resutils 1)
