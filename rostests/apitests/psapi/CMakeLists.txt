
list(APPEND SOURCE
    psapi.c
    testlist.c)

add_executable(psapi_apitest ${SOURCE})
target_link_libraries(psapi_apitest wine)
set_module_type(psapi_apitest win32cui)
add_importlibs(psapi_apitest psapi msvcrt kernel32 ntdll)
add_cd_file(TARGET psapi_apitest DESTINATION reactos/bin FOR all)
