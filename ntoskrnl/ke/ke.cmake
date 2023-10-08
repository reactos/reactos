
list(APPEND SOURCE
    ke/apc.c
    ke/balmgr.c
    ke/bug.c
    ke/clock.c
    ke/config.c
    ke/devqueue.c
    ke/dpc.c
    ke/eventobj.c
    ke/except.c
    ke/freeze.c
    ke/gate.c
    ke/gmutex.c
    ke/ipi.c
    ke/krnlinit.c
    ke/mutex.c
    ke/procobj.c
    ke/profobj.c
    ke/queue.c
    ke/semphobj.c
    ke/spinlock.c
    ke/thrdobj.c
    ke/thrdschd.c
    ke/time.c
    ke/timerobj.c
    ke/wait.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        ke/i386/ctxswitch.S
        ke/i386/trap.s
        ke/i386/usercall_asm.S
        ke/i386/zeropage.S
    )
    list(APPEND SOURCE
        ke/i386/abios.c
        ke/i386/cpu.c
        ke/i386/context.c
        ke/i386/exp.c
        ke/i386/irqobj.c
        ke/i386/kiinit.c
        ke/i386/ldt.c
        ke/i386/mtrr.c
        ke/i386/patpge.c
        ke/i386/thrdini.c
        ke/i386/traphdlr.c
        ke/i386/usercall.c
        ke/i386/v86vdm.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        ke/amd64/boot.S
        ke/amd64/ctxswitch.S
        ke/amd64/trap.S
        ke/amd64/usercall_asm.S
        ke/amd64/zeropage.S
    )
    list(APPEND SOURCE
        ke/amd64/context.c
        ke/amd64/cpu.c
        ke/amd64/except.c
        ke/amd64/interrupt.c
        ke/amd64/irql.c
        ke/amd64/kiinit.c
        ke/amd64/krnlinit.c
        ke/amd64/spinlock.c
        ke/amd64/thrdini.c
        ke/amd64/stubs.c
        ke/amd64/usercall.c
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        ke/arm/boot.s
        ke/arm/ctxswtch.s
        ke/arm/stubs_asm.s
        ke/arm/trap.s
    )
    list(APPEND SOURCE
        ke/arm/cpu.c
        ke/arm/exp.c
        ke/arm/interrupt.c
        ke/arm/kiinit.c
        ke/arm/thrdini.c
        ke/arm/trapc.c
        ke/arm/usercall.c
    )
endif()
