
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(ntprint.dll ntprint.spec)

add_library(ntprint SHARED ntprint.c ntprint.rc ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def)

set_module_type(ntprint win32dll)

target_link_libraries(ntprint wine)

add_importlibs(ntprint winspool msvcrt kernel32 ntdll)
add_dependencies(ntprint psdk)
add_cab_target(ntprint 1)
