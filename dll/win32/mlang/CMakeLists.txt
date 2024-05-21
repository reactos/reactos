
add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mlang.dll mlang.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    mlang.c
    ${CMAKE_CURRENT_BINARY_DIR}/mlang_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mlang.def)

add_library(mlang MODULE ${SOURCE} mlang.rc)
set_module_type(mlang win32dll UNICODE)
target_link_libraries(mlang uuid wine oldnames)
add_delay_importlibs(mlang oleaut32)
add_importlibs(mlang gdi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mlang DESTINATION reactos/system32 FOR all)
