
list(APPEND SOURCE
    cmd.c
    fc.c
    testlist.c)

add_executable(cmd_apitest ${SOURCE})
target_link_libraries(cmd_apitest wine ${PSEH_LIB})
set_module_type(cmd_apitest win32cui)
add_importlibs(cmd_apitest shlwapi msvcrt kernel32)
add_rostests_file(TARGET cmd_apitest)
