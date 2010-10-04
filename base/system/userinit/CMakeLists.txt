
set_unicode()
include_directories(.)
set_rc_compiler()

add_executable(userinit userinit.c userinit.rc)

set_module_type(userinit win32gui)

add_importlibs(userinit user32 gdi32 advapi32 shell32 shlwapi msvcrt)
target_link_libraries(userinit wine)
