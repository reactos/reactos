


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

add_library(blue SHARED blue.c font.c blue.rc)

set_target_properties(blue PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(blue
    -lntoskrnl
    -lhal)

add_dependencies(blue psdk bugcodes)
add_minicd_target(blue reactos blue.sys)