
file(GLOB welcome_rc_deps res/*.*)
add_rc_deps(welcome.rc ${welcome_rc_deps})
add_executable(welcome welcome.c welcome.rc)
set_module_type(welcome win32gui UNICODE)
add_importlibs(welcome gdi32 user32 shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET welcome DESTINATION reactos FOR all)

# Welcome.exe renamed as Setup.exe for the BootCD
add_cd_file(TARGET welcome DESTINATION root NO_CAB NAME_ON_CD setup.exe FOR bootcd)
