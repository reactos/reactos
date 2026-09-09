
add_definitions(-DDEBUG_MODE)
add_library(mouhid MODULE mouhid.c mouhid.rc)
set_module_type(mouhid kernelmodedriver)
add_importlibs(mouhid ntoskrnl hal hidparse)
add_cd_file(TARGET mouhid DESTINATION reactos/system32/drivers FOR all)
