


add_library(atapi SHARED atapi.c atapi.rc)

set_target_properties(atapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(atapi
    -lscsiport
    libcntpr)

add_dependencies(atapi psdk bugcodes)
add_minicd_target(atapi reactos atapi.sys)