
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dwmapi.dll dwmapi.spec)

add_library(dwmapi SHARED dwmapi_main.c version.rc ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def)

set_module_type(dwmapi win32dll)

target_link_libraries(dwmapi wine)

add_importlibs(dwmapi kernel32 ntdll)

add_cab_target(dwmapi 1)
