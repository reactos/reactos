
add_library(pciide SHARED pciide.c pciide.rc)

set_module_type(pciide kernelmodedriver)
add_importlibs(pciide pciidex ntoskrnl)

add_cab_target(pciide 2)
