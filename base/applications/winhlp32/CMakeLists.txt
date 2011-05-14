
set_rc_compiler()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(
    -D__ROS_LONG64__
    -D_CRT_NONSTDC_NO_DEPRECATE
    -Dfileno=_fileno
    -Disatty=_isatty
    -D_DLL -D__USE_CRTIMP)

add_executable(winhlp32
    callback.c
    hlpfile.c
    macro.c
    string.c
    winhelp.c
    lex.yy.c
    rsrc.rc)

set_module_type(winhlp32 win32gui)

target_link_libraries(winhlp32 wine)
add_importlibs(winhlp32 user32 gdi32 shell32 comctl32 comdlg32 msvcrt kernel32 ntdll)
add_cd_file(TARGET winhlp32 DESTINATION reactos)
