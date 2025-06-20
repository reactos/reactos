

add_definitions(-D_KERNEL32_)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)
spec2def(kernel32.dll kernel32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    client/dllmain.c
    client/job.c
    client/pathansi.c
    client/startupansi.c
    client/version.c
    client/file/mailslot.c
    client/file/tape.c
    wine/lzexpand.c
    wine/resource.c
    k32.h)


add_library(kernel32 MODULE
    ${SOURCE}
    kernel32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/kernel32.def)

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    set_source_files_properties(wine/resource.c PROPERTIES COMPILE_OPTIONS "/wd4146")
endif()

set_module_type(kernel32 win32dll ENTRYPOINT DllMain 12)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(kernel32 console)
################# END  HACK #################

target_link_libraries(kernel32 chkstk ${PSEH_LIB})
add_importlibs(kernel32 kernel32_win7 ntdll)
add_pch(kernel32 k32.h SOURCE)
add_dependencies(kernel32 psdk errcodes asm)
add_cd_file(TARGET kernel32 DESTINATION reactos/system32 FOR all)
