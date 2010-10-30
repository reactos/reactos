
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(main.cpl main.spec)

add_library(main SHARED
    keyboard.c
    main.c
    mouse.c
    main.rc
    ${CMAKE_CURRENT_BINARY_DIR}/main.def)

set_module_type(main cpl)

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

add_cab_target(main 1)
