
add_executable(user32_dynamic_apitest load.c testlist.c)
set_module_type(user32_dynamic_apitest win32cui)
add_importlibs(user32_dynamic_apitest msvcrt kernel32)
add_cd_file(TARGET user32_dynamic_apitest DESTINATION reactos/bin FOR all)
