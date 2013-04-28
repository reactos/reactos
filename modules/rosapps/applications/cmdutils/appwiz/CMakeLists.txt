
add_executable(appwiz.exe appwiz.c appwiz.rc)
set_module_type(appwiz.exe win32cui)
set_target_properties(appwiz.exe PROPERTIES SUFFIX "")
add_importlibs(appwiz.exe advapi32 user32 shell32 msvcrt kernel32)
add_cd_file(TARGET appwiz.exe DESTINATION reactos/system32 FOR all)
