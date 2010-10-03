
set_unicode()

add_executable(kbswitch kbswitch.c kbswitch.rc)

set_module_type(kbswitch win32gui)
add_importlibs(kbswitch advapi32 user32 shell32 gdi32 msvcrt)

add_subdirectory(kbsdll)
