
add_library(atapi MODULE atapi.c atapi.rc)
target_link_libraries(atapi libcntpr)
set_module_type(atapi kernelmodedriver)
add_importlibs(atapi scsiport)
add_cd_file(TARGET atapi DESTINATION reactos/system32/drivers NO_CAB FOR all)
