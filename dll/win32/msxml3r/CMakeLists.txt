
add_library(msxml3r MODULE msxml3r.rc)
set_module_type(msxml3r win32dll ENTRYPOINT 0)
add_cd_file(TARGET msxml3r DESTINATION reactos/system32 FOR all)
