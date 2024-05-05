
add_definitions(
    -D_NTOSKRNL_
    -DNO_RTL_INLINES
    -D_NTSYSTEM_
    -D_NTDLLBUILD_)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Enable this again. CORE-17637
    add_compile_options(-Wunused-result)
endif()

list(APPEND RTL_WINE_SOURCE
    actctx.c
    timerqueue.c
    wait.c
)

if(MSVC)
    # Silence warning C4267: 'initializing': conversion from 'size_t' to 'const int', possible loss of data
    set_source_files_properties(${RTL_WINE_SOURCE} PROPERTIES COMPILE_OPTIONS "/wd4267")
endif()

list(APPEND SOURCE
    ${RTL_WINE_SOURCE}
    wine_debug.c
    access.c
    acl.c
    appverifier.c
    assert.c
    atom.c
    avltable.c
    bitmap.c
    bootdata.c
    compress.c
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
    largeint.c
    luid.c
    memstream.c
    message.c
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
    sid.c
    slist.c
    splaytree.c
    sysvol.c
    thread.c
    time.c
    timezone.c
    trace.c
    unicode.c
    unicodeprefix.c
    vectoreh.c
    version.c
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
        amd64/dynfntbl.c
        amd64/except.c
        amd64/unwind.c
        amd64/stubs.c
        mem.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE arm/debug_asm.S)
    list(APPEND SOURCE
        arm/except.c
        byteswap.c
        mem.c)
endif()

add_asm_files(rtl_asm ${ASM_SOURCE})
add_library(rtl ${SOURCE} ${rtl_asm})
target_link_libraries(rtl PRIVATE pseh)
add_pch(rtl rtl.h SOURCE)
add_dependencies(rtl psdk asm)

list(APPEND SOURCE_VISTA
    condvar.c
    runonce.c
    srw.c
    utf8.c)

add_library(rtl_vista ${SOURCE_VISTA})
add_pch(rtl_vista rtl_vista.h SOURCE_VISTA)
add_dependencies(rtl_vista psdk)
target_link_libraries(rtl_vista PRIVATE pseh)
