
add_library(pciide SHARED pciide.c pciide.rc)

set_target_properties(pciide PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(pciide pciidex ntoskrnl)
add_dependencies(pciide psdk bugcodes)
add_cab_target(pciide 2)