
add_executable(softmodalmsgbox softmodalmsgbox.c softmodalmsgbox.rc)
set_module_type(softmodalmsgbox win32cui UNICODE)
add_importlibs(softmodalmsgbox user32 msvcrt kernel32)
add_rostests_file(TARGET softmodalmsgbox SUBDIR suppl)
