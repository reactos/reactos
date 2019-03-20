
add_library(nmidebug MODULE nmidebug.c nmidebug.rc)
set_module_type(nmidebug kernelmodedriver)
add_importlibs(nmidebug ntoskrnl hal)
add_cd_file(TARGET nmidebug DESTINATION reactos/system32/drivers NO_CAB FOR all)
