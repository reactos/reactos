
list(APPEND SOURCE
    devclass.c
    SetupDiInstallClassExA.c
    SetupInstallServicesFromInfSectionEx.c
    testlist.c)

add_executable(setupapi_apitest ${SOURCE})
set_module_type(setupapi_apitest win32cui)
add_importlibs(setupapi_apitest advapi32 setupapi user32 msvcrt kernel32)
add_cd_file(TARGET setupapi_apitest DESTINATION reactos/bin FOR all)
