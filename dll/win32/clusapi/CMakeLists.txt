
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(clusapi ${CMAKE_CURRENT_SOURCE_DIR}/clusapi.spec ${CMAKE_CURRENT_BINARY_DIR}/clusapi.def)

add_library(clusapi SHARED clusapi.c)

set_module_type(clusapi win32dll)

target_link_libraries(clusapi
    ${CMAKE_CURRENT_BINARY_DIR}/clusapi.def
    wine)

add_importlibs(clusapi kernel32 ntdll)
add_dependencies(clusapi clusapi_def)
