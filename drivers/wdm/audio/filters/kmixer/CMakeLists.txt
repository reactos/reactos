
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/libsamplerate)

add_library(kmixer SHARED
    kmixer.c
    filter.c
    pin.c)

set_target_properties(kmixer PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(kmixer
    libcntpr
    libsamplerate)

add_importlibs(kmixer ntoskrnl ks hal)
add_dependencies(kmixer psdk bugcodes)
