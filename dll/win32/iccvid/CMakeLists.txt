
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(iccvid.dll iccvid.spec)

list(APPEND SOURCE
    iccvid.c
    ${CMAKE_CURRENT_BINARY_DIR}/iccvid.def)

add_library(iccvid MODULE ${SOURCE} rsrc.rc)
set_module_type(iccvid win32dll)
target_link_libraries(iccvid wine)
add_importlibs(iccvid user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET iccvid DESTINATION reactos/system32 FOR all)
