project(appcompat)
add_definitions(-D__ROS_LONG64__ -DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    apphelp.c
    data.c
    db.cpp
    layerapi.c
    testlist.c)

add_executable(apphelp_apitest ${SOURCE})
set_module_type(apphelp_apitest win32cui)
add_importlibs(apphelp_apitest advapi32 userenv version shlwapi msvcrt kernel32 ntdll)
add_rostests_file(TARGET apphelp_apitest)
