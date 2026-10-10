
list(APPEND SOURCE
    samplerate.c
    src_linear.c
    src_sinc.c
    src_zoh.c
    precomp.h)

add_library(libsamplerate STATIC ${SOURCE})
add_pch(libsamplerate precomp.h SOURCE)
add_dependencies(libsamplerate psdk)
target_include_directories(libsamplerate INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
