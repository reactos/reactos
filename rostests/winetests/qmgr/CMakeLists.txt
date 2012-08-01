
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    enum_files.c
    enum_jobs.c
    file.c
    job.c
    qmgr.c
    testlist.c)

add_executable(qmgr_winetest ${SOURCE})
target_link_libraries(qmgr_winetest wine)
set_module_type(qmgr_winetest win32cui)
add_importlibs(qmgr_winetest ole32 shlwapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET qmgr_winetest DESTINATION reactos/bin FOR all)
