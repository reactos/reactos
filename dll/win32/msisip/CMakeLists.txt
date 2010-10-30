
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msisip.dll msisip.spec)

add_library(msisip SHARED main.c ${CMAKE_CURRENT_BINARY_DIR}/msisip.def)

set_module_type(msisip win32dll)

target_link_libraries(msisip wine)

add_importlibs(msisip crypt32 ole32 kernel32 ntdll)

add_cab_target(msisip 1)
