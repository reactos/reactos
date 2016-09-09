
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(clusapi.dll clusapi.spec)

list(APPEND SOURCE
    clusapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/clusapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/clusapi.def)

add_library(clusapi SHARED ${SOURCE})
set_module_type(clusapi win32dll)
target_link_libraries(clusapi wine)
add_importlibs(clusapi msvcrt kernel32 ntdll)
add_cd_file(TARGET clusapi DESTINATION reactos/system32 FOR all)
