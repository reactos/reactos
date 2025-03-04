
list(APPEND SOURCE
    main.c)

add_executable(updateclient ${SOURCE} updateclient.rc)
set_module_type(updateclient win32gui UNICODE)
add_importlibs(updateclient comdlg32 winmm shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET updateclient DESTINATION reactos/system32 FOR all)
