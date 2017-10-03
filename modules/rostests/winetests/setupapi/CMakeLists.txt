
list(APPEND SOURCE
    devinst.c
    dialog.c
    diskspace.c
    install.c
    misc.c
    parser.c
    query.c
    setupcab.c
    stringtable.c
    testlist.c)

add_executable(setupapi_winetest ${SOURCE} setupapi.rc)
set_module_type(setupapi_winetest win32cui)
add_importlibs(setupapi_winetest advapi32 setupapi user32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET setupapi_winetest)
