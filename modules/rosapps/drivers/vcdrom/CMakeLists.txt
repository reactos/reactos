
add_library(vcdrom MODULE vcdrom.c vcdrom.rc)
set_module_type(vcdrom kernelmodedriver)
target_link_libraries(vcdrom ${PSEH_LIB})
add_importlibs(vcdrom ntoskrnl hal)
add_cd_file(TARGET vcdrom DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(vcdrom_reg.inf)
