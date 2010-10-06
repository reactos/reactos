
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(msisip ${CMAKE_CURRENT_SOURCE_DIR}/msisip.spec ${CMAKE_CURRENT_BINARY_DIR}/msisip.def)

add_library(msisip SHARED main.c)

set_module_type(msisip win32dll)

target_link_libraries(msisip
    ${CMAKE_CURRENT_BINARY_DIR}/msisip.def
    wine)

add_importlibs(msisip
    crypt32
    ole32
    kernel32
    ntdll)

add_dependencies(msisip msisip_def)
