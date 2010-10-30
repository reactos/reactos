


add_library(beep SHARED beep.c beep.rc)

set_target_properties(beep PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(beep
    -lntoskrnl
    -lhal)

add_dependencies(beep psdk bugcodes)
add_cab_target(beep 2)