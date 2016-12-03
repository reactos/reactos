project(appcompat)
add_definitions(-D__ROS_LONG64__)

list(APPEND SOURCE
    dispmode.c
    versionlie.c
    testlist.c)

add_executable(appshim_apitest ${SOURCE})
set_module_type(appshim_apitest win32cui)
add_importlibs(appshim_apitest msvcrt kernel32 ntdll)
add_cd_file(TARGET appshim_apitest DESTINATION reactos/bin FOR all)
