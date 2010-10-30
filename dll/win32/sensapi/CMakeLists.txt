
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(sensapi.dll sensapi.spec)

add_library(sensapi SHARED sensapi.c ${CMAKE_CURRENT_BINARY_DIR}/sensapi.def)

set_module_type(sensapi win32dll)

target_link_libraries(sensapi wine)

add_importlibs(sensapi kernel32 ntdll)

add_cab_target(sensapi 1)
