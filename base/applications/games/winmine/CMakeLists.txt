
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set_rc_compiler()

add_executable(winmine
    main.c
    dialog.c
    rsrc.rc)

set_module_type(winmine win32gui)
target_link_libraries(winmine wine)
add_importlibs(winmine user32 gdi32 advapi32 shell32 msvcrt kernel32 ntdll)
