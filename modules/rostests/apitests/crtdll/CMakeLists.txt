
list(APPEND SOURCE_CRTDLL
    ../crt/_mbsncmp.c
    ../crt/_mbsstr.c
    ../crt/setjmp.c
    ../crt/_snprintf.c
    ../crt/_snwprintf.c
    ../crt/_vsnprintf.c
    ../crt/_vsnwprintf.c
    ../crt/mbstowcs.c
    ../crt/mbtowc.c
    ../crt/sprintf.c
    ../crt/strcpy.c
    ../crt/strlen.c
    ../crt/strtoul.c
    ../crt/system.c
    ../crt/wcstombs.c
    ../crt/wcstoul.c
    ../crt/wctomb.c
)

add_executable(crtdll_apitest testlist.c ${SOURCE_CRTDLL})
target_compile_definitions(crtdll_apitest PRIVATE TEST_CRTDLL)
target_link_libraries(crtdll_apitest wine ${PSEH_LIB})
set_module_type(crtdll_apitest win32cui)
add_importlibs(crtdll_apitest crtdll msvcrt kernel32 ntdll)
add_rostests_file(TARGET crtdll_apitest)
