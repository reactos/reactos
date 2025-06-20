if (DLL_EXPORT_VERSION LESS 0x601)
    set(kernelbase_name kernel32_win7)
else()
    set(kernelbase_name kernelbase)
endif()

add_subdirectory(wine)
add_subdirectory(kernel32)

add_definitions(-D_KERNEL32_
                -D_KERNELBASE_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
include_directories(kernel32/)
spec2def(${kernelbase_name}.dll kernelbase.spec ADD_IMPORTLIB)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
list(APPEND SOURCE
    kernel32/client/actctx.c
    kernel32/client/appcache.c
    kernel32/client/atom.c
    kernel32/client/compname.c
    kernel32/client/debugger.c
    kernel32/client/dosdev.c
    kernel32/client/dllmain.c
    kernel32/client/environ.c
    kernel32/client/except.c
    kernel32/client/fiber.c
    kernel32/client/handle.c
    kernel32/client/heapmem.c
    kernel32/client/job.c
    kernel32/client/loader.c
    kernel32/client/path.c
    kernel32/client/perfcnt.c
    kernel32/client/power.c
    kernel32/client/proc.c
    kernel32/client/resntfy.c
    kernel32/client/session.c
    kernel32/client/shared_utils.c
    kernel32/client/synch.c
    kernel32/client/sysinfo.c
    kernel32/client/time.c
    kernel32/client/timerqueue.c
    kernel32/client/toolhelp.c
    kernel32/client/utils.c
    kernel32/client/thread.c
    kernel32/client/vdm.c
    kernel32/client/version.c
    kernel32/client/virtmem.c
    kernel32/client/console/alias.c
    kernel32/client/console/console.c
    kernel32/client/console/history.c
    kernel32/client/console/init.c
    kernel32/client/console/readwrite.c
    kernel32/client/file/backup.c
    kernel32/client/file/cnotify.c
    kernel32/client/file/copy.c
    kernel32/client/file/create.c
    kernel32/client/file/delete.c
    kernel32/client/file/deviceio.c
    kernel32/client/file/dir.c
    kernel32/client/file/disk.c
    kernel32/client/file/fileinfo.c
    kernel32/client/file/filemap.c
    kernel32/client/file/filename.c
    kernel32/client/file/fileutils.c
    kernel32/client/file/find.c
    kernel32/client/file/hardlink.c
    kernel32/client/file/iocompl.c
    kernel32/client/file/lfile.c
    kernel32/client/file/lock.c
    kernel32/client/file/move.c
    kernel32/client/file/mntpoint.c
    kernel32/client/file/npipe.c
    kernel32/client/file/rw.c
    kernel32/client/file/volume.c
    kernel32/wine/actctx.c
    kernel32/wine/comm.c
    kernel32/wine/muldiv.c
    kernel32/wine/profile.c
    kernel32/wine/timezone.c
    kernel32/winnls/string/casemap.c
    kernel32/winnls/string/collation.c
    kernel32/winnls/string/digitmap.c
    kernel32/winnls/string/fold.c
    kernel32/winnls/string/format_msg.c
    kernel32/winnls/string/japanese.c
    kernel32/winnls/string/locale.c
    kernel32/winnls/string/lcformat.c
    kernel32/winnls/string/lstring.c
    kernel32/winnls/string/nls.c
    kernel32/winnls/string/sortkey.c
    kernel32/k32.h)

if(MSVC)
    # Disable warning C4267: 'initializing': conversion from 'size_t' to 'DWORD', possible loss of data
    set_source_files_properties(kernel32/winnls/string/locale.c PROPERTIES COMPILE_OPTIONS "/wd4267")
else()
    # Disable warning: 'variable' defined but not used
    set_source_files_properties(kernel32/winnls/string/locale.c PROPERTIES COMPILE_OPTIONS "-Wno-unused-variable")
endif()

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        kernel32/client/i386/fiber.S
        kernel32/client/i386/thread.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        kernel32/client/amd64/fiber.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        kernel32/client/arm/fiber.S
        kernel32/client/arm/thread.S)
endif()

add_asm_files(kernelbase_asm ${ASM_SOURCE})
# This isn't called KernelBase because apps will explode and we aren't win7 yet.
add_library(${kernelbase_name} MODULE
    ${SOURCE}
    ${kernelbase_asm}
    kernelbase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/${kernelbase_name}_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/${kernelbase_name}.def)

set_module_type(${kernelbase_name} win32dll ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(${kernelbase_name} console)
################# END  HACK #################

target_link_libraries(${kernelbase_name} kernel32_vista_static kernelbase_static wine chkstk ${PSEH_LIB})
add_importlibs(${kernelbase_name} ntdll_vista ntdll)
add_pch(${kernelbase_name} kernel32/k32.h SOURCE)
add_dependencies(${kernelbase_name} psdk errcodes asm)
add_cd_file(TARGET ${kernelbase_name} DESTINATION reactos/system32 FOR all)
