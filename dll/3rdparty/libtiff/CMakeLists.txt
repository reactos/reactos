
add_definitions(
    -DWIN32
    -DNDEBUG=
    -DUSE_WIN32_FILEIO
    -DTIF_PLATFORM_CONSOLE)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/libtiff
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)

spec2def(libtiff.dll libtiff.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    #mkg3states.c
    tif_aux.c
    tif_close.c
    tif_codec.c
    tif_color.c
    tif_compress.c
    tif_dir.c
    tif_dirinfo.c
    tif_dirread.c
    tif_dirwrite.c
    tif_dumpmode.c
    tif_error.c
    tif_extension.c
    tif_fax3.c
    tif_fax3sm.c
    tif_flush.c
    tif_getimage.c
    #tif_jbig.c
    #tif_jpeg.c
    tif_luv.c
    #tif_lzma.c
    tif_lzw.c
    tif_next.c
    #tif_ojpeg.c
    tif_open.c
    tif_packbits.c
    tif_pixarlog.c
    tif_predict.c
    tif_print.c
    tif_read.c
    #tif_stream.cxx
    tif_strip.c
    tif_swab.c
    tif_thunder.c
    tif_tile.c
    tif_version.c
    tif_warning.c
    #tif_webp.c
    tif_win32.c
    tif_write.c
    tif_zip.c
    #tif_zstd.c
    precomp.h)

add_library(libtiff MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/libtiff.def)

set_module_type(libtiff win32dll)
target_link_libraries(libtiff zlib getopt)
add_importlibs(libtiff user32 msvcrt kernel32 ntdll)
add_pch(libtiff precomp.h SOURCE)
add_cd_file(TARGET libtiff DESTINATION reactos/system32 FOR all)

if(MSVC)
    # error C4311: 'type cast': pointer truncation from 'thandle_t' to 'int'
    remove_target_compile_option(libtiff "/we4311")
    # error C4312: 'type cast': conversion from 'int' to 'thandle_t' of greater size
    remove_target_compile_option(libtiff "/we4312")
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(libtiff PRIVATE -Wno-unused-but-set-variable)
endif()
