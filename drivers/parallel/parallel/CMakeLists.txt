
add_library(parallel SHARED parallel.c parallel.rc)

set_target_properties(parallel PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(parallel ntoskrnl hal)
add_dependencies(parallel psdk bugcodes)
