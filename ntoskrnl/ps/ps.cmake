
list(APPEND SOURCE
    ps/apphelp.c
    ps/debug.c
    ps/job.c
    ps/kill.c
    ps/process.c
    ps/psmgr.c
    ps/psnotify.c
    ps/query.c
    ps/quota.c
    ps/security.c
    ps/state.c
    ps/thread.c
    ps/win32.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        ps/i386/psctx.c
        ps/i386/psldt.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE
        ps/amd64/psctx.c
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE
        ps/arm/psctx.c
    )
endif()
