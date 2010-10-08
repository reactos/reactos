
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(sensapi ${CMAKE_CURRENT_SOURCE_DIR}/sensapi.spec ${CMAKE_CURRENT_BINARY_DIR}/sensapi.def)

add_library(sensapi SHARED sensapi.c)

set_module_type(sensapi win32dll)

target_link_libraries(sensapi
    ${CMAKE_CURRENT_BINARY_DIR}/sensapi.def
    wine)

add_importlibs(sensapi kernel32 ntdll)
add_dependencies(sensapi sensapi_def)
