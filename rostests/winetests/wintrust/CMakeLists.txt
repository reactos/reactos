
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    asn.c
    crypt.c
    register.c
    softpub.c
    testlist.c)

add_executable(wintrust_winetest ${SOURCE})
set_module_type(wintrust_winetest win32cui)
add_importlibs(wintrust_winetest wintrust crypt32 advapi32 msvcrt kernel32)
add_cd_file(TARGET wintrust_winetest DESTINATION reactos/bin FOR all)
