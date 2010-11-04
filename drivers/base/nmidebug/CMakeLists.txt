
add_library(nmidebug SHARED nmidebug.c nmidebug.rc)

set_module_type(nmidebug kernelmodedriver)
add_importlibs(nmidebug ntoskrnl hal)

add_minicd_target(nmidebug reactos)
add_cab_target(nmidebug 2)
