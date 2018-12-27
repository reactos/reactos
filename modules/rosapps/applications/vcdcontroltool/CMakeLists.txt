include_directories(${REACTOS_SOURCE_DIR}/modules/rosapps/drivers/vcdrom)

list(APPEND SOURCE
    vcdcontroltool.c)

add_executable(vcdcontroltool ${SOURCE} vcdcontroltool.rc)
set_module_type(vcdcontroltool win32gui UNICODE)
add_importlibs(vcdcontroltool advapi32 user32 gdi32 comctl32 comdlg32 ntdll msvcrt kernel32)
add_cd_file(TARGET vcdcontroltool DESTINATION reactos/system32 FOR all)
