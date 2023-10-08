
list(APPEND SOURCE
    ex/atom.c
    ex/callback.c
    ex/dbgctrl.c
    ex/efi.c
    ex/event.c
    ex/evtpair.c
    ex/exintrin.c
    ex/fmutex.c
    ex/handle.c
    ex/harderr.c
    ex/hdlsterm.c
    ex/init.c
    ex/interlocked.c
    ex/keyedevt.c
    ex/locale.c
    ex/lookas.c
    ex/mutant.c
    ex/profile.c
    ex/pushlock.c
    ex/resource.c
    ex/rundown.c
    ex/sem.c
    ex/shutdown.c
    ex/sysinfo.c
    ex/time.c
    ex/timer.c
    ex/uuid.c
    ex/win32k.c
    ex/work.c
    ex/xipdisp.c
    ex/zone.c
    )

list(APPEND ASM_SOURCE ex/zw.S)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        ex/i386/fastinterlck_asm.S
        ex/i386/ioport.S
    )
elseif(ARCH STREQUAL "amd64")
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        ex/arm/ioport.s
    )
endif()
