


add_library(pcmcia SHARED
    fdo.c
    pcmcia.c
    pdo.c
    pcmcia.rc)

set_target_properties(pcmcia PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(pcmcia
    -lntoskrnl
    -lhal)

add_dependencies(pcmcia psdk bugcodes buildno_header)
