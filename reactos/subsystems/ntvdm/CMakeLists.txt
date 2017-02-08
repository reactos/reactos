
include_directories(.)

add_executable(ntvdm
    ntvdm.c
    ntvdm.rc)

set_module_type(ntvdm win32cui)
add_importlibs(ntvdm ntdll user32 gdi32 advapi32 msvcrt kernel32)
add_dependencies(ntvdm ndk bugcodes)
add_cd_file(TARGET ntvdm DESTINATION reactos/system32 FOR all)
