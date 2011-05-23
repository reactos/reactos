
include_directories(include/internal/mingw-w64)

if(NOT MSVC)
    add_definitions(-Wno-main)
endif()

list(APPEND MSVCRTEX_SOURCE
    startup/crtexe.c
    startup/wcrtexe.c
    startup/_newmode.c
    startup/wildcard.c
    startup/tlssup.c
    startup/mingw_helpers.c
    startup/natstart.c
    startup/charmax.c
    startup/merr.c
    startup/atonexit.c
    startup/txtmode.c
    startup/pseudo-reloc.c
    startup/tlsmcrt.c
    startup/tlsthrd.c
    startup/tlsmthread.c
    startup/cinitexe.c
    startup/gs_support.c
    startup/dll_argv.c
    startup/dllargv.c
    startup/wdllargv.c
    startup/crt0_c.c
    startup/crt0_w.c

    misc/fltused.c
)

if(MSVC)
    list(APPEND MSVCRTEX_SOURCE startup/mscmain.c)
else()
    list(APPEND MSVCRTEX_SOURCE startup/gccmain.c)
endif()

add_library(msvcrtex ${MSVCRTEX_SOURCE})
set_target_properties(msvcrtex PROPERTIES COMPILE_DEFINITIONS _M_CEE_PURE)

if(NOT MSVC)
    target_link_libraries(msvcrtex oldnames)
endif()

