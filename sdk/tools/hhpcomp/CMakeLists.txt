
list(APPEND SOURCE
    hhpcomp.cpp
    hhp_reader.cpp
    utils.cpp
    chmc/chmc.c
    chmc/err.c
    lzx_compress/lz_nonslide.c
    lzx_compress/lzx_layer.c
    ../port/mkstemps.c)

# used by lzx_compress
add_definitions(-DNONSLIDE -D_CRT_NONSTDC_NO_DEPRECATE)

add_executable(hhpcomp ${SOURCE})
target_link_libraries(hhpcomp)

if(MSVC)
    # Disable warning "'x': unreferenced local variable"
    add_target_compile_flags(hhpcomp "/wd4101")

    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    add_target_compile_flags(hhpcomp "/wd4244")
endif()
