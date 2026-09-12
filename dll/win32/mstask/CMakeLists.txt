
include_directories(BEFORE
    ${REACTOS_BINARY_DIR}/dll/win32/netapi32)
add_rpc_file(client atsvc.idl)
spec2def(mstask.dll mstask.spec)

list(APPEND SOURCE
    factory.c
    mstask_main.c
    task.c
    task_scheduler.c
    task_trigger.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/atsvc_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/mstask_stubs.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(mstask MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/mstask.def)

set_module_type(mstask win32dll)
add_idl_reg_scripts(mstask registry mstask_local.idl)
target_link_libraries(mstask uuid wine wine_dll_register oldnames)
add_importlibs(mstask ole32 oleaut32 rpcrt4 msvcrt kernel32 ntdll)
add_dependencies(mstask netapi32 wineheaders)
add_pch(mstask precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET mstask DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mstask) # CORE-5743: No CONST_VTABLE
