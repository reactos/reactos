
add_definitions(-D_KERNEL32_)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

set_rc_compiler()

spec2def(kernel32.dll kernel32.spec)

list(APPEND SOURCE
    client/debugger.c
    client/except.c
    client/file/ackup.c
    client/file/bintype.c
    client/file/cnotify.c
    client/file/copy.c
	client/file/console.c
    client/file/create.c
    client/file/curdir.c
    client/file/delete.c
    client/file/deviceio.c
    client/file/dir.c
    client/file/dosdev.c
    client/file/file.c
	client/file/filemap.c
    client/file/find.c
    client/file/hardlink.c
    client/file/iocompl.c
    client/file/lfile.c
    client/file/lock.c
    client/file/mailslot.c
    client/file/move.c
    client/file/npipe.c
    client/file/pipe.c
    client/file/rw.c
    client/file/tape.c
    client/file/volume.c
    client/heapmem.c
    client/virtmem.c
    client/misc/actctx.c
    client/misc/atom.c
    client/misc/comm.c
    client/misc/computername.c
    client/misc/dllmain.c
    client/misc/env.c
    client/misc/handle.c
    client/misc/ldr.c
    client/misc/lzexpand.c
    client/misc/muldiv.c
    client/misc/perfcnt.c
    client/misc/power.c
    client/misc/profile.c
    client/misc/recovery.c
    client/misc/res.c
	client/misc/resntfy.c
    client/misc/stubs.c
    client/misc/sysinfo.c
    client/misc/time.c
    client/misc/timerqueue.c
    client/misc/toolhelp.c
    client/misc/utils.c
    client/misc/version.c
    client/job.c
    client/proc.c
    client/session.c
    winnls/string/casemap.c
    winnls/string/chartype.c
    winnls/string/collation.c
    winnls/string/fold.c
    winnls/string/format_msg.c
    winnls/string/lang.c
    winnls/string/lcformat.c
    winnls/string/lstring.c
    winnls/string/nls.c
    winnls/string/sortkey.c
    client/synch.c
    client/vista.c
    client/fiber.c
    client/thread.c
    kernel32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32.def)

if(ARCH MATCHES i386)
list(APPEND SOURCE
    client/i386/fiber.S
    client/i386/thread.S)
elseif(ARCH MATCHES amd64)
list(APPEND SOURCE
    client/amd64/fiber.S
    client/amd64/thread.S)
endif(ARCH MATCHES i386)

add_library(kernel32 SHARED ${SOURCE})

set_entrypoint(kernel32 DllMain@12)
set_image_base(kernel32 ${baseaddress_kernel32})

target_link_libraries(kernel32 chkstk ${PSEH_LIB})

add_importlibs(kernel32 ntdll)
add_pch(kernel32 k32.h)
add_dependencies(kernel32 psdk errcodes asm)
add_cd_file(TARGET kernel32 DESTINATION reactos/system32 FOR all)
add_importlib_target(kernel32.spec)
