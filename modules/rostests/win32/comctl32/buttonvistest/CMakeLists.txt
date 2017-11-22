add_executable(buttonvistest buttonvistest.cpp buttonvistest.rc)
set_module_type(buttonvistest win32gui UNICODE)
add_delay_importlibs(buttonvistest comctl32)
add_importlibs(buttonvistest uxtheme user32 gdi32 advapi32 msvcrt kernel32)
