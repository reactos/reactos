
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mprapi.dll mprapi.spec)

list(APPEND SOURCE
    mprapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/mprapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mprapi.def)

add_library(mprapi MODULE ${SOURCE})
set_module_type(mprapi win32dll)
target_link_libraries(mprapi wine)
add_importlibs(mprapi msvcrt kernel32 ntdll)
add_cd_file(TARGET mprapi DESTINATION reactos/system32 FOR all)
