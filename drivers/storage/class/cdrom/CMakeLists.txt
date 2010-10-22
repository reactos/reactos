


include_directories(..)

add_library(cdrom SHARED cdrom.c findscsi.c cdrom.rc)

set_target_properties(cdrom PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(cdrom
    -lclass2
    -lscsiport
    -lntoskrnl
    -lhal)

add_dependencies(cdrom psdk bugcodes)
add_minicd_target(cdrom reactos cdrom.sys)
add_livecd_target(cdrom reactos/system32/drivers)