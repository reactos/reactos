
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

if (DLL_EXPORT_VERSION LESS 0x601)
    set(kernelbase_name kernelbase)
else()
    set(kernelbase_name kernelbase)
endif()

add_definitions(-D_KERNEL32_
                -D_KERNELBASE_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
include_directories(nativekernel32/)
spec2def(${kernelbase_name}.dll kernelbase.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    nativekernel32/client/actctx.c
    nativekernel32/client/appcache.c
    nativekernel32/client/compname.c
    nativekernel32/client/debugger.c
    nativekernel32/client/dosdev.c
    nativekernel32/client/dllmain.c
    nativekernel32/client/environ.c
    nativekernel32/client/except.c
    nativekernel32/client/fiber.c
    nativekernel32/client/handle.c
    nativekernel32/client/heapmem.c
    nativekernel32/client/loader.c
    nativekernel32/client/path.c
    nativekernel32/client/perfcnt.c
    nativekernel32/client/proc.c
    nativekernel32/client/resntfy.c
    nativekernel32/client/session.c
    nativekernel32/client/shared_utils.c
    nativekernel32/client/synch.c
    nativekernel32/client/sysinfo.c
    nativekernel32/client/time.c
    nativekernel32/client/timerqueue.c
    nativekernel32/client/toolhelp.c
    nativekernel32/client/utils.c
    nativekernel32/client/thread.c
    nativekernel32/client/vdm.c
    nativekernel32/client/version.c
    nativekernel32/client/virtmem.c
    nativekernel32/client/console/alias.c
    nativekernel32/client/console/console.c
    nativekernel32/client/console/history.c
    nativekernel32/client/console/init.c
    nativekernel32/client/console/readwrite.c
    nativekernel32/client/file/backup.c
    nativekernel32/client/file/cnotify.c
    nativekernel32/client/file/copy.c
    nativekernel32/client/file/create.c
    nativekernel32/client/file/delete.c
    nativekernel32/client/file/deviceio.c
    nativekernel32/client/file/dir.c
    nativekernel32/client/file/disk.c
    nativekernel32/client/file/fileinfo.c
    nativekernel32/client/file/filemap.c
    nativekernel32/client/file/filename.c
    nativekernel32/client/file/fileutils.c
    nativekernel32/client/file/find.c
    nativekernel32/client/file/hardlink.c
    nativekernel32/client/file/iocompl.c
    nativekernel32/client/file/lfile.c
    nativekernel32/client/file/lock.c
    nativekernel32/client/file/move.c
    nativekernel32/client/file/mntpoint.c
    nativekernel32/client/file/npipe.c
    nativekernel32/client/file/rw.c
    nativekernel32/client/file/volume.c
    nativekernel32/wine/actctx.c
    nativekernel32/wine/comm.c
    nativekernel32/wine/muldiv.c
    nativekernel32/wine/profile.c
    nativekernel32/wine/timezone.c
    nativekernel32/winnls/string/casemap.c
    nativekernel32/winnls/string/collation.c
    nativekernel32/winnls/string/digitmap.c
    nativekernel32/winnls/string/fold.c
    nativekernel32/winnls/string/format_msg.c
    nativekernel32/winnls/string/japanese.c
    nativekernel32/winnls/string/locale.c
    nativekernel32/winnls/string/lcformat.c
    nativekernel32/winnls/string/lstring.c
    nativekernel32/winnls/string/nls.c
    nativekernel32/winnls/string/sortkey.c
    nativekernel32/k32.h)

if(MSVC)
    # Disable warning C4267: 'initializing': conversion from 'size_t' to 'DWORD', possible loss of data
    set_source_files_properties(nativekernel32/winnls/string/locale.c PROPERTIES COMPILE_OPTIONS "/wd4267")
else()
    # Disable warning: 'variable' defined but not used
    set_source_files_properties(nativekernel32/winnls/string/locale.c PROPERTIES COMPILE_OPTIONS "-Wno-unused-variable")
endif()

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        nativekernel32/client/i386/fiber.S
        nativekernel32/client/i386/thread.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        nativekernel32/client/amd64/fiber.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        nativekernel32/client/arm/fiber.S
        nativekernel32/client/arm/thread.S)
endif()

add_asm_files(kernelbase_asm ${ASM_SOURCE})
# This isn't called KernelBase because apps will explode and we aren't win7 yet.
add_library(${kernelbase_name} MODULE
    ${SOURCE}
    ${kernelbase_asm}
    kernelbase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/${kernelbase_name}_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/${kernelbase_name}.def)
if(MSVC)
    # Disable warning C4133: 'function': incompatible types - from 'WICPixelFormatNumericRepresentation *' to 'DWORD *'
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    target_compile_options(${kernelbase_name} PRIVATE /wd4047 /wd4024 /wd4133 /wd4146)

endif()
set_module_type(${kernelbase_name} win32dll ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(${kernelbase_name} console)
################# END  HACK #################

target_link_libraries(${kernelbase_name} win32_registry psapi_static cryptlib wine chkstk ${PSEH_LIB})
add_importlibs(${kernelbase_name} ntdll)
add_pch(${kernelbase_name} kernel32/k32.h SOURCE)
add_dependencies(${kernelbase_name} psdk errcodes asm)
add_cd_file(TARGET ${kernelbase_name} DESTINATION reactos/system32 FOR all)
