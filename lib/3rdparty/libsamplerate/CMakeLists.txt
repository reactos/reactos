
add_definitions(
    -DHAVE_LRINT
    -DHAVE_LRINTF)

list(APPEND SOURCE
    samplerate.c
    src_linear.c
    src_sinc.c
    src_zoh.c)

add_library(libsamplerate ${SOURCE})
