
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mprapi ${CMAKE_CURRENT_SOURCE_DIR}/mprapi.spec ${CMAKE_CURRENT_BINARY_DIR}/mprapi.def)

add_library(mprapi SHARED mprapi.c)

set_module_type(mprapi win32dll)

target_link_libraries(mprapi
    ${CMAKE_CURRENT_BINARY_DIR}/mprapi.def
    wine)

add_importlibs(mprapi kernel32 ntdll)
add_dependencies(mprapi mprapi_def)
add_livecd_target(mprapi reactos/system32)
