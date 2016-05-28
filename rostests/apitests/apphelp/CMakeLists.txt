project(appcompat)
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    apphelp.c
    data.c
    db.c
    layerapi.c
    testlist.c)

add_executable(apphelp_apitest ${SOURCE})
set_module_type(apphelp_apitest win32cui)
add_importlibs(apphelp_apitest advapi32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET apphelp_apitest DESTINATION reactos/bin FOR all)
