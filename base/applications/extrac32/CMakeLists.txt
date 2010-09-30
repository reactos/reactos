
add_executable(extrac32 extrac32.c)

set_unicode(extrac32 yes)

set_module_type(extrac32 win32gui)

target_link_libraries(extrac32 wine)

add_importlibs(extrac32 ntdll shell32 setupapi shlwapi user32 msvcrt)