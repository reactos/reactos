


add_library(isapnp SHARED
    isapnp.c
    pdo.c
    fdo.c
    hardware.c
    isapnp.rc)

set_target_properties(isapnp PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(isapnp
    -lntoskrnl
    -lhal)

add_dependencies(isapnp psdk bugcodes)
