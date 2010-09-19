
include_directories(BEFORE include)

list(APPEND SOURCE
    ne2000/8390.c
    ne2000/main.c
    ne2000.rc)

add_library(ne2000 SHARED ${SOURCE})

set_target_properties(ne2000 PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(ne2000
    -lndis
    -lntoskrnl
    -lhal)

add_dependencies(ne2000 psdk bugcodes buildno_header)
