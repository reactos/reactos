
add_executable(ssstars
    ssstars.c
    settings.c
    resource.rc)

set_module_type(ssstars win32gui UNICODE)
set_target_properties(ssstars PROPERTIES SUFFIX ".scr")
target_link_libraries(ssstars scrnsave)
add_importlibs(ssstars user32 gdi32 opengl32 glu32 advapi32 comctl32 shell32 msvcrt kernel32)
add_cd_file(TARGET ssstars DESTINATION reactos/system32 FOR all)
