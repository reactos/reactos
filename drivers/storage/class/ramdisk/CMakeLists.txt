


include_directories(..)

add_library(ramdisk SHARED ramdisk.c ramdisk.rc)

set_target_properties(ramdisk PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(ramdisk
    -lntoskrnl
    -lhal)

add_dependencies(ramdisk psdk bugcodes)
add_minicd_target(ramdisk reactos ramdisk.sys)