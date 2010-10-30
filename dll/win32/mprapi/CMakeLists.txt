
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mprapi.dll mprapi.spec)

add_library(mprapi SHARED mprapi.c ${CMAKE_CURRENT_BINARY_DIR}/mprapi.def)

set_module_type(mprapi win32dll)

target_link_libraries(mprapi wine)

add_importlibs(mprapi kernel32 ntdll)

add_cab_target(mprapi 1)
