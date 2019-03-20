
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(msvidc32.dll msvidc32.spec)

list(APPEND SOURCE
    msvideo1.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvidc32.def)

add_library(msvidc32 MODULE ${SOURCE} rsrc.rc)
set_module_type(msvidc32 win32dll)
target_link_libraries(msvidc32 wine)
add_importlibs(msvidc32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msvidc32 DESTINATION reactos/system32 FOR all)
