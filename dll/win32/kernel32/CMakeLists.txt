
add_subdirectory(kernel32_vista)

add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
spec2def(kernel32.dll kernel32.spec ADD_IMPORTLIB)

# Shared between kernel32 and kernel32_vista
add_library(kernel32_shared
    client/shared_utils.c
    client/file/fileutils.c
)
target_link_libraries(kernel32_shared PRIVATE pseh)
add_dependencies(kernel32_shared psdk)

list(APPEND SOURCE
    client/actctx.c
    client/appcache.c
    client/atom.c
    client/compname.c
    client/debugger.c
    client/dosdev.c
    client/dllmain.c
    client/environ.c
    client/except.c
    client/fiber.c
    client/handle.c
    client/heapmem.c
    client/job.c
    client/loader.c
    client/path.c
    client/perfcnt.c
    client/power.c
    client/proc.c
    client/resntfy.c
    client/session.c
    client/synch.c
    client/sysinfo.c
    client/time.c
    client/timerqueue.c
    client/toolhelp.c
    client/utils.c
    client/thread.c
    client/vdm.c
    client/version.c
    client/virtmem.c
    client/console/alias.c
    client/console/console.c
    client/console/history.c
    client/console/init.c
    client/console/readwrite.c
    client/file/backup.c
    client/file/cnotify.c
    client/file/copy.c
    client/file/create.c
    client/file/delete.c
    client/file/deviceio.c
    client/file/dir.c
    client/file/disk.c
    client/file/fileinfo.c
    client/file/filemap.c
    client/file/filename.c
    client/file/find.c
    client/file/hardlink.c
    client/file/iocompl.c
    client/file/lfile.c
    client/file/lock.c
    client/file/mailslot.c
    client/file/move.c
    client/file/mntpoint.c
    client/file/npipe.c
    client/file/rw.c
    client/file/tape.c
    client/file/volume.c
    wine/actctx.c
    wine/comm.c
    wine/lzexpand.c
    wine/muldiv.c
    wine/profile.c
    wine/res.c
    wine/timezone.c
    winnls/string/casemap.c
    winnls/string/collation.c
    winnls/string/digitmap.c
    winnls/string/fold.c
    winnls/string/format_msg.c
    winnls/string/japanese.c
    winnls/string/locale.c
    winnls/string/lcformat.c
    winnls/string/lstring.c
    winnls/string/nls.c
    winnls/string/sortkey.c
    k32.h)

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    set_source_files_properties(wine/res.c PROPERTIES COMPILE_OPTIONS "/wd4146")

    # Disable warning C4267: 'initializing': conversion from 'size_t' to 'DWORD', possible loss of data
    set_source_files_properties(winnls/string/locale.c PROPERTIES COMPILE_OPTIONS "/wd4267")
else()
    # Disable warning: 'variable' defined but not used
    set_source_files_properties(winnls/string/locale.c PROPERTIES COMPILE_OPTIONS "-Wno-unused-variable")
endif()

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        client/i386/fiber.S
        client/i386/thread.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        client/amd64/fiber.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        client/arm/fiber.S
        client/arm/thread.S)
endif()

add_asm_files(kernel32_asm ${ASM_SOURCE})
add_library(kernel32 MODULE
    ${SOURCE}
    ${kernel32_asm}
    kernel32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32.def)

set_module_type(kernel32 win32dll ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(kernel32 console)
################# END  HACK #################

target_link_libraries(kernel32 kernel32_vista_static kernel32_shared wine chkstk ${PSEH_LIB})
add_importlibs(kernel32 ntdll)
add_pch(kernel32 k32.h SOURCE)
add_dependencies(kernel32 psdk errcodes asm)
add_cd_file(TARGET kernel32 DESTINATION reactos/system32 FOR all)
