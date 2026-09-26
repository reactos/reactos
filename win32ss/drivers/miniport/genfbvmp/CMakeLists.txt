
add_library(genfbvmp MODULE genfbvmp.c genfbvmp.rc)
set_module_type(genfbvmp kernelmodedriver)
add_importlibs(genfbvmp ntoskrnl videoprt hal)
target_link_libraries(genfbvmp libcntpr)
add_cd_file(TARGET genfbvmp DESTINATION reactos/system32/drivers FOR all)
add_registry_inf(genfbvmp_reg.inf)
add_driver_inf(genfbvmp genfbvmp.inf)
