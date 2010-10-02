
set_unicode()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set_rc_compiler()

add_executable(winemine
    main.c
    dialog.c
    rsrc.rc)

set_module_type(winemine win32gui)
add_importlibs(winemine user32 gdi32 advapi32 shell32 msvcrt)
