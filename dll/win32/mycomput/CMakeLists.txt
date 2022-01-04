
add_library(mycomput MODULE
    mycomput.rc)
add_dependencies(mycomput psdk)
set_module_type(mycomput win32dll UNICODE ENTRYPOINT 0)

add_cd_file(TARGET mycomput DESTINATION reactos/system32 FOR all)
