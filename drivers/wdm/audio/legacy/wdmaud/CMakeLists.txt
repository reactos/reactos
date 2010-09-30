
add_definitions(-D_COMDDK_)

include_directories(${REACTOS_SOURCE_DIR}/lib/drivers/sound/mmixer)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/sound)

add_library(wdmaud SHARED
    control.c
    deviface.c
    entry.c
    mmixer.c
    sup.c
    wdmaud.rc)

set_target_properties(wdmaud PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(wdmaud
    mmixer
    libcntpr
    pseh)

add_importlibs(wdmaud ntoskrnl ks hal)
add_dependencies(wdmaud psdk bugcodes)
