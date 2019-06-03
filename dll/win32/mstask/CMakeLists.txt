
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
generate_idl_iids(mstask_local.idl)

list(APPEND SOURCE
    factory.c
    mstask_main.c
    task.c
    task_scheduler.c
    task_trigger.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/mstask_local_i.c)

add_library(mstask MODULE
    ${SOURCE}
    rsrc.rc)
spec2def(mstask mstask.dll mstask.spec)

set_module_type(mstask win32dll)
target_link_libraries(mstask uuid wine)
add_importlibs(mstask ole32 msvcrt kernel32 ntdll)
add_pch(mstask precomp.h SOURCE)
add_cd_file(TARGET mstask DESTINATION reactos/system32 FOR all)
