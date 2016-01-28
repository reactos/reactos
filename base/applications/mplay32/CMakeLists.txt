
file(GLOB mplay32_rc_deps resources/*.*)
add_rc_deps(mplay32.rc ${mplay32_rc_deps})
add_executable(mplay32 mplay32.c mplay32.rc)
set_module_type(mplay32 win32gui UNICODE)
add_importlibs(mplay32 advapi32 comctl32 comdlg32 user32 gdi32 winmm shell32 msvcrt kernel32)
add_cd_file(TARGET mplay32 DESTINATION reactos/system32 FOR all)
