
add_library(pciide MODULE pciide.c pciide.rc)
set_module_type(pciide kernelmodedriver)
add_importlibs(pciide pciidex ntoskrnl)
add_cd_file(TARGET pciide DESTINATION reactos/system32/drivers NO_CAB FOR all)
