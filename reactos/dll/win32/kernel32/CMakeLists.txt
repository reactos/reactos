
add_definitions(-D_KERNEL32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

set_rc_compiler()

spec2def(kernel32.dll kernel32.spec)

list(APPEND SOURCE
    debug/debugger.c
    debug/output.c
    except/except.c
    file/backup.c
    file/bintype.c
    file/cnotify.c
    file/copy.c
    file/create.c
    file/curdir.c
    file/delete.c
    file/deviceio.c
    file/dir.c
    file/dosdev.c
    file/file.c
    file/find.c
    file/hardlink.c
    file/iocompl.c
    file/lfile.c
    file/lock.c
    file/mailslot.c
    file/move.c
    file/npipe.c
    file/pipe.c
    file/rw.c
    file/tape.c
    file/volume.c
    mem/global.c
    mem/heap.c
    mem/isbad.c
    mem/local.c
    mem/procmem.c
    mem/resnotify.c
    mem/section.c
    mem/virtual.c
    misc/actctx.c
    misc/atom.c
    misc/casemap.c
    misc/chartype.c
    misc/collation.c
    misc/comm.c
    misc/commdcb.c
    misc/computername.c
    misc/console.c
    misc/dllmain.c
    misc/env.c
    misc/error.c
    misc/fold.c
    misc/format_msg.c
    misc/handle.c
    misc/lang.c
    misc/lcformat.c
    misc/ldr.c
    misc/lzexpand.c
    misc/muldiv.c
    misc/nls.c
    misc/perfcnt.c
    misc/power.c
    misc/profile.c
    misc/recovery.c
    misc/res.c
    misc/sortkey.c
    misc/stubs.c
    misc/sysinfo.c
    misc/time.c
    misc/timerqueue.c
    misc/toolhelp.c
    misc/utils.c
    misc/version.c
    process/cmdline.c
    process/procsup.c
    process/job.c
    process/proc.c
    process/session.c
    string/lstring.c
    synch/condvar.c
    synch/critical.c
    synch/event.c
    synch/mutex.c
    synch/sem.c
    synch/timer.c
    synch/wait.c
    thread/fiber.c
    thread/fls.c
    thread/thread.c
    thread/tls.c
    kernel32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32.def)

if(ARCH MATCHES i386)
list(APPEND SOURCE
    thread/i386/fiber.S
    thread/i386/thread.S)
elseif(ARCH MATCHES amd64)
list(APPEND SOURCE
    thread/amd64/fiber.S
    thread/amd64/thread.S)
endif(ARCH MATCHES i386)

add_library(kernel32 SHARED ${SOURCE})

set_entrypoint(kernel32 DllMain@12)
set_image_base(kernel32 ${baseaddress_kernel32})

target_link_libraries(kernel32 ${PSEH_LIB})

add_importlibs(kernel32 ntdll)
add_pch(kernel32 k32.h)
add_dependencies(kernel32 psdk errcodes asm)
add_cd_file(TARGET kernel32 DESTINATION reactos/system32 FOR all)
add_importlib_target(kernel32.spec)
