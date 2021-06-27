
add_library(themeui MODULE themeui.rc)
add_dependencies(themeui psdk)
set_module_type(themeui win32dll ENTRYPOINT 0)
add_cd_file(TARGET themeui DESTINATION reactos/system32 FOR all)
