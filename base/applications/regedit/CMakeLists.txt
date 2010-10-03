
set_unicode()

include_directories(BEFORE .)

list(APPEND SOURCE
    about.c
    childwnd.c
    edit.c
    find.c
    framewnd.c
    hexedit.c
    listview.c
    main.c
    regedit.c
    regproc.c
    security.c
    treeview.c
    regedit.rc)

add_pch(regedit ${CMAKE_CURRENT_SOURCE_DIR}/regedit.h ${SOURCE})

add_executable(regedit ${CMAKE_CURRENT_BINARY_DIR}/regedit_regedit.h.gch ${SOURCE})

set_module_type(regedit win32gui)

target_link_libraries(regedit uuid)

add_importlibs(regedit user32 gdi32 advapi32 ole32 shell32 comctl32 comdlg32 shlwapi msvcrt)

#add_subdirectory(clb)
