
list(APPEND SOURCE
    PathFindOnPath.c
    PathIsUNC.c
    PathIsUNCServer.c
    PathIsUNCServerShare.c
    PathUnExpandEnvStrings.c
    PathUnExpandEnvStringsForUser.c
    SHAreIconsEqual.c
    StrFormatByteSizeW.c
    testdata.rc
    testlist.c)

add_executable(shlwapi_apitest ${SOURCE})
set_module_type(shlwapi_apitest win32cui)
target_link_libraries(shlwapi_apitest ${PSEH_LIB})
add_importlibs(shlwapi_apitest shlwapi user32 msvcrt kernel32)
add_rostests_file(TARGET shlwapi_apitest)
