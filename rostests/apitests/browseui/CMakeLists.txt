
list(APPEND SOURCE
    SHExplorerParseCmdLine.c
    testlist.c)

add_executable(browseui_apitest ${SOURCE})
target_link_libraries(browseui_apitest wine)
set_module_type(browseui_apitest win32cui)
add_importlibs(browseui_apitest browseui shell32 msvcrt kernel32 ntdll)
add_cd_file(TARGET browseui_apitest DESTINATION reactos/bin FOR all)
