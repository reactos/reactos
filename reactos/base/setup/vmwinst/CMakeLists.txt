
file(GLOB vmwinst_rc_deps resources/*.*)
add_rc_deps(vmwinst.rc ${vmwinst_rc_deps})
add_executable(vmwinst vmwinst.c vmwinst.rc)
target_link_libraries(vmwinst ${PSEH_LIB})
set_module_type(vmwinst win32gui UNICODE)
add_importlibs(vmwinst advapi32 comctl32 comdlg32 newdev user32 setupapi shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET vmwinst DESTINATION reactos/system32 FOR all)
