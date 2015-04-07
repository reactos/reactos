
add_definitions(
    -DOPT_I386
    -DREAL_IS_FLOAT
    -DNOXFERMEM
    -DHAVE_CONFIG_H
    -DEOVERFLOW=75
    -Dlseek=_lseek
    -Dread=_read
    -Dstrdup=_strdup
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D_DLL -D__USE_CRTIMP)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/libs/libmpg123)

list(APPEND SOURCE
    compat.c
    dct64.c
    dct64_i386.c
    equalizer.c
    feature.c
    format.c
    frame.c
    icy.c
    icy2utf8.c
    id3.c
    index.c
    layer1.c
    layer2.c
    layer3.c
    libmpg123.c
    ntom.c
    parse.c
    readers.c
    stringbuf.c
    synth.c
    synth_8bit.c
    synth_real.c
    synth_s32.c
    tabinit.c
    precomp.h)

add_library(libmpg123 ${SOURCE} optimize.c)
add_dependencies(libmpg123 psdk)
add_pch(libmpg123 precomp.h SOURCE)

if((NOT MSVC) AND (NOT CMAKE_C_COMPILER_ID STREQUAL "Clang"))
    add_target_compile_flags(libmpg123 "-Wno-unused-but-set-variable")
endif()
