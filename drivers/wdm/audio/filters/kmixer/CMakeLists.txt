
include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/libsamplerate)

add_library(kmixer SHARED
    kmixer.c
    filter.c
    pin.c)

set_module_type(kmixer kernelmodedriver)

target_link_libraries(kmixer
    libcntpr
    libsamplerate)

add_pch(kmixer kmixer.h)
add_importlibs(kmixer ntoskrnl ks hal)
add_cd_file(TARGET kmixer DESTINATION reactos/system32/drivers FOR all)
