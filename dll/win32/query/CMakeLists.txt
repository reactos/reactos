
add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    query_main.c)

add_library(query MODULE ${SOURCE})
spec2def(query query.dll query.spec)

set_module_type(query win32dll)
target_link_libraries(query wine)
add_importlibs(query msvcrt kernel32 ntdll)
add_cd_file(TARGET query DESTINATION reactos/system32 FOR all)
