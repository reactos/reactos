


add_library(mouclass SHARED
    misc.c
    mouclass.c
    mouclass.rc)

set_target_properties(mouclass PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(mouclass
    pseh
    -lntoskrnl
    -lhal)

add_dependencies(mouclass psdk bugcodes)
add_livecd_target(mouclass reactos/system32/drivers)