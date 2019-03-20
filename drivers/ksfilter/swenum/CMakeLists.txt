
add_library(swenum MODULE swenum.c swenum.rc)
set_module_type(swenum kernelmodedriver)
add_importlibs(swenum ks ntoskrnl hal)
add_cd_file(TARGET swenum DESTINATION reactos/system32/drivers NO_CAB FOR all)
