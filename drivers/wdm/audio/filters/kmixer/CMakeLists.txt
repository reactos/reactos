
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/libsamplerate)

list(APPEND SOURCE
    kmixer.c
    filter.c
    pin.c
    kmixer.h)

add_library(kmixer MODULE ${SOURCE})
set_module_type(kmixer kernelmodedriver)
target_link_libraries(kmixer libcntpr libsamplerate)
add_pch(kmixer kmixer.h SOURCE)
add_importlibs(kmixer ntoskrnl ks hal)
add_cd_file(TARGET kmixer DESTINATION reactos/system32/drivers FOR all)
