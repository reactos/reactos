
add_library(beep MODULE beep.c beep.rc)
set_module_type(beep kernelmodedriver)
add_importlibs(beep ntoskrnl hal)
add_cd_file(TARGET beep DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(beep_reg.inf)
