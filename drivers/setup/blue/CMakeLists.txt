
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_library(blue SHARED blue.c font.c blue.rc)

set_module_type(blue kernelmodedriver)
add_importlibs(blue ntoskrnl hal)

add_minicd_target(blue reactos blue.sys)
add_cab_target(blue 2)
