
add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
spec2def(kernel32.dll kernel32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    client/atomansi.c
    client/dllmain.c
    client/job.c
    client/pathansi.c
    client/power.c
    client/procansi.c
    client/versionansi.c
    client/file/copyansi.c
    client/file/diransi.c
    client/file/fileinfoansi.c
    client/file/mailslot.c
    client/file/moveansi.c
    client/file/mntpointansi.c
    client/file/tape.c
    client/file/volumeansi.c
    wine/lzexpand.c
    wine/res.c
    k32.h)

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    set_source_files_properties(wine/res.c PROPERTIES COMPILE_OPTIONS "/wd4146")
endif()
add_library(kernel32 MODULE
    ${SOURCE}
    kernel32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32.def)

set_module_type(kernel32 win32dll ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(kernel32 console)
################# END  HACK #################

target_link_libraries(kernel32 psapi_static wine chkstk ${PSEH_LIB})
add_importlibs(kernel32 ntdll)
add_pch(kernel32 k32.h SOURCE)
add_dependencies(kernel32 psdk errcodes asm)
add_cd_file(TARGET kernel32 DESTINATION reactos/system32 FOR all)
