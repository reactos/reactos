
add_definitions(-DWINE_NO_UNICODE_MACROS)

list(APPEND SOURCE
    browseui.c
    ieframe.c
    netshell.c
    shdocvw.c
    shell32.c
    com_apitest.c
    testlist.c)

add_executable(com_apitest ${SOURCE})
target_link_libraries(com_apitest wine uuid)
set_module_type(com_apitest win32cui)
add_importlibs(com_apitest advapi32 ole32 shlwapi shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET com_apitest DESTINATION reactos/bin FOR all)
