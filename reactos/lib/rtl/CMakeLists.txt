
add_definitions(
    -D_NTOSKRNL_
    -DNO_RTL_INLINES
    -D_NTSYSTEM_
    -D_NTDLLBUILD_)

list(APPEND SOURCE
    access.c
    acl.c
    actctx.c
    assert.c
    atom.c
    avltable.c
    bitmap.c
    bootdata.c
    compress.c
    condvar.c
    crc32.c
    critical.c
    dbgbuffer.c
    debug.c
    dos8dot3.c
    encode.c
    env.c
    error.c
    exception.c
    generictable.c
    handle.c
    heap.c
    heapdbg.c
	heappage.c
    image.c
    interlck.c
    message.c
    largeint.c
    luid.c
    network.c
    nls.c
    path.c
    ppb.c
    process.c
    propvar.c
    random.c
    rangelist.c
    registry.c
    res.c
    resource.c
    sd.c
    security.c
    slist.c
    sid.c
    srw.c
    splaytree.c
    thread.c
    time.c
    timezone.c
    timerqueue.c
    unicode.c
    unicodeprefix.c
    vectoreh.c
    version.c
    wait.c
    workitem.c)

if(ARCH MATCHES i386)
    list(APPEND SOURCE
        i386/debug_asm.S
        i386/except_asm.s
        i386/except.c
        i386/interlck.S
        i386/rtlmem.s
        i386/rtlswap.S
        i386/res_asm.s
        i386/thread.c)
elseif(ARCH MATCHES amd64)
    list(APPEND SOURCE
        byteswap.c
        amd64/debug_asm.S
        amd64/except_asm.S
        amd64/slist.S
        amd64/unwind.c
        amd64/stubs.c
        mem.c)
elseif(ARCH MATCHES arm)
    list(APPEND SOURCE
        byteswap.c
        arm/debug_asm.S
        mem.c)
elseif(ARCH MATCHES powerpc)
    list(APPEND SOURCE
        byteswap.c
        powerpc/debug.c
        powerpc/except.c
        powerpc/interlocked.c
        powerpc/rtlmem.s
        powerpc/rtlswap.s
        powerpc/thread.c)
endif()

add_library(rtl ${SOURCE})
add_pch(rtl rtl.h)
add_dependencies(rtl psdk asm)
