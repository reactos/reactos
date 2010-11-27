
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(wtsapi32.dll wtsapi32.spec)

add_library(wtsapi32 SHARED wtsapi32.c ${CMAKE_CURRENT_BINARY_DIR}/wtsapi32.def)

set_module_type(wtsapi32 win32dll)

target_link_libraries(wtsapi32 wine)

add_importlibs(wtsapi32 advapi32 msvcrt kernel32 ntdll)

add_cab_target(wtsapi32 1)
