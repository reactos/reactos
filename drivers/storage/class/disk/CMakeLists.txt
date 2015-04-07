
include_directories(..)
add_library(disk SHARED disk.c disk.rc)
set_module_type(disk kernelmodedriver)
add_importlibs(disk class2 scsiport ntoskrnl hal)
add_cd_file(TARGET disk DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(disk_reg.inf)
