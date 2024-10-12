
add_definitions(
    -DOPT_I386
    -DREAL_IS_FLOAT
    -DNOXFERMEM
    -DHAVE_CONFIG_H
    -DEOVERFLOW=75
    -Dstrcasecmp=_stricmp
    -Dstrncasecmp=_strnicmp
    -D_DLL -D__USE_CRTIMP
    -D_CRT_NO_POSIX_ERROR_CODES)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libmpg123)

list(APPEND SOURCE
    compat/compat.c
    compat/compat_str.c
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
    tabinit.c)

list(APPEND PCH_SKIP_SOURCE
    optimize.c)

add_library(libmpg123
    ${SOURCE}
    ${PCH_SKIP_SOURCE})

target_link_libraries(libmpg123 oldnames)
add_dependencies(libmpg123 psdk)
add_pch(libmpg123 precomp.h "${PCH_SKIP_SOURCE}")
