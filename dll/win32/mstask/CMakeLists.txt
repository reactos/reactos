
add_definitions(-D__WINESRC__)

remove_definitions(-DWINVER=0x502)
add_definitions(-DWINVER=0x600)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mstask.dll mstask.spec)

add_library(mstask SHARED
    factory.c
    mstask_main.c
    task.c
    task_scheduler.c
    task_trigger.c
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mstask.def)

set_module_type(mstask win32dll)

target_link_libraries(mstask
    mstask_local_interface
    uuid
    wine)

add_importlibs(mstask ole32 ntdll)

add_cab_target(mstask 1)
