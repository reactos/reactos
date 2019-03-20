
add_library(tapiui MODULE tapiui.rc)
set_module_type(tapiui win32dll ENTRYPOINT 0 )
add_dependencies(tapiui psdk)
add_cd_file(TARGET tapiui DESTINATION reactos/system32 FOR all)
