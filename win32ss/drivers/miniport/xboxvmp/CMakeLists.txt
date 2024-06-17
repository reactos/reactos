
add_library(xboxvmp MODULE xboxvmp.c xboxvmp.rc)
set_module_type(xboxvmp kernelmodedriver)
add_importlibs(xboxvmp ntoskrnl videoprt)
add_driver_inf(xboxvmp xboxdisp.inf)
