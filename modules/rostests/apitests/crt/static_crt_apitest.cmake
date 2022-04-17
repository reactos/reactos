
list(APPEND SOURCE_STATIC
    _snprintf.c
    _snwprintf.c
    _vscprintf.c
    _vscwprintf.c
    _vsnprintf.c
    _vsnwprintf.c
    atexit.c
    mbstowcs.c
    mbtowc.c
    sprintf.c
    strcpy.c
    strlen.c
    strtoul.c
    wcstombs.c
    wcstoul.c
    wctomb.c
)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE_STATIC
        # To be filled
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE_STATIC
        # To be filled
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE_STATIC
        __rt_div.c
        __fto64.c
        __64tof.c
    )
endif()

add_executable(static_crt_apitest EXCLUDE_FROM_ALL testlist.c ${SOURCE_STATIC})
target_compile_definitions(static_crt_apitest PRIVATE TEST_STATIC_CRT wine_dbgstr_an=wine_dbgstr_an_ wine_dbgstr_wn=wine_dbgstr_wn_)
target_link_libraries(static_crt_apitest crt wine ${PSEH_LIB})
set_module_type(static_crt_apitest win32cui)
add_importlibs(static_crt_apitest kernel32 ntdll)
add_rostests_file(TARGET static_crt_apitest)
