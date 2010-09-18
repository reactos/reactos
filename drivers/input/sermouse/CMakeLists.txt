


add_library(sermouse SHARED
    createclose.c
    detect.c
    fdo.c
    internaldevctl.c
    misc.c
    readmouse.c
    sermouse.c
    sermouse.rc)

set_target_properties(sermouse PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(sermouse
    -lntoskrnl
    -lhal)

add_dependencies(sermouse psdk bugcodes)
