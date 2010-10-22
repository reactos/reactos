
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(mssip32 ${CMAKE_CURRENT_SOURCE_DIR}/mssip32.spec ${CMAKE_CURRENT_BINARY_DIR}/mssip32.def)

add_library(mssip32 SHARED main.c)

set_module_type(mssip32 win32dll)

target_link_libraries(mssip32
    ${CMAKE_CURRENT_BINARY_DIR}/mssip32.def
    wine)

add_importlibs(mssip32 kernel32 ntdll)
add_dependencies(mssip32 mssip32_def)
add_livecd_target(mssip32 reactos/system32)
