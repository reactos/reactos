


add_definitions(-D_BATTERYCLASS_)

add_library(battc SHARED battc.c battc.rc)

set_target_properties(battc PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(battc
    -lntoskrnl
    -lhal)

add_dependencies(battc psdk bugcodes)
