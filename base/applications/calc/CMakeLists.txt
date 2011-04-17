
set_unicode()
set_rc_compiler()

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
add_importlibs(calc advapi32 user32 gdi32 msvcrt kernel32)
if(MSVC)
add_importlibs(calc ntdll)
endif()

add_cab_target(calc 1)
