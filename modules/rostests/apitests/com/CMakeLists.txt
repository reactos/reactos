
add_definitions(-DWINE_NO_UNICODE_MACROS)

list(APPEND SOURCE
    browseui.c
    certmgr.c
    com_apitest.c
    ieframe.c
    netcfgx.c
    netshell.c
    ole32.c
    shdocvw.c
    shell32.c
    zipfldr.c)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(com_apitest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(com_apitest wine uuid)
set_module_type(com_apitest win32cui)
add_importlibs(com_apitest advapi32 ole32 shlwapi shell32 msvcrt kernel32 ntdll)
add_pch(com_apitest com_apitest.h "${PCH_SKIP_SOURCE}")
add_rostests_file(TARGET com_apitest)
