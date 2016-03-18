
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
generate_idl_iids(mstask_local.idl)
spec2def(mstask.dll mstask.spec)

list(APPEND SOURCE
    factory.c
    mstask_main.c
    task.c
    task_scheduler.c
    task_trigger.c
    mstask_private.h
    ${CMAKE_CURRENT_BINARY_DIR}/mstask_local_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/mstask_stubs.c)

add_library(mstask SHARED
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mstask.def)

set_module_type(mstask win32dll)
target_link_libraries(mstask uuid wine)
add_importlibs(mstask ole32 msvcrt kernel32 ntdll)
add_pch(mstask mstask_private.h SOURCE)
add_cd_file(TARGET mstask DESTINATION reactos/system32 FOR all)
