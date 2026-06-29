
include_directories(BEFORE .)

if(DBG)
    add_definitions(-D_DEBUG=1)
endif()

list(APPEND SOURCE
    about.c
    childwnd.c
    edit.c
    error.c
    find.c
    framewnd.c
    hexedit.c
    listview.c
    main.c
    regedit.c
    regproc.c
    security.c
    settings.c
    treeview.c
    txtproc.c
    regedit.h)

file(GLOB regedit_rc_deps res/*.*)
add_rc_deps(regedit.rc ${regedit_rc_deps})
add_executable(regedit ${SOURCE} regedit.rc)
set_module_type(regedit win32gui UNICODE)
target_compile_definitions(regedit PRIVATE _CRT_NON_CONFORMING_SWPRINTFS)
target_link_libraries(regedit uuid wine)
add_importlibs(regedit user32 gdi32 advapi32 ole32 shell32 comctl32 comdlg32 shlwapi msvcrt kernel32 ntdll)
add_pch(regedit regedit.h SOURCE)
add_cd_file(TARGET regedit DESTINATION reactos FOR all)
add_subdirectory(clb)
