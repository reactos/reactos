
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(traffic.dll traffic.spec)

add_library(traffic SHARED
    traffic_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/traffic.def)

set_module_type(traffic win32dll)

target_link_libraries(traffic wine)

add_importlibs(traffic msvcrt kernel32 ntdll)
add_dependencies(traffic psdk buildno_header)
add_cab_target(traffic 1)
