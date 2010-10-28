list(APPEND SOURCE
    dialog.c
    main.c
    settings.c
    text.c
    rsrc.rc)
    
    
set_rc_compiler()

add_pch(notepad ${CMAKE_CURRENT_SOURCE_DIR}/notepad.h ${SOURCE})
add_executable(notepad ${CMAKE_CURRENT_BINARY_DIR}/notepad_notepad.h.gch ${SOURCE})

set_module_type(notepad win32gui)

add_importlibs(notepad user32 gdi32 comctl32 comdlg32 advapi32 shell32 msvcrt kernel32)
