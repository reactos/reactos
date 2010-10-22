
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(httpapi ${CMAKE_CURRENT_SOURCE_DIR}/httpapi.spec ${CMAKE_CURRENT_BINARY_DIR}/httpapi.def)

add_library(httpapi SHARED httpapi_main.c)

set_module_type(httpapi win32dll)

target_link_libraries(httpapi
    ${CMAKE_CURRENT_BINARY_DIR}/httpapi.def
    wine)

add_importlibs(httpapi kernel32 ntdll)
add_dependencies(httpapi httpapi_def)
add_livecd_target(httpapi reactos/system32)
