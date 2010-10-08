
set_unicode()

add_executable(mplay32 mplay32.c mplay32.rc)

set_module_type(mplay32 win32gui)

add_importlibs(mplay32 advapi32 comctl32 comdlg32 user32 gdi32 winmm shell32 msvcrt kernel32)
