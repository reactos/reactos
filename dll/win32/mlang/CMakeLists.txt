
add_definitions(
    -D__WINESRC__
    -DCOM_NO_WINDOWS_H)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    mlang.c)

add_library(mlang MODULE ${SOURCE} mlang.rc)
spec2def(mlang mlang.dll mlang.spec ADD_IMPORTLIB)

set_module_type(mlang win32dll UNICODE)
target_link_libraries(mlang uuid wine)
add_delay_importlibs(mlang oleaut32)
add_importlibs(mlang gdi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mlang DESTINATION reactos/system32 FOR all)
