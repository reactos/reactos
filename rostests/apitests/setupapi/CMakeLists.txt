
add_executable(setupapi_apitest devclass.c testlist.c)
set_module_type(setupapi_apitest win32cui)
add_importlibs(setupapi_apitest advapi32 setupapi msvcrt kernel32)
add_cd_file(TARGET setupapi_apitest DESTINATION reactos/bin FOR all)
