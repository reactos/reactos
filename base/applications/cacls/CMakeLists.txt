
add_executable(cacls 
    cacls.c
    cacls.rc)

set_module_type(cacls win32cui)

set_unicode(cacls yes)

add_importlibs(cacls advapi32 ntdll user32 shell32 msvcrt)

add_dependencies(cacls psdk)
