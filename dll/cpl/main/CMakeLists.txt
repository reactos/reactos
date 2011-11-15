
set_rc_compiler()

spec2def(main.cpl main.spec)

add_library(main SHARED
    keyboard.c
    main.c
    mouse.c
    main.rc
    ${CMAKE_CURRENT_BINARY_DIR}/main.def)

set_module_type(main cpl UNICODE)

add_importlibs(main
    msvcrt
    advapi32
    user32
    comctl32
    devmgr
    comdlg32
    shell32
    gdi32
    kernel32)

add_pch(main main.h)
add_cd_file(TARGET main DESTINATION reactos/system32 FOR all)
