
add_definitions(-DDISABLE_HTMLHELP_SUPPORT=1)

list(APPEND SOURCE
    about.c
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
add_importlibs(calc advapi32 user32 gdi32 msvcrt kernel32)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(calc mingwex)
elseif(MSVC)
    add_importlibs(calc ntdll)
endif()

add_pch(calc calc.h SOURCE)
add_cd_file(TARGET calc DESTINATION reactos/system32 FOR all)
