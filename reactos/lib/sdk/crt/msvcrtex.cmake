
include_directories(include/internal/mingw-w64)

if(NOT MSVC)
    add_definitions(-Wno-main)
endif()

list(APPEND MSVCRTEX_SOURCE
    startup/crtexe.c
    startup/wcrtexe.c
    startup/crtdll.c
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
    startup/pseudo-reloc-list.c
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
    startup/dllentry.c
    misc/fltused.c
    misc/ofmt_stub.c
)

if(ARCH MATCHES i386)
list(APPEND MSVCRTEX_SOURCE
    except/i386/chkstk_asm.s
    math/i386/ci.c
    math/i386/ftol2_asm.S
    math/i386/alldiv_asm.s
)
endif()

if(MSVC)
    list(APPEND MSVCRTEX_SOURCE startup/mscmain.c)
else()
    list(APPEND MSVCRTEX_SOURCE startup/gccmain.c)
endif()

add_library(msvcrtex ${MSVCRTEX_SOURCE})
set_target_properties(msvcrtex PROPERTIES COMPILE_DEFINITIONS _M_CEE_PURE)
set_source_files_properties(startup/crtdll.c PROPERTIES COMPILE_DEFINITIONS CRTDLL)

if(NOT MSVC)
    target_link_libraries(msvcrtex chkstk oldnames)
endif()

add_dependencies(msvcrtex psdk)
