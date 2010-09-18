


add_library(kbdclass SHARED
    kbdclass.c
    misc.c
    setup.c
    kbdclass.rc)

set_target_properties(kbdclass PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(kbdclass
    pseh
    -lntoskrnl
    -lhal)

add_dependencies(kbdclass psdk bugcodes)
