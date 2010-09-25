
add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DDISABLE_HTMLHELP_SUPPORT=1)

add_executable(calc 
    about.c
    convert.c
    function.c
    rpn.c
    utl.c
    winmain.c
    resource.rc)

set_module_type(calc win32gui)
set_unicode(calc yes)
add_importlibs(calc advapi32 user32 gdi32 msvcrt)

add_dependencies(calc psdk)
