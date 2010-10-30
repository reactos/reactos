
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(snmpapi.dll snmpapi.spec)

add_library(snmpapi SHARED main.c snmpapi.rc ${CMAKE_CURRENT_BINARY_DIR}/snmpapi.def)

set_module_type(snmpapi win32dll)

target_link_libraries(snmpapi wine)

add_importlibs(snmpapi kernel32 ntdll)

add_cab_target(snmpapi 1)
