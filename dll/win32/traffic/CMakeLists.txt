
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(traffic ${CMAKE_CURRENT_SOURCE_DIR}/traffic.spec ${CMAKE_CURRENT_BINARY_DIR}/traffic.def)

add_library(traffic SHARED traffic_main.c)

set_module_type(traffic win32dll)

target_link_libraries(traffic
    ${CMAKE_CURRENT_BINARY_DIR}/traffic.def
    wine)

add_importlibs(traffic kernel32 ntdll)
add_dependencies(traffic traffic_def psdk buildno_header)
add_cab_target(traffic 1)
