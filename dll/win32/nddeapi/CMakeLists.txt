
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(nddeapi.dll nddeapi.spec)

list(APPEND SOURCE
    nddeapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/nddeapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/nddeapi.def)

add_library(nddeapi MODULE ${SOURCE})
set_module_type(nddeapi win32dll)
target_link_libraries(nddeapi wine)
add_importlibs(nddeapi msvcrt kernel32 ntdll)
add_cd_file(TARGET nddeapi DESTINATION reactos/system32 FOR all)
