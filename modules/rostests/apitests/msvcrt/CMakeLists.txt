
add_subdirectory(CmdLineUtil)

# Disable builtins, because we want to test the exports, not the compiler's builtins.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fno-builtin)
endif()

list(APPEND SOURCE_CRT_TESTS
    ../crt/fpcontrol.c
    ../crt/_finite.c
    ../crt/_isnan.c
    ../crt/_mbsncmp.c
    ../crt/_mbsstr.c
    ../crt/_snprintf.c
    ../crt/_snwprintf.c
    ../crt/_vscprintf.c
    ../crt/_vscwprintf.c
    ../crt/_vsnprintf.c
    ../crt/_vsnwprintf.c
    ../crt/_wsystem.c
    ../crt/acos.c
    ../crt/asin.c
    ../crt/atan.c
    ../crt/atexit.c
    ../crt/ceil.c
    ../crt/cos.c
    ../crt/crtdata.c
    ../crt/exp.c
    ../crt/fabs.c
    ../crt/floor.c
    ../crt/log.c
    ../crt/log10.c
    ../crt/mbstowcs.c
    ../crt/mbtowc.c
    ../crt/rand_s.c
    ../crt/setjmp.c
    ../crt/sin.c
    ../crt/sqrt.c
    ../crt/sprintf.c
    ../crt/static_construct.cpp
    ../crt/static_init.c
    ../crt/strcpy.c
    ../crt/strlen.c
    ../crt/strtoul.c
    ../crt/system.c
    ../crt/tan.c
    ../crt/wcstombs.c
    ../crt/wcstoul.c
    ../crt/wctomb.c
)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE_CRT_TESTS
        ../crt/__getmainargs.c # FIXME: Moved here because it doesn't work on x64
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE_CRT_TESTS
        ../crt/__rt_div.c
        ../crt/__fto64.c
        ../crt/__64tof.c
    )
endif()

list(APPEND SOURCE
    ${SOURCE_CRT_TESTS}
    CommandLine.c
    ieee.c
    popen.c
    splitpath.c
    testlist.c)

add_executable(msvcrt_apitest ${SOURCE})
target_compile_definitions(msvcrt_apitest PRIVATE TEST_MSVCRT)
target_link_libraries(msvcrt_apitest wine ${PSEH_LIB})
set_module_type(msvcrt_apitest win32cui)
add_importlibs(msvcrt_apitest msvcrt kernel32 ntdll)
add_rostests_file(TARGET msvcrt_apitest)

# Helper DLL for testing CRT startup
spec2def(msvcrt_crt_dll_startup.dll ../crt/dll_startup.spec)
add_library(msvcrt_crt_dll_startup MODULE
    ../crt/dll_startup.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_crt_dll_startup.def)
set_module_type(msvcrt_crt_dll_startup win32dll)
add_importlibs(msvcrt_crt_dll_startup msvcrt kernel32 ntdll)
add_rostests_file(TARGET msvcrt_crt_dll_startup)
