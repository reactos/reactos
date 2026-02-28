
set(SOURCES
    src/autofit/autofit.c
    src/base/ftbase.c
    src/base/ftbbox.c
    src/base/ftbdf.c
    src/base/ftbitmap.c
    src/base/ftcid.c
    src/base/ftfstype.c
    src/base/ftgasp.c
    src/base/ftglyph.c
    src/base/ftgxval.c
    src/base/ftinit.c
    src/base/ftmm.c
    src/base/ftotval.c
    src/base/ftpatent.c
    src/base/ftpfr.c
    src/base/ftstroke.c
    src/base/ftsynth.c
    src/base/fttype1.c
    src/base/ftwinfnt.c
    src/bdf/bdf.c
    src/bzip2/ftbzip2.c
    src/cache/ftcache.c
    src/cff/cff.c
    src/cid/type1cid.c
    src/gzip/ftgzip.c
    src/lzw/ftlzw.c
    src/pcf/pcf.c
    src/pfr/pfr.c
    src/psaux/psaux.c
    src/pshinter/pshinter.c
    src/psnames/psnames.c
    src/raster/raster.c
    src/sfnt/sfnt.c
    src/smooth/smooth.c
    src/truetype/truetype.c
    src/type1/type1.c
    src/type42/type42.c
    src/winfonts/winfnt.c

    src/base/ftsystem.c
    builds/windows/ftdebug.c
)

add_library(freetype STATIC ${SOURCES})
target_compile_definitions(freetype PRIVATE FT2_BUILD_LIBRARY)
target_compile_definitions(freetype PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
target_include_directories(freetype PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC" AND (ARCH STREQUAL "amd64" OR ARCH STREQUAL "arm64"))
    # error C4312: 'type cast': conversion from 'unsigned long' to 'void *' of greater size
    remove_target_compile_option(freetype "/we4312")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(freetype PRIVATE -fno-builtin-malloc)
endif()
