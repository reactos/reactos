
add_library(parallel SHARED parallel.c parallel.rc)

set_module_type(parallel kernelmodedriver)
add_importlibs(parallel ntoskrnl hal)
