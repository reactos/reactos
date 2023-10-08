
list(APPEND SOURCE
    mm/ARM3/contmem.c
    mm/ARM3/drvmgmt.c
    mm/ARM3/dynamic.c
    mm/ARM3/expool.c
    mm/ARM3/hypermap.c
    mm/ARM3/iosup.c
    mm/ARM3/kdbg.c
    mm/ARM3/largepag.c
    mm/ARM3/mdlsup.c
    mm/ARM3/mmdbg.c
    mm/ARM3/mminit.c
    mm/ARM3/mmsup.c
    mm/ARM3/ncache.c
    mm/ARM3/pagfault.c
    mm/ARM3/pfnlist.c
    mm/ARM3/pool.c
    mm/ARM3/procsup.c
    mm/ARM3/section.c
    mm/ARM3/session.c
    mm/ARM3/special.c
    mm/ARM3/sysldr.c
    mm/ARM3/syspte.c
    mm/ARM3/vadnode.c
    mm/ARM3/virtual.c
    mm/ARM3/wslist.cpp
    mm/ARM3/zeropage.c
    mm/balance.c
    mm/freelist.c
    mm/marea.c
    mm/mmfault.c
    mm/mminit.c
    mm/pagefile.c
    mm/region.c
    mm/rmap.c
    mm/section.c
    mm/shutdown.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        mm/i386/page.c
        mm/i386/procsup.c
        mm/ARM3/i386/init.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE
        mm/i386/page.c
        mm/amd64/init.c
        mm/amd64/procsup.c
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE
        mm/arm/page.c
        mm/ARM3/arm/init.c
    )
endif()
