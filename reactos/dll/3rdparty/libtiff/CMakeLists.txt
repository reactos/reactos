
add_definitions(
    -DWIN32
    -DNDEBUG
    -DBUILD_LIBTIFF_DLL
    -DDLL_EXPORT
    -DUSE_WIN32_FILEIO)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/libtiff
    ${REACTOS_SOURCE_DIR}/include/reactos/libs/zlib)

list(APPEND SOURCE
    mkg3states.c
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
    tif_jbig.c
    tif_jpeg.c
    tif_luv.c
    tif_lzw.c
    tif_next.c
    tif_ojpeg.c
    tif_open.c
    tif_packbits.c
    tif_pixarlog.c
    tif_predict.c
    tif_print.c
    tif_read.c
    tif_strip.c
    tif_swab.c
    tif_thunder.c
    tif_tile.c
    tif_version.c
    tif_warning.c
    tif_win32.c
    tif_write.c
    tif_zip.c
    precomp.h)

add_library(libtiff SHARED ${SOURCE})
set_module_type(libtiff win32dll)
target_link_libraries(libtiff zlib getopt)
add_importlibs(libtiff user32 msvcrt kernel32 ntdll)
add_pch(libtiff precomp.h SOURCE)
add_cd_file(TARGET libtiff DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(libtiff "-Wno-unused-but-set-variable")
    endif()
endif()
