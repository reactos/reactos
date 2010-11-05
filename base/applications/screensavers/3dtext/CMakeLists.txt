
set_rc_compiler()
set_unicode()

add_executable(3dtext
    3dtext.c
    settings.c
    rsrc.rc)

set_module_type(3dtext win32gui)
set_target_properties(3dtext PROPERTIES SUFFIX ".scr")

target_link_libraries(3dtext scrnsave)
add_importlibs(3dtext user32 gdi32 opengl32 glu32 advapi32 msvcrt kernel32)

add_cab_target(3dtext 1)