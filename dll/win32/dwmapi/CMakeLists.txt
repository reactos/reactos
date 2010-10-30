
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dwmapi ${CMAKE_CURRENT_SOURCE_DIR}/dwmapi.spec ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def)

add_library(dwmapi SHARED dwmapi_main.c version.rc)

set_module_type(dwmapi win32dll)

target_link_libraries(dwmapi
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def
    wine)

add_importlibs(dwmapi kernel32 ntdll)
add_dependencies(dwmapi dwmapi_def)
add_cab_target(dwmapi 1)
