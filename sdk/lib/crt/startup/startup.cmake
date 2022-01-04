
list(APPEND CRT_STARTUP_SOURCE
    startup/_matherr.c
    startup/crtexe.c
    startup/wcrtexe.c
    startup/crt_handler.c
    startup/crtdll.c
    startup/_newmode.c
    startup/wildcard.c
    startup/tlssup.c
    startup/mingw_helpers.c
    startup/natstart.c
    startup/charmax.c
    startup/atonexit.c
    startup/dllmain.c
    startup/pesect.c
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
    startup/reactos.c
)

if(MSVC)
    list(APPEND CRT_STARTUP_SOURCE
        startup/mscmain.c
        startup/threadSafeInit.c
    )
else()
    list(APPEND CRT_STARTUP_SOURCE
        startup/gccmain.c
        startup/pseudo-reloc.c
        startup/pseudo-reloc-list.c
    )
endif()
