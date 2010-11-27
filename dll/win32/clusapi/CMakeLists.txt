
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(clusapi.dll clusapi.spec)

add_library(clusapi SHARED clusapi.c ${CMAKE_CURRENT_BINARY_DIR}/clusapi.def)

set_module_type(clusapi win32dll)

target_link_libraries(clusapi wine)

add_importlibs(clusapi msvcrt kernel32 ntdll)

add_cab_target(clusapi 1)
