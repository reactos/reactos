
add_definitions(-DDISABLE_HTMLHELP_SUPPORT=1)

list(APPEND SOURCE
    convert.c
    function.c
    rpn.c
    utl.c
    winmain.c
    calc.h)

file(GLOB calc_rc_deps res/*.*)
add_rc_deps(resource.rc ${calc_rc_deps})
add_executable(calc ${SOURCE} resource.rc)
set_module_type(calc win32gui UNICODE)
add_importlibs(calc advapi32 comctl32 user32 shell32 gdi32 msvcrt kernel32)

if(MSVC)
    add_importlibs(calc ntdll)
endif()

add_pch(calc calc.h SOURCE)
add_cd_file(TARGET calc DESTINATION reactos/system32 FOR all)
