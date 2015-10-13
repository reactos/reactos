
spec2def(freetype.dll freetype.spec ADD_IMPORTLIB)

add_definitions(
    -DFT2_BUILD_LIBRARY)

include_directories(${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/include)

list(APPEND SOURCE
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/autofit/autofit.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftbase.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftbbox.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftbdf.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftbitmap.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftcid.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftdebug.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftfntfmt.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftfstype.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftgasp.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftglyph.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftgxval.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftinit.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftlcdfil.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftmm.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftotval.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftpatent.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftpfr.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftstroke.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftsynth.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftsystem.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/fttype1.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/base/ftwinfnt.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/bdf/bdf.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/cache/ftcache.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/cff/cff.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/cid/type1cid.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/gxvalid/gxvalid.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/gzip/ftgzip.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/lzw/ftlzw.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/otvalid/otvalid.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/pcf/pcf.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/pfr/pfr.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/psaux/psaux.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/pshinter/pshinter.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/psnames/psnames.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/raster/raster.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/sfnt/sfnt.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/smooth/smooth.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/truetype/truetype.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/type1/type1.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/type42/type42.c
  ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/src/winfonts/winfnt.c)

add_library(freetype SHARED
    ${SOURCE}
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/freetype/freetype.rc
    ${CMAKE_CURRENT_BINARY_DIR}/freetype.def)

set_module_type(freetype win32dll UNICODE)
set_entrypoint(freetype 0)

#target_link_libraries(freetype freetype_s)

add_importlibs(freetype msvcrt kernel32 ntdll)

add_cd_file(TARGET freetype DESTINATION reactos/system32 FOR all)
