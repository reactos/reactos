
list(APPEND SOURCE
    CloseThemeData.c
    DrawThemeParentBackground.c
    GetThemeParseErrorInfo.c
    SetWindowTheme.c
    SetThemeAppProperties.c
    ../include/msgtrace.c
    testlist.c)

add_executable(uxtheme_apitest ${SOURCE})
target_link_libraries(uxtheme_apitest wine ${PSEH_LIB})
set_module_type(uxtheme_apitest win32cui)
add_importlibs(uxtheme_apitest uxtheme comctl32 user32 msvcrt kernel32)
add_rostests_file(TARGET uxtheme_apitest)
