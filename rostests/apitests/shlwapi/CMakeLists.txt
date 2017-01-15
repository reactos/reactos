
list(APPEND SOURCE
    PathIsUNC.c
    PathIsUNCServer.c
    PathIsUNCServerShare.c
    PathUnExpandEnvStrings.c
    StrFormatByteSizeW.c
    testlist.c)

add_executable(shlwapi_apitest ${SOURCE})
set_module_type(shlwapi_apitest win32cui)
add_importlibs(shlwapi_apitest shlwapi msvcrt kernel32)
add_rostests_file(TARGET shlwapi_apitest)
