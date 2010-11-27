
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(mssip32.dll mssip32.spec)

add_library(mssip32 SHARED main.c ${CMAKE_CURRENT_BINARY_DIR}/mssip32.def)

set_module_type(mssip32 win32dll)

target_link_libraries(mssip32 wine)

add_importlibs(mssip32 msvcrt kernel32 ntdll)

add_cab_target(mssip32 1)
