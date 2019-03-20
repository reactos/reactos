
add_library(null MODULE null.c null.rc)
target_link_libraries(null ${PSEH_LIB})
set_module_type(null kernelmodedriver)
add_importlibs(null ntoskrnl hal)
add_cd_file(TARGET null DESTINATION reactos/system32/drivers FOR all)
