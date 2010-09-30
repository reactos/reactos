
add_library(xboxvmp SHARED xboxvmp.c xboxvmp.rc)

set_target_properties(xboxvmp PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

add_importlibs(xboxvmp ntoskrnl videoprt)
add_dependencies(xboxvmp psdk bugcodes)
