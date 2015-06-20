
file(GLOB reactos_rc_deps res/*.*)
add_rc_deps(reactos.rc ${reactos_rc_deps})
add_executable(reactos reactos.c reactos.rc)
target_link_libraries(reactos uuid)
set_module_type(reactos win32gui UNICODE)
add_importlibs(reactos gdi32 user32 comctl32 setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET reactos DESTINATION reactos NO_CAB FOR bootcd)
