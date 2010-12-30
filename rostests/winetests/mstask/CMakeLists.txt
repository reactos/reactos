
add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    task.c
    task_scheduler.c
    task_trigger.c
    testlist.c)

add_executable(mstask_winetest ${SOURCE})
target_link_libraries(mstask_winetest wine)
set_module_type(mstask_winetest win32cui)
add_importlibs(mstask_winetest ole32 msvcrt kernel32 ntdll)
add_cab_target(mstask_winetest 7)
