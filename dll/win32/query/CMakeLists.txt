
add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(query.dll query.spec)

list(APPEND SOURCE
    query_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/query_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/query.def)

add_library(query MODULE ${SOURCE})
set_module_type(query win32dll)
target_link_libraries(query wine)
add_importlibs(query msvcrt kernel32 ntdll)
add_cd_file(TARGET query DESTINATION reactos/system32 FOR all)
