
include_directories(.)

set_rc_compiler()

add_executable(ntvdm
    ntvdm.c
    ntvdm.rc)

set_module_type(ntvdm win32cui)

target_link_libraries(ntvdm
    mingw_main
    mingw_common)

add_importlibs(ntvdm ntdll user32 gdi32 advapi32 kernel32 msvcrt)
add_dependencies(ntvdm ndk bugcodes)

add_cd_file(TARGET ntvdm DESTINATION reactos/system32 FOR all)