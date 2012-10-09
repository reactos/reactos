
add_subdirectory(CmdLineUtil)

list(APPEND SOURCE
    CommandLine.c
    ieee.c
    splitpath.c
    testlist.c)

add_executable(msvcrt_apitest ${SOURCE})
target_link_libraries(msvcrt_apitest wine)
set_module_type(msvcrt_apitest win32cui)
add_importlibs(msvcrt_apitest msvcrt kernel32)
add_cd_file(TARGET msvcrt_apitest DESTINATION reactos/bin FOR all)
