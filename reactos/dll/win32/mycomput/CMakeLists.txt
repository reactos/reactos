
add_library(mycomput SHARED
    mycomput.rc)

set_module_type(mycomput win32dll UNICODE)
add_cd_file(TARGET mycomput DESTINATION reactos/system32 FOR all)
