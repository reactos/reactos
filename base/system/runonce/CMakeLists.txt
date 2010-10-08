
set_unicode()

add_executable(runonce runonce.c runonce.rc)

set_module_type(runonce win32gui)

add_importlibs(runonce advapi32 user32 msvcrt kernel32 ntdll)
