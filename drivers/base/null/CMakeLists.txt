
add_library(null SHARED null.c null.rc)

target_link_libraries(null ${PSEH_LIB})

set_module_type(null kernelmodedriver)
add_importlibs(null ntoskrnl hal)

add_cab_target(null 2)
