
list(APPEND SOURCE
    psapi.c
    testlist.c)

add_executable(psapi_apitest ${SOURCE})
set_module_type(psapi_apitest win32cui)
add_importlibs(psapi_apitest psapi msvcrt kernel32)
add_cd_file(TARGET psapi_apitest DESTINATION reactos/bin FOR all)
