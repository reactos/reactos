
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(httpapi.dll httpapi.spec)

add_library(httpapi SHARED httpapi_main.c ${CMAKE_CURRENT_BINARY_DIR}/httpapi.def)

set_module_type(httpapi win32dll)

target_link_libraries(httpapi wine)

add_importlibs(httpapi kernel32 ntdll)

add_cab_target(httpapi 1)
