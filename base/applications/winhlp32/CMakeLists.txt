
add_definitions(
    -D__ROS_LONG64__
    -Dstrcasecmp=_stricmp
)

list(APPEND SOURCE
    callback.c
    hlpfile.c
    macro.c
    winhelp.c)

FLEX_TARGET(macro_scanner macro.lex.l ${CMAKE_CURRENT_BINARY_DIR}/macro.lex.yy.c)

list(APPEND PCH_SKIP_SOURCE
    ${FLEX_macro_scanner_OUTPUTS}
    string.c)

add_rc_deps(rsrc.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/winhelp.ico)

add_executable(winhlp32
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    rsrc.rc)

set_module_type(winhlp32 win32gui)
target_link_libraries(winhlp32 wine oldnames)
add_importlibs(winhlp32 user32 gdi32 shell32 comctl32 comdlg32 msvcrt kernel32 ntdll)
add_pch(winhlp32 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET winhlp32 DESTINATION reactos FOR all)
set_wine_module_FIXME(winhlp32) # CORE-5743: No ARRAY_SIZE macro
