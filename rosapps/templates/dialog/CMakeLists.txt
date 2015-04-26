
list(APPEND SOURCE
    dialog.c
    memdlg.c
    page1.c
    page2.c
    page3.c
    trace.c)

add_executable(template_dialog ${SOURCE} dialog.rc)
set_module_type(template_dialog win32cui)
add_target_compile_flags(template_dialog "-Wno-unused-but-set-variable")
add_importlibs(template_dialog user32 gdi32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET template_dialog DESTINATION reactos/system32 FOR all)
