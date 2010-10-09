


include_directories(..)

add_library(disk SHARED disk.c disk.rc)

set_target_properties(disk PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(disk
    -lclass2
    -lscsiport
    -lntoskrnl
    -lhal)

add_dependencies(disk psdk bugcodes)
add_minicd_target(disk reactos disk.sys)