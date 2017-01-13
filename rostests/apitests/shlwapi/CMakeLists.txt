
list(APPEND SOURCE
    PathIsUNC.c
    PathUnExpandEnvStrings.c
    testlist.c)

add_executable(shlwapi_apitest ${SOURCE})
set_module_type(shlwapi_apitest win32cui)
add_importlibs(shlwapi_apitest shlwapi msvcrt kernel32)
add_cd_file(TARGET shlwapi_apitest DESTINATION reactos/bin FOR all)
