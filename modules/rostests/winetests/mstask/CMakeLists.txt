
list(APPEND SOURCE
    task.c
    task_scheduler.c
    task_trigger.c
    testlist.c)

add_executable(mstask_winetest ${SOURCE})
set_module_type(mstask_winetest win32cui)
add_importlibs(mstask_winetest ole32 user32 msvcrt kernel32)
add_rostests_file(TARGET mstask_winetest)
