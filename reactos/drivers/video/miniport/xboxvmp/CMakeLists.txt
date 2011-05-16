
add_library(xboxvmp SHARED xboxvmp.c xboxvmp.rc)
set_module_type(xboxvmp kernelmodedriver)
add_importlibs(xboxvmp ntoskrnl videoprt)
