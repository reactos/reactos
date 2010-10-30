
add_definitions(-D__WINESRC__)

remove_definitions(-DWINVER=0x502)
add_definitions(-DWINVER=0x600)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mstask ${CMAKE_CURRENT_SOURCE_DIR}/mstask.spec ${CMAKE_CURRENT_BINARY_DIR}/mstask.def)

add_library(mstask SHARED
    factory.c
    mstask_main.c
    task.c
    task_scheduler.c
    task_trigger.c
    rsrc.rc)

set_module_type(mstask win32dll)

target_link_libraries(mstask
    ${CMAKE_CURRENT_BINARY_DIR}/mstask.def
    mstask_local_interface
    uuid
    wine)

add_importlibs(mstask ole32 ntdll)
add_dependencies(mstask mstask_def)
add_cab_target(mstask 1)
