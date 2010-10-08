
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(snmpapi ${CMAKE_CURRENT_SOURCE_DIR}/snmpapi.spec ${CMAKE_CURRENT_BINARY_DIR}/snmpapi.def)

add_library(snmpapi SHARED main.c snmpapi.rc)

set_module_type(snmpapi win32dll)

target_link_libraries(snmpapi
    ${CMAKE_CURRENT_BINARY_DIR}/snmpapi.def
    wine)

add_importlibs(snmpapi kernel32 ntdll)
add_dependencies(snmpapi snmpapi_def)
