


add_library(nmidebug SHARED nmidebug.c nmidebug.rc)

set_target_properties(nmidebug PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(nmidebug
    -lntoskrnl
    -lhal)

add_dependencies(nmidebug psdk bugcodes)
add_livecd_target(nmidebug reactos/system32/drivers)