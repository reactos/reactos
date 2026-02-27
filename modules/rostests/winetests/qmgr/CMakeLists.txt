
add_definitions(-DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    enum_files.c
    enum_jobs.c
    file.c
    job.c
    qmgr.c
    testlist.c)

add_executable(qmgr_winetest ${SOURCE})
set_module_type(qmgr_winetest win32cui)
add_importlibs(qmgr_winetest ole32 shlwapi user32 msvcrt kernel32)

if(MSVC)
    add_importlibs(qmgr_winetest ntdll)
endif()

add_rostests_file(TARGET qmgr_winetest)
