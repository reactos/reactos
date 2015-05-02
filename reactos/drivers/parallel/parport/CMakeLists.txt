
add_library(parport SHARED parport.c parport.rc)
set_module_type(parport kernelmodedriver)
add_importlibs(parport ntoskrnl hal)
add_cd_file(TARGET parport DESTINATION reactos/system32/drivers FOR all)
