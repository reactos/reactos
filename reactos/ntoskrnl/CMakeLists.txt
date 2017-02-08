
PROJECT(NTOS)

spec2def(ntoskrnl.exe ntoskrnl.spec ADD_IMPORTLIB)

include_directories(
    ${REACTOS_SOURCE_DIR}
    ${REACTOS_SOURCE_DIR}/lib/cmlib
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include/internal
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_definitions(
    -D__NTOSKRNL__
    -D_NTOSKRNL_
    -D_NTSYSTEM_
    -D_IN_KERNEL_
    -DNTDDI_VERSION=0x05020400)

if(NOT DEFINED NEWCC)
    set(NEWCC FALSE)
endif()

if(NEWCC)
    add_definitions(-DNEWCC)
    list(APPEND SOURCE
        cache/cachesub.c
        cache/copysup.c
        cache/fssup.c
        cache/lazyrite.c
        cache/logsup.c
        cache/mdlsup.c
        cache/pinsup.c)
else()
    list(APPEND SOURCE
        cc/cacheman.c
        cc/copy.c
        cc/fs.c
        cc/mdl.c
        cc/pin.c
        cc/view.c)
endif()

list(APPEND SOURCE
    cache/section/io.c
    cache/section/data.c
    cache/section/fault.c
    cache/section/reqtools.c
    cache/section/sptab.c
    cache/section/swapout.c
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
    config/cmindex.c
    config/cminit.c
    config/cmkcbncb.c
    config/cmkeydel.c
    config/cmlazy.c
    config/cmmapvw.c
    config/cmname.c
    config/cmnotify.c
    config/cmparse.c
    config/cmquota.c
    config/cmse.c
    config/cmsecach.c
    config/cmsysini.c
    config/cmvalche.c
    config/cmvalue.c
    config/cmwraprs.c
    config/ntapi.c
    dbgk/dbgkobj.c
    dbgk/dbgkutil.c
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
    fsrtl/dbcsname.c
    fsrtl/fastio.c
    fsrtl/faulttol.c
    fsrtl/filelock.c
    fsrtl/filter.c
    fsrtl/filtrctx.c
    fsrtl/fsfilter.c
    fsrtl/fsrtlpc.c
    fsrtl/largemcb.c
    fsrtl/mcb.c
    fsrtl/name.c
    fsrtl/notify.c
    fsrtl/oplock.c
    fsrtl/pnp.c
    fsrtl/stackovf.c
    fsrtl/tunnel.c
    fsrtl/unc.c
    fstub/disksup.c
    fstub/fstubex.c
    fstub/halstub.c
    fstub/translate.c
    inbv/inbv.c
    inbv/inbvport.c
    io/iomgr/adapter.c
    io/iomgr/arcname.c
    io/iomgr/bootlog.c
    io/iomgr/controller.c
    io/iomgr/device.c
    io/iomgr/deviface.c
    io/iomgr/driver.c
    io/iomgr/error.c
    io/iomgr/file.c
    io/iomgr/iocomp.c
    io/iomgr/ioevent.c
    io/iomgr/iofunc.c
    io/iomgr/iomdl.c
    io/iomgr/iomgr.c
    io/iomgr/iorsrce.c
    io/iomgr/iotimer.c
    io/iomgr/iowork.c
    io/iomgr/irp.c
    io/iomgr/irq.c
    io/iomgr/ramdisk.c
    io/iomgr/rawfs.c
    io/iomgr/remlock.c
    io/iomgr/symlink.c
    io/iomgr/util.c
    io/iomgr/volume.c
    io/pnpmgr/plugplay.c
    io/pnpmgr/pnpdma.c
    io/pnpmgr/pnpinit.c
    io/pnpmgr/pnpmgr.c
    io/pnpmgr/pnpnotify.c
    io/pnpmgr/pnpreport.c
    io/pnpmgr/pnpres.c
    io/pnpmgr/pnproot.c
    io/pnpmgr/pnputil.c
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
    lpc/close.c
    lpc/complete.c
    lpc/connect.c
    lpc/create.c
    lpc/listen.c
    lpc/port.c
    lpc/reply.c
    lpc/send.c
    mm/ARM3/contmem.c
    mm/ARM3/drvmgmt.c
    mm/ARM3/dynamic.c
    mm/ARM3/expool.c
    mm/ARM3/hypermap.c
    mm/ARM3/iosup.c
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
    mm/ARM3/special.c
    mm/ARM3/sysldr.c
    mm/ARM3/syspte.c
    mm/ARM3/vadnode.c
    mm/ARM3/virtual.c
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
    ob/obdir.c
    ob/obhandle.c
    ob/obinit.c
    ob/oblife.c
    ob/oblink.c
    ob/obname.c
    ob/obref.c
    ob/obsdcach.c
    ob/obsecure.c
    ob/obwait.c
    po/events.c
    po/guid.c
    po/poshtdwn.c
    po/povolume.c
    po/power.c
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
    rtl/libsupp.c
    rtl/misc.c
    se/access.c
    se/acl.c
    se/audit.c
    se/lsa.c
    se/priv.c
    se/sd.c
    se/semgr.c
    se/sid.c
    se/token.c
    vf/driver.c
    wmi/wmi.c
    ntoskrnl.rc)

list(APPEND ASM_SOURCE ex/zw.S)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        ex/i386/fastinterlck_asm.S
        ex/i386/ioport.S
        ke/i386/ctxswitch.S
        ke/i386/trap.s
        ke/i386/usercall_asm.S
        rtl/i386/stack.S)
    list(APPEND SOURCE
        config/i386/cmhardwr.c
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
        mm/i386/page.c
        mm/ARM3/i386/init.c
        ps/i386/psctx.c
        ps/i386/psldt.c
        vdm/vdmmain.c
        vdm/vdmexec.c)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        ke/amd64/boot.S
        ke/amd64/ctxswitch.S
        ke/amd64/trap.S)
    list(APPEND SOURCE
        config/i386/cmhardwr.c
        ke/amd64/context.c
        ke/amd64/cpu.c
        ke/amd64/except.c
        ke/amd64/interrupt.c
        ke/amd64/irql.c
        ke/amd64/kiinit.c
        ke/amd64/krnlinit.c
        ke/amd64/spinlock.c
        ke/amd64/stubs.c
        ke/amd64/thrdini.c
        mm/amd64/init.c
        mm/amd64/page.c
        ps/amd64/psctx.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        ke/arm/boot.s
        ke/arm/ctxswtch.s
        ke/arm/stubs_asm.s
        ke/arm/trap.s)
    list(APPEND SOURCE
        config/arm/cmhardwr.c
        ke/arm/cpu.c
        ke/arm/exp.c
        ke/arm/kiinit.c
        ke/arm/thrdini.c
        ke/arm/trapc.c
        ke/arm/usercall.c
        mm/arm/page.c
        mm/ARM3/arm/init.c
        ps/arm/psctx.c
        rtl/arm/rtlexcpt.c)
elseif(ARCH STREQUAL "powerpc")
    list(APPEND ASM_SOURCE
        ke/powerpc/main_asm.S
        ke/powerpc/ctxhelp.S)
    list(APPEND SOURCE
        config/powerpc/cmhardwr.c
        ke/powerpc/cpu.c
        ke/powerpc/exp.c
        ke/powerpc/kiinit.c
        ke/powerpc/ppc_irq.c
        ke/powerpc/stubs.c
        ke/powerpc/systimer.c
        ke/powerpc/thrdini.c
        ke/powerpc/ctxswitch.c
        mm/powerpc/pfault.c
        mm/powerpc/page.c)
endif()

if(NOT _WINKD_)
    if(ARCH STREQUAL "i386")
        list(APPEND SOURCE
            kd/i386/kdbg.c
            kd/i386/kdmemsup.c
            kd/wrappers/gdbstub.c)
        if(KDBG)
            list(APPEND ASM_SOURCE kdbg/i386/kdb_help.S)
            list(APPEND SOURCE kdbg/i386/i386-dis.c)
        endif()
    elseif(ARCH STREQUAL "amd64")
        list(APPEND SOURCE
            kd/amd64/kd.c
            kd/i386/kdbg.c  # Use the x86 file
            kd/amd64/kdmemsup.c)
        if(KDBG)
            list(APPEND ASM_SOURCE kdbg/amd64/kdb_help.S)
            list(APPEND SOURCE
                kdbg/amd64/i386-dis.c
                kdbg/amd64/kdb.c)
        endif()
    elseif(ARCH STREQUAL "arm")
        list(APPEND SOURCE kd/arm/kdbg.c)
    elseif(ARCH STREQUAL "powerpc")
        list(APPEND SOURCE kd/wrappers/gdbstub_powerpc.c)
    endif()

    if(KDBG)
        list(APPEND SOURCE
            kdbg/kdb.c
            kdbg/kdb_cli.c
            kdbg/kdb_expr.c
            kdbg/kdb_keyboard.c
            kdbg/kdb_serial.c)
    endif()

    if(DBG OR KDBG)
        #list(APPEND SOURCE kdbg/kdb_symbols.cmake.c)
        list(APPEND SOURCE kdbg/kdb_symbols.c)
    endif()

    list(APPEND SOURCE
        kd/wrappers/bochs.c
        kd/wrappers/kdbg.c
        kd/kdinit.c
        kd/kdio.c
        kd/kdmain.c)

else() # _WINKD_

    list(APPEND SOURCE
        kd64/kdapi.c
        kd64/kdbreak.c
        kd64/kddata.c
        kd64/kdinit.c
        kd64/kdlock.c
        kd64/kdprint.c
        kd64/kdtrap.c)

    if(ARCH STREQUAL "i386")
        list(APPEND SOURCE kd64/i386/kdx86.c)
    elseif(ARCH STREQUAL "amd64")
        list(APPEND SOURCE kd64/amd64/kdx64.c)
    elseif(ARCH STREQUAL "arm")
        list(APPEND SOURCE kd64/arm/kdarm.c)
    endif()

endif()

add_asm_files(ntoskrnl_asm ${ASM_SOURCE})

add_executable(ntoskrnl
    ${ntoskrnl_asm}
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/ntoskrnl.def)

if(ARCH STREQUAL "i386")
    set_entrypoint(ntoskrnl KiSystemStartup 4)
else()
    set_entrypoint(ntoskrnl KiSystemStartup)
endif()
set_subsystem(ntoskrnl native)

if(MSVC)
    set_image_base(ntoskrnl 0x00400000)
else()
    set_image_base(ntoskrnl 0x80800000)
endif()

# Linker bug
if(NOT MSVC AND LTCG)
    add_target_link_flags(ntoskrnl "-shared")
endif()

target_link_libraries(ntoskrnl
    cportlib
    csq
    ${PSEH_LIB}
    cmlib
    rtl
    rossym
    libcntpr
    wdmguid
    ioevent)

add_importlibs(ntoskrnl hal kdcom bootvid)
add_pch(ntoskrnl include/ntoskrnl.h)
add_dependencies(ntoskrnl psdk bugcodes asm)
add_cd_file(TARGET ntoskrnl DESTINATION reactos/system32 NO_CAB FOR all)

if(BUILD_MP)
    spec2def(ntkrnlmp.exe ntoskrnl.spec)

    if(MSVC_IDE AND (CMAKE_VERSION MATCHES "ReactOS"))
        set_source_files_properties(${ASM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "CONFIG_SMP")
    endif()

    add_asm_files(ntkrnlmp_asm ${ASM_SOURCE})

    add_executable(ntkrnlmp
        ${ntkrnlmp_asm}
        ${SOURCE}
        ${CMAKE_CURRENT_BINARY_DIR}/ntkrnlmp.def)

    add_target_compile_definitions(ntkrnlmp CONFIG_SMP)

    if(ARCH STREQUAL "i386")
        set_entrypoint(ntkrnlmp KiSystemStartup 4)
    else()
        set_entrypoint(ntkrnlmp KiSystemStartup)
    endif()
    set_subsystem(ntkrnlmp native)

    if(MSVC)
        set_image_base(ntkrnlmp 0x00400000)
    else()
        set_image_base(ntkrnlmp 0x80800000)
    endif()

    # Linker bug
    if(NOT MSVC AND LTCG)
        add_target_link_flags(ntkrnlmp "-shared")
    endif()

    target_link_libraries(ntkrnlmp cportlib csq ${PSEH_LIB} cmlib rtl rossym libcntpr wdmguid ioevent)
    add_importlibs(ntkrnlmp hal kdcom bootvid)
    add_dependencies(ntkrnlmp psdk bugcodes asm)
    add_cd_file(TARGET ntkrnlmp DESTINATION reactos/system32 NO_CAB FOR all)
endif()

add_asm_files(ntdllsys_asm ntdll.S)
add_library(ntdllsys ${ntdllsys_asm})
set_target_properties(ntdllsys PROPERTIES LINKER_LANGUAGE "C")
