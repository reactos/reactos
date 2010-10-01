
add_library(mup SHARED
    create.c
    mup.c
    mup.rc)

set_target_properties(mup PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

add_importlibs(mup ntoskrnl hal)
add_dependencies(mup psdk bugcodes)
