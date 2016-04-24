
add_definitions(
    -D_NTOSKRNL_
    -DNO_RTL_INLINES
    -D_NTSYSTEM_
    -D_NTDLLBUILD_)

list(APPEND SOURCE
    access.c
    acl.c
    actctx.c
    appverifier.c
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
    heapuser.c
    image.c
    interlck.c
    memstream.c
    message.c
    largeint.c
    luid.c
    network.c
    nls.c
    path.c
    ppb.c
    prefix.c
    priv.c
    process.c
    propvar.c
    random.c
    rangelist.c
    registry.c
    res.c
    resource.c
    rxact.c
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
    trace.c
    unicode.c
    unicodeprefix.c
    vectoreh.c
    version.c
    wait.c
    workitem.c
    rtl.h)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        i386/debug_asm.S
        i386/except_asm.s
        i386/interlck.S
        i386/rtlmem.s
        i386/rtlswap.S
        i386/res_asm.s)
    list(APPEND SOURCE
        i386/except.c
        i386/thread.c)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        amd64/debug_asm.S
        amd64/except_asm.S
        amd64/slist.S)
    list(APPEND SOURCE
        bitmap64.c
        byteswap.c
        amd64/unwind.c
        amd64/stubs.c
        mem.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE arm/debug_asm.S)
    list(APPEND SOURCE
        arm/except.c
        byteswap.c
        mem.c)
elseif(ARCH STREQUAL "powerpc")
    list(APPEND ASM_SOURCE
        powerpc/rtlmem.s
        powerpc/rtlswap.s)
    list(APPEND SOURCE
        byteswap.c
        powerpc/debug.c
        powerpc/except.c
        powerpc/interlocked.c
        powerpc/thread.c)
endif()

add_asm_files(rtl_asm ${ASM_SOURCE})
add_library(rtl ${SOURCE} ${rtl_asm})
add_pch(rtl rtl.h SOURCE)
add_dependencies(rtl psdk asm)
