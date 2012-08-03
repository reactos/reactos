
set_rc_compiler()
set_cpp()

add_executable(mmcclient 
    main.cpp
    register_functions.cpp
    snapin.cpp
    fonction.cpp
    MainWindow.cpp
    MMC_Parser.cpp
    ressource.rc)


target_link_libraries(mmcclient uuid libxml2 mmclib)
allow_warnings(mmcclient)
set_module_type(mmcclient win32gui UNICODE)
add_importlibs(mmcclient user32 gdi32 comdlg32 advapi32 Mmcndmgr shell32 ws2_32 comctl32 msvcrt kernel32 ole32)
add_pch(mmcclient precomp.h)
add_cd_file(TARGET mmcclient DESTINATION reactos/system32 FOR all)