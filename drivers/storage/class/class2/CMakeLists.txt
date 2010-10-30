


include_directories(..)

spec2def(class2.sys class2.spec)

add_library(class2 SHARED class2.c class2.rc
    ${CMAKE_CURRENT_BINARY_DIR}/class2.def)

set_target_properties(class2 PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(class2
    -lscsiport
    -lntoskrnl
    -lhal)

add_dependencies(class2 psdk bugcodes)
add_minicd_target(class2 reactos class2.sys)
add_livecd_target(class2 reactos/system32/drivers)