
include_directories(
    ${REACTOS_SOURCE_DIR}
    ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/arbiter
    ${REACTOS_SOURCE_DIR}/sdk/lib/cmlib
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include/internal
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers)

add_definitions(
    -D_NTOSKRNL_
    -D_NTSYSTEM_
    -DNTDDI_VERSION=0x05020400)

if(NOT DEFINED NEWCC)
    set(NEWCC FALSE)
endif()

if(NEWCC)
    add_definitions(-DNEWCC)
    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/cachesub.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/copysup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/fssup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/lazyrite.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/logsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/mdlsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/pinsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/fault.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/swapout.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/data.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/reqtools.c)
else()
    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cc/cacheman.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cc/copy.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cc/fs.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cc/lazywrite.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cc/mdl.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cc/pin.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/cc/view.c)
endif()

list(APPEND SOURCE
    ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/io.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/cache/section/sptab.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmalloc.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmapi.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmboot.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmconfig.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmcontrl.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmdata.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmdelay.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmhook.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmhvlist.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cminit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmkcbncb.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmlazy.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmmapvw.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmnotify.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmparse.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmquota.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmse.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmsecach.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmsysini.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmvalche.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/cmwraprs.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/config/ntapi.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/dbgk/dbgkobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/dbgk/dbgkutil.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/atom.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/callback.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/dbgctrl.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/efi.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/event.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/evtpair.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/exintrin.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/fmutex.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/handle.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/harderr.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/hdlsterm.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/init.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/interlocked.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/keyedevt.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/locale.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/lookas.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/mutant.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/profile.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/pushlock.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/resource.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/rundown.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/sem.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/shutdown.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/sysinfo.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/time.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/timer.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/uuid.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/win32k.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/work.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/xipdisp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/zone.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/dbcsname.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/fastio.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/faulttol.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/filelock.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/filter.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/filtrctx.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/fsfilter.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/fsrtlpc.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/largemcb.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/mcb.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/name.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/notify.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/oplock.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/pnp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/stackovf.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/tunnel.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fsrtl/unc.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fstub/disksup.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fstub/fstubex.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fstub/halstub.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/fstub/translate.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/inbv/bootanim.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/inbv/inbv.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/inbv/inbvport.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/adapter.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/arcname.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/bootlog.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/controller.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/device.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/deviface.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/driver.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/error.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/file.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/iocomp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/ioevent.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/iofunc.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/iomdl.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/iomgr.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/iorsrce.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/iotimer.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/iowork.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/irp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/irq.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/ramdisk.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/rawfs.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/remlock.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/symlink.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/util.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/iomgr/volume.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/arbiters.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/devaction.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/devnode.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/plugplay.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpdma.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpinit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpirp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpmap.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpmgr.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpnotify.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpreport.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnpres.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnproot.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/pnpmgr/pnputil.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/io/debug.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/kdapi.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/kdbreak.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/kddata.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/kdinit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/kdlock.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/kdprint.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/kdtrap.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/apc.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/balmgr.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/bug.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/clock.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/config.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/devqueue.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/dpc.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/eventobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/except.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/freeze.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/gate.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/gmutex.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/ipi.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/krnlinit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/mutex.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/processor.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/procobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/profobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/queue.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/semphobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/spinlock.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/thrdobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/thrdschd.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/time.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/timerobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/wait.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/close.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/complete.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/connect.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/create.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/listen.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/port.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/reply.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/lpc/send.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/contmem.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/drvmgmt.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/dynamic.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/expool.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/hypermap.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/iosup.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/kdbg.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/largepag.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/mdlsup.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/mmdbg.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/mminit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/mmsup.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/ncache.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/pagfault.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/pfnlist.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/pool.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/procsup.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/section.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/session.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/special.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/sysldr.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/syspte.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/vadnode.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/virtual.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/wslist.cpp
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/zeropage.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/balance.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/freelist.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/marea.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/mmfault.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/mminit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/pagefile.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/region.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/rmap.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/section.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/shutdown.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/devicemap.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obdir.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obhandle.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obinit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/oblife.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/oblink.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obname.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obref.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obsdcach.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obsecure.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ob/obwait.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/po/events.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/po/guid.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/po/poshtdwn.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/po/povolume.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/po/power.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/apphelp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/debug.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/job.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/kill.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/process.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/psmgr.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/psnotify.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/query.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/quota.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/security.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/state.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/thread.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/win32.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/rtl/libsupp.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/rtl/misc.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/access.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/accesschk.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/acl.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/audit.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/client.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/objtype.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/priv.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/sd.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/semgr.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/sid.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/sqos.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/srm.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/subject.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/token.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/tokenadj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/tokencls.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/se/tokenlif.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/vf/driver.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/wmi/guidobj.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/wmi/smbios.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/wmi/wmi.c
    ${REACTOS_SOURCE_DIR}/ntoskrnl/wmi/wmidrv.c)

if(DBG)
    list(APPEND SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/se/debug.c)
endif()

list(APPEND ASM_SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/zw.S)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/i386/fastinterlck_asm.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/i386/ioport.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/ctxswitch.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/trap.s
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/usercall_asm.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/zeropage.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/rtl/i386/prefetch.S)
    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/config/i386/cmhardwr.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/i386/kdx86.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/abios.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/cpu.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/context.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/exp.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/freeze.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/irqobj.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/kiinit.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/ldt.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/mtrr.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/patpge.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/thrdini.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/traphdlr.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/usercall.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/v86vdm.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/i386/page.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/i386/procsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/i386/init.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/i386/psctx.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/i386/psldt.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/vdm/vdmmain.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/vdm/vdmexec.c)
    if(BUILD_MP)
        list(APPEND SOURCE
            ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/i386/mproc.c)
    endif()
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/boot.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/ctxswitch.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/trap.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/usercall_asm.S
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/zeropage.S)
    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/config/i386/cmhardwr.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/i386/page.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/amd64/kdx64.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/context.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/cpu.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/except.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/freeze.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/interrupt.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/ipi.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/irql.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/kiinit.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/krnlinit.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/spinlock.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/thrdini.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/amd64/init.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/amd64/procsup.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/amd64/psctx.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/stubs.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/traphandler.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/usercall.c)
    if(BUILD_MP)
        list(APPEND SOURCE
            ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/amd64/mproc.c)
    endif()
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ex/arm/ioport.s
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/boot.s
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/ctxswtch.s
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/stubs_asm.s
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/trap.s)
    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/config/arm/cmhardwr.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd64/arm/kdarm.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/cpu.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/exp.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/interrupt.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/kiinit.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/thrdini.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/trapc.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ke/arm/usercall.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/arm/page.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/mm/ARM3/arm/init.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/ps/arm/psctx.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/rtl/arm/rtlexcpt.c)
endif()

if(NOT _WINKD_)
    if(KDBG)
        add_definitions(-DKDBG)
    endif()

    if(ARCH STREQUAL "i386")
        list(APPEND SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/i386/kdserial.c)
        if(KDBG)
            list(APPEND ASM_SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/i386/kdb_help.S)
            list(APPEND SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/i386/i386-dis.c)
        endif()
    elseif(ARCH STREQUAL "amd64")
        list(APPEND SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/i386/kdserial.c)
        if(KDBG)
            list(APPEND ASM_SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/amd64/kdb_help.S)
            list(APPEND SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/i386/i386-dis.c)
        endif()
    elseif(ARCH STREQUAL "arm")
        list(APPEND SOURCE ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/arm/kdserial.c)
    endif()

    if(KDBG)
        list(APPEND SOURCE
            ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/kdbg.c
            ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/kdb.c
            ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/kdb_cli.c
            ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/kdb_cmdhist.c
            ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/kdb_expr.c
            ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/kdb_print.c
            ${REACTOS_SOURCE_DIR}/ntoskrnl/kdbg/kdb_symbols.c)
    endif()

    list(APPEND SOURCE
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/kdio.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/kdmain.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/kdprompt.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/kdps2kbd.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/kdserial.c
        ${REACTOS_SOURCE_DIR}/ntoskrnl/kd/kdterminal.c)

else()
    add_definitions(-D_WINKD_)
endif()
