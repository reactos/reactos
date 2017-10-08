
list(APPEND SOURCE
    3dtext.c
    settings.c
    3dtext.h)

add_rc_deps(rsrc.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/icon_3dtext.ico)
add_executable(3dtext ${SOURCE} rsrc.rc)
set_module_type(3dtext win32gui UNICODE)
set_target_properties(3dtext PROPERTIES SUFFIX ".scr")
target_link_libraries(3dtext scrnsave)
add_importlibs(3dtext user32 gdi32 opengl32 glu32 advapi32 msvcrt kernel32)
add_pch(3dtext 3dtext.h SOURCE)
add_cd_file(TARGET 3dtext DESTINATION reactos/system32 FOR all)
