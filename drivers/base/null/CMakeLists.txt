


add_library(null SHARED null.c null.rc)

set_target_properties(null PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(null
    ${PSEH_LIB}
    -lntoskrnl
    -lhal)

add_dependencies(null psdk bugcodes)
add_cab_target(null 2)