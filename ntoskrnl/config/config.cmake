
list(APPEND SOURCE
    config/cmalloc.c
    config/cmapi.c
    config/cmboot.c
    config/cmcheck.c
    config/cmconfig.c
    config/cmcontrl.c
    config/cmdata.c
    config/cmdelay.c
    config/cmhook.c
    config/cmhvlist.c
    config/cminit.c
    config/cmkcbncb.c
    config/cmlazy.c
    config/cmmapvw.c
    config/cmnotify.c
    config/cmparse.c
    config/cmquota.c
    config/cmse.c
    config/cmsecach.c
    config/cmsysini.c
    config/cmvalche.c
    config/cmwraprs.c
    config/ntapi.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        config/i386/cmhardwr.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE
        config/i386/cmhardwr.c
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE
        config/arm/cmhardwr.c
    )
endif()
