
add_library(themeui SHARED themeui.rc)
set_module_type(themeui win32dll ENTRYPOINT 0)
add_registry_inf(themeui_reg.inf)
add_cd_file(TARGET themeui DESTINATION reactos/system32 FOR all)
