
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_library(blue SHARED blue.c font.c blue.rc)

add_pch(blue blue.h)
set_module_type(blue kernelmodedriver)
add_importlibs(blue ntoskrnl hal)
add_cd_file(TARGET blue DESTINATION reactos/system32/drivers NO_CAB FOR all)
