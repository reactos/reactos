
spec2def(main.cpl main.spec)

list(APPEND SOURCE
    keyboard.c
    main.c
    mouse.c
    main.h)

add_library(main SHARED
    ${SOURCE}
    main.rc
    ${CMAKE_CURRENT_BINARY_DIR}/main.def)

set_module_type(main cpl UNICODE)
target_link_libraries(main uuid)

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

add_pch(main main.h SOURCE)
add_cd_file(TARGET main DESTINATION reactos/system32 FOR all)
