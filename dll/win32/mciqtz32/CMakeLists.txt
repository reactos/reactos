
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    mciqtz.c)

add_library(mciqtz32 MODULE ${SOURCE} mciavi_res.rc)
spec2def(mciqtz32 mciqtz32.dll mciqtz32.spec)

set_module_type(mciqtz32 win32dll)
target_link_libraries(mciqtz32 wine strmiids)
add_importlibs(mciqtz32 winmm ole32 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mciqtz32 DESTINATION reactos/system32 FOR all)
