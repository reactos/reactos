
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

spec2def(main ${CMAKE_CURRENT_SOURCE_DIR}/main.spec ${CMAKE_CURRENT_BINARY_DIR}/main.def)

add_library(main SHARED
    keyboard.c
    main.c
    mouse.c
    main.rc)

set_module_type(main cpl)

target_link_libraries(main ${CMAKE_CURRENT_BINARY_DIR}/main.def)

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

add_dependencies(main main_def)
add_livecd_target(main reactos/system32)
