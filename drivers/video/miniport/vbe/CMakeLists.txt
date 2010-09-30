
add_library(vbemp SHARED
    edid.c
    vbemp.c
    vbemp.rc)

set_target_properties(vbemp PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native -Wl,--exclude-all-symbols" SUFFIX ".sys")

add_importlibs(vbemp videoprt)
add_dependencies(vbemp psdk bugcodes)
