


add_library(buslogic SHARED BusLogic958.c BusLogic958.rc)

set_target_properties(buslogic PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(buslogic
    -lscsiport
    libcntpr)

add_dependencies(buslogic psdk bugcodes)
add_minicd_target(buslogic reactos buslogic.sys)