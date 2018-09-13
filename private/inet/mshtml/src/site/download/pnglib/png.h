
/* png.h - header file for png reference library

   libpng 1.0 beta 2 - version 0.88
   Jan 25, 1996

   Note: This is a beta version.  It reads and writes valid files
   on the platforms I have, but it has had limited portability
   testing.  Furthermore, you may have to modify the
   includes below to get it to work on your system, and you
   may have to supply the correct compiler flags in the makefile.
   Read the readme.txt for more information, and how to contact
   me if you have any problems, or if you want your compiler/
   platform to be supported in the next official libpng release.

   See readme.txt for more information

   Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
   Contributing Authors:
      Andreas Dilger
      Dave Martindale
      Guy Eric Schalnat
      Paul Schmidt
      Tim Wegner

   The contributing authors would like to thank all those who helped
   with testing, bug fixes, and patience.  You know who you are.  This
   wouldn't have been possible without all of you.

   Thanks to Frank J. T. Wojcik for helping with the documentation

   The PNG Reference Library is supplied "AS IS". The Contributing Authors
   and Group 42, Inc. disclaim all warranties, expressed or implied,
   including, without limitation, the warranties of merchantability and of
   fitness for any purpose. The Contributing Authors and Group 42, Inc.
   assume no liability for damages, direct or consequential, which may
   result from the use of the PNG Reference Library.

   Permission is hereby granted to use, copy, modify, and distribute this
   source code, or portions hereof, for any purpose, without fee, subject
   to the following restrictions:
   1. The origin of this source code must not be misrepresented.
   2. Altered versions must be plainly marked as such and must not be
     misrepresented as being the original source.
   3. This Copyright notice may not be removed or altered from any source or
     altered source distribution.

   The Contributing Authors and Group 42, Inc. specifically permit, without
   fee, and encourage the use of this source code as a component to
   supporting the PNG file format in commercial products. If you use this
   source code in a product, acknowledgment is not required but would be
   appreciated.
   */

#ifndef _PNG_H
#define _PNG_H

#include <windows.h>

/* This is not the place to learn how to use libpng.  The file libpng.txt
   describes how to use libpng, and the file example.c summarizes it
   with some code to build around.  This file is useful for looking
   at the actual function definitions and structure components. */

/* include the compression library's header */
#include "zlib.h"

/* include all user configurable info */
#include "pngconf.h"

/* This file is arranged in several sections.  The first section details
   the functions most users will use.  The third section describes the
   stub files that users will most likely need to change.  The last
   section contains functions used internally by the code.
   */

/* version information for png.h - this should match the version
   number in png.c */
#define PNG_LIBPNG_VER_STRING "0.88"
/* careful here.  I wanted to use 088, but that would be octal.  Version
   1.0 will be 100 here, etc. */
#define PNG_LIBPNG_VER 88

/* variables defined in png.c - only it needs to define PNG_NO_EXTERN */
#ifndef PNG_NO_EXTERN
/* version information for c files, stored in png.c. This better match
   the version above. */
extern char png_libpng_ver[];
#endif

/* three color definitions.  The order of the red, green, and blue, (and the
   exact size) is not important, although the size of the fields need to
   be png_byte or png_uint_16 (as defined below).  While png_color_8 and
   png_color_16 have more fields then they need, they are never used in
   arrays, so the size isn't that important.  I thought about using
   unions, but it looked too clumsy, so I left it. If you're using C++,
   you can union red, index, and gray, if you really want too. */
typedef struct png_color_struct
{
   png_byte red;
   png_byte green;
   png_byte blue;
} png_color;
typedef png_color       FAR *    png_colorp;
typedef png_color       FAR * FAR * png_colorpp;

typedef struct png_color_16_struct
{
   png_byte index; /* used for palette files */
   png_uint_16 red; /* for use in red green blue files */
   png_uint_16 green;
   png_uint_16 blue;
   png_uint_16 gray; /* for use in grayscale files */
} png_color_16;
typedef png_color_16    FAR *    png_color_16p;
typedef png_color_16    FAR * FAR * png_color_16pp;

typedef struct png_color_8_struct
{
   png_byte red; /* for use in red green blue files */
   png_byte green;
   png_byte blue;
   png_byte gray; /* for use in grayscale files */
   png_byte alpha; /* for alpha channel files */
} png_color_8;
typedef png_color_8     FAR *    png_color_8p;
typedef png_color_8     FAR * FAR * png_color_8pp;

/* png_text holds the text in a png file, and whether they are compressed
   or not.  If compression is -1, the text is not compressed.  */
typedef struct png_text_struct
{
   int compression; /* compression value, -1 if uncompressed */
   png_charp key; /* keyword */
   png_charp text; /* comment */
   png_uint_32 text_length; /* length of text field */
} png_text;
typedef png_text        FAR *    png_textp;
typedef png_text        FAR * FAR * png_textpp;

/* png_time is a way to hold the time in an machine independent way.
   Two conversions are provided, both from time_t and struct tm.  There
   is no portable way to convert to either of these structures, as far
   as I know.  If you know of a portable way, send it to me. */
typedef struct png_time_struct
{
   png_uint_16 year; /* full year, as in, 1995 */
   png_byte month; /* month of year, 1 - 12 */

   png_byte day; /* day of month, 1 - 31 */
   png_byte hour; /* hour of day, 0 - 23 */
   png_byte minute; /* minute of hour, 0 - 59 */
   png_byte second; /* second of minute, 0 - 60 (for leap seconds) */
} png_time;
typedef png_time        FAR *    png_timep;
typedef png_time        FAR * FAR * png_timepp;

/* png_info is a structure that holds the information in a png file.
   If you are reading the file, This structure will tell you what is
   in the png file.  If you are writing the file, fill in the information
   you want to put into the png file, then call png_write_info().
   The names chosen should be very close to the PNG
   specification, so consult that document for information
   about the meaning of each field. */
typedef struct png_info_struct
{
   /* the following are necessary for every png file */
   png_uint_32 width; /* with of file */
   png_uint_32 height; /* height of file */
   png_byte bit_depth; /* 1, 2, 4, 8, or 16 */
   png_byte color_type; /* use the PNG_COLOR_TYPE_ defines */
   png_byte compression_type; /* must be 0 */
   png_byte filter_type; /* must be 0 */
   png_byte interlace_type; /* 0 for non-interlaced, 1 for interlaced */
   png_uint_32 valid; /* the PNG_INFO_ defines, OR'd together */
   /* the following is informational only on read, and not used on
      writes */
   png_byte channels; /* number of channels of data per pixel */
   png_byte pixel_depth; /* number of bits per pixel */

   png_uint_32 rowbytes; /* bytes needed for untransformed row */
   /* the rest are optional.  If you are reading, check the valid
      field to see if the information in these are valid.  If you
      are writing, set the valid field to those chunks you want
      written, and initialize the appropriate fields below */
#if defined(PNG_READ_gAMA_SUPPORTED) || defined(PNG_WRITE_gAMA_SUPPORTED)
   float gamma; /* gamma value of file, if gAMA chunk is valid */
#endif
#if defined(PNG_READ_sBIT_SUPPORTED) || defined(PNG_WRITE_sBIT_SUPPORTED)
   png_color_8 sig_bit; /* significant bits */
#endif
#if defined(PNG_READ_cHRM_SUPPORTED) || defined(PNG_WRITE_cHRM_SUPPORTED)
   float x_white; /* cHRM chunk values */
   float y_white;
   float x_red;
   float y_red;
   float x_green;
   float y_green;
   float x_blue;
   float y_blue;
#endif
   png_colorp palette; /* palette of file */
   png_uint_16 num_palette; /* number of values in palette */
   png_uint_16 num_trans; /* number of trans values */
#if defined(PNG_READ_tRNS_SUPPORTED) || defined(PNG_WRITE_tRNS_SUPPORTED)
   png_bytep trans; /* tRNS values for palette image */
   png_color_16 trans_values; /* tRNS values for non-palette image */
#endif
#if defined(PNG_READ_bKGD_SUPPORTED) || defined(PNG_WRITE_bKGD_SUPPORTED)

   png_color_16 background; /* background color of image */
#endif
#if defined(PNG_READ_hIST_SUPPORTED) || defined(PNG_WRITE_hIST_SUPPORTED)
   png_uint_16p hist; /* histogram of palette usage */
#endif
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
   png_uint_32 x_pixels_per_unit; /* x resolution */
   png_uint_32 y_pixels_per_unit; /* y resolution */
   png_byte phys_unit_type; /* resolution type */
#endif
#if defined(PNG_READ_oFFs_SUPPORTED) || defined(PNG_WRITE_oFFs_SUPPORTED)
   png_uint_32 x_offset; /* x offset on page */
   png_uint_32 y_offset; /* y offset on page */
   png_byte offset_unit_type; /* offset units type */
#endif
#if defined(PNG_READ_tIME_SUPPORTED) || defined(PNG_WRITE_tIME_SUPPORTED)
   png_time mod_time; /* modification time */
#endif
#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED) || \
    defined(PNG_READ_zTXt_SUPPORTED) || defined(PNG_WRITE_zTXt_SUPPORTED)
   int num_text; /* number of comments */
   int max_text; /* size of text array */
   png_textp text; /* array of comments */
#endif
} png_info;
typedef png_info        FAR *    png_infop;
typedef png_info        FAR * FAR * png_infopp;

#define PNG_RESOLUTION_UNKNOWN 0
#define PNG_RESOLUTION_METER 1

#define PNG_OFFSET_PIXEL 0
#define PNG_OFFSET_MICROMETER 1

/* these describe the color_type field in png_info */

/* color type masks */
#define PNG_COLOR_MASK_PALETTE 1
#define PNG_COLOR_MASK_COLOR 2
#define PNG_COLOR_MASK_ALPHA 4

/* color types.  Note that not all combinations are legal */
#define PNG_COLOR_TYPE_PALETTE \
   (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE)
#define PNG_COLOR_TYPE_RGB (PNG_COLOR_MASK_COLOR)
#define PNG_COLOR_TYPE_GRAY 0
#define PNG_COLOR_TYPE_RGB_ALPHA \
   (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_ALPHA)
#define PNG_COLOR_TYPE_GRAY_ALPHA (PNG_COLOR_MASK_ALPHA)

/* These determine if a chunks information is present in a read operation, or
   if the chunk should be written in a write operation.  */
#define PNG_INFO_gAMA 0x0001
#define PNG_INFO_sBIT 0x0002
#define PNG_INFO_cHRM 0x0004
#define PNG_INFO_PLTE 0x0008
#define PNG_INFO_tRNS 0x0010
#define PNG_INFO_bKGD 0x0020
#define PNG_INFO_hIST 0x0040
#define PNG_INFO_pHYs 0x0080
#define PNG_INFO_oFFs 0x0100
#define PNG_INFO_tIME 0x0200

/* these determine if a function in the info needs to be freed */
#define PNG_FREE_PALETTE 0x0001
#define PNG_FREE_HIST 0x0002
#define PNG_FREE_TRANS 0x0004

/* this is used for the transformation routines, as some of them
   change these values for the row.  It also should enable using
   the routines for other uses. */
typedef struct png_row_info_struct
{
   png_uint_32 width; /* width of row */
   png_uint_32 rowbytes; /* number of bytes in row */
   png_byte color_type; /* color type of row */
   png_byte bit_depth; /* bit depth of row */
   png_byte channels; /* number of channels (1, 2, 3, or 4) */
   png_byte pixel_depth; /* bits per pixel (depth * channels) */
} png_row_info;

typedef png_row_info    FAR *    png_row_infop;
typedef png_row_info    FAR * FAR * png_row_infopp;

/* These are the function types for the I/O functions, and the functions which
 * modify the default I/O functions to user I/O functions.  The png_msg_ptr
 * type should match that of user supplied warning and error functions, while
 * the png_rw_ptr type should match that of the user read/write data functions.
 */
typedef struct png_struct_def png_struct;
typedef png_struct FAR * png_structp;

typedef void (*png_msg_ptr) PNGARG((png_structp, png_const_charp));
typedef void (*png_rw_ptr) PNGARG((png_structp, png_bytep, png_uint_32));
typedef void (*png_flush_ptr) PNGARG((png_structp));
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
typedef void (*png_progressive_info_ptr) PNGARG((png_structp, png_infop));
typedef void (*png_progressive_end_ptr) PNGARG((png_structp, png_infop));
typedef void (*png_progressive_row_ptr) PNGARG((png_structp, png_bytep,
   png_uint_32, int));
#endif

/* The structure that holds the information to read and write png files.
   The only people who need to care about what is inside of this are the
   people who will be modifying the library for their own special needs.
   */

struct png_struct_def
{
   jmp_buf jmpbuf; /* used in png_error */
   png_byte mode; /* used to determine where we are in the png file */
   png_byte read_mode;
   png_byte color_type; /* color type of file */
   png_byte bit_depth; /* bit depth of file */
   png_byte interlaced; /* interlace type of file */
   png_byte compession; /* compression type of file */
   png_byte filter; /* filter type */
   png_byte channels; /* number of channels in file */
   png_byte pixel_depth; /* number of bits per pixel */
   png_byte usr_bit_depth; /* bit depth of users row */
   png_byte usr_channels; /* channels at start of write */
#if defined(PNG_READ_GAMMA_SUPPORTED)
   png_byte gamma_shift; /* amount of shift for 16 bit gammas */
#endif
   png_byte pass; /* current pass (0 - 6) */
   png_byte row_init; /* 1 if png_read_start_row() has been called */
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_byte background_gamma_type;
   png_byte background_expand;
#endif
   png_byte zlib_finished;
   png_byte user_palette;
#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
   png_byte filler;
   png_byte filler_loc;
#endif
   png_byte zlib_custom_level; /* one if custom compression level */
   png_byte zlib_custom_method; /* one if custom compression method */
   png_byte zlib_custom_window_bits; /* one if custom compression window bits */
   png_byte zlib_custom_mem_level; /* one if custom compression memory level */
   png_byte zlib_custom_strategy; /* one if custom compression strategy */
   png_byte do_filter; /* one if filtering, zero if not */
   png_byte do_custom_filter; /* one if filtering, zero if not */
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_byte have_chunk_header;
#endif
   png_uint_16 num_palette; /* number of entries in palette */
   png_uint_16 num_trans; /* number of transparency values */
   int zlib_level; /* holds zlib compression level */
   int zlib_method; /* holds zlib compression method */
   int zlib_window_bits; /* holds zlib compression window bits */
   int zlib_mem_level; /* holds zlib compression memory level */
   int zlib_strategy; /* holds zlib compression strategy */
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   int process_mode;
   int cur_palette;
#endif
   png_uint_32 transformations; /* which transformations to perform */
   png_uint_32 crc; /* current crc value */
   png_uint_32 width; /* width of file */
   png_uint_32 height; /* height of file */
   png_uint_32 num_rows; /* number of rows in current pass */
   png_uint_32 rowbytes; /* size of row in bytes */
   png_uint_32 usr_width; /* width of row at start of write */
   png_uint_32 iwidth; /* interlaced width */
   png_uint_32 irowbytes; /* interlaced rowbytes */
   png_uint_32 row_number; /* current row in pass */
   png_uint_32 idat_size; /* current idat size for read */
   png_uint_32 zbuf_size; /* size of zbuf */
   png_uint_32 do_free; /* flags indicating if libpng should free memory */
#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   png_uint_32 flush_dist;  /* how many rows apart to flush, 0 for no flush */
   png_uint_32 flush_rows;  /* number of rows written since last flush */
#endif /* PNG_WRITE_FLUSH_SUPPORTED */
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_uint_32 push_length;
   png_uint_32 skip_length;
   png_uint_32 save_buffer_size;
   png_uint_32 save_buffer_max;
   png_uint_32 buffer_size;
   png_uint_32 current_buffer_size;
#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_READ_zTXt_SUPPORTED)
   png_uint_32 current_text_size;
   png_uint_32 current_text_left;
   png_charp current_text;
   png_charp current_text_ptr;
#endif
#if defined(__TURBOC__) && !defined(_Windows) && !defined(__FLAT__)
/* for the Borland special 64K segment handler */
   png_bytepp offset_table_ptr;
   png_bytep offset_table;
   png_uint_16 offset_table_number;
   png_uint_16 offset_table_count;
   png_uint_16 offset_table_count_free;
#endif
   png_byte push_chunk_name[4];
   png_bytep save_buffer_ptr;
   png_bytep save_buffer;
   png_bytep current_buffer_ptr;
   png_bytep current_buffer;
#endif
   png_colorp palette; /* files palette */
#if defined(PNG_READ_DITHER_SUPPORTED)
   png_bytep palette_lookup; /* lookup table for dithering */
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep gamma_table; /* gamma table for 8 bit depth files */
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep gamma_from_1; /* converts from 1.0 to screen */
   png_bytep gamma_to_1; /* converts from file to 1.0 */
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_bytep trans; /* transparency values for paletted files */
#endif
#if defined(PNG_READ_DITHER_SUPPORTED)
   png_bytep dither_index; /* index translation for palette files */
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_uint_16pp gamma_16_table; /* gamma table for 16 bit depth files */
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_uint_16pp gamma_16_from_1; /* converts from 1.0 to screen */
   png_uint_16pp gamma_16_to_1; /* converts from file to 1.0 */
#endif
#if defined(PNG_READ_DITHER_SUPPORTED)
   png_uint_16p hist; /* histogram */
#endif
   png_bytep zbuf; /* buffer for zlib */
   png_bytep row_buf; /* row buffer */
   png_bytep prev_row; /* previous row */
   png_bytep save_row; /* place to save row before filtering */
   z_stream * zstream; /* pointer to decompression structure (below) */
#if defined(PNG_READ_GAMMA_SUPPORTED)
   float gamma; /* file gamma value */
   float display_gamma; /* display gamma value */
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   float background_gamma;
#endif
#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
   png_color_8 shift; /* shift for significant bit tranformation */
#endif
#if defined(PNG_READ_GAMMA_SUPPORTED) || defined (PNG_READ_sBIT_SUPPORTED)
   png_color_8 sig_bit; /* significant bits in file */
#endif
#if defined(PNG_READ_BACKGROUND_SUPPORTED)
   png_color_16 trans_values; /* transparency values for non-paletted files */
   png_color_16 background; /* background color, gamma corrected for screen */
#if defined(PNG_READ_GAMMA_SUPPORTED)
   png_color_16 background_1; /* background normalized to gamma 1.0 */
#endif
#endif
   png_row_info row_info; /* used for transformation routines */
   LPSTREAM pstream; /* used for default png_read and png_write */
   png_msg_ptr error_fn;         /* Function for printing errors and aborting */
   png_msg_ptr warning_fn;       /* Function for printing warnings */
   png_rw_ptr write_data_fn;     /* Function for writing output data */
   png_rw_ptr read_data_fn;      /* Function for reading input data */
#if defined(PNG_WRITE_FLUSH_SUPPORTED)
   png_flush_ptr output_flush_fn;/* Function for flushing output */
#endif /* PNG_WRITE_FLUSH_SUPPORTED */
#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
   png_progressive_info_ptr info_fn;
   png_progressive_row_ptr row_fn;
   png_progressive_end_ptr end_fn;
   png_voidp push_ptr;
#endif
   png_voidp io_ptr;  /* Pointer to user supplied struct for I/O functions */
   png_voidp msg_ptr;  /* Pointer to user supplied struct for message functions */
};

typedef png_struct      FAR * FAR * png_structpp;

/* Here are the function definitions most commonly used.  This is not
   the place to find out how to use libpng.  See libpng.txt for the
   full explanation, see example.c for the summary.  This just provides
   a simple one line of the use of each function. */

#ifdef __cplusplus
extern "C" {
#endif

/* check the first 1 - 8 bytes to see if it is a png file */
extern int png_check_sig PNGARG((png_bytep sig, int num));

/* initialize png structure for reading, and allocate any memory needed */
extern void png_read_init PNGARG((png_structp png_ptr));

/* initialize png structure for writing, and allocate any memory needed */
extern void png_write_init PNGARG((png_structp png_ptr));

/* initialize the info structure */
extern void png_info_init PNGARG((png_infop info));

/* Writes all the png information before the image. */
extern void png_write_info PNGARG((png_structp png_ptr, png_infop info));

/* read the information before the actual image data. */
extern void png_read_info PNGARG((png_structp png_ptr, png_infop info));

#if defined(PNG_WRITE_tIME_SUPPORTED)
/* convert from a struct tm to png_time */
extern void png_convert_from_struct_tm PNGARG((png_timep ptime,
   struct tm FAR * ttime)); /*SJT: struct tm FAR *tttime ??? */

/* convert from time_t to png_time.  Uses gmtime() */
extern void png_convert_from_time_t PNGARG((png_timep ptime, time_t ttime));
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
/* Expand the data to 24 bit RGB, or 8 bit Grayscale,
   with alpha if necessary. */
extern void png_set_expand PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
/* Use blue, green, red order for pixels. */
extern void png_set_bgr PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
/* Expand the grayscale to 24 bit RGB if necessary. */
extern void png_set_gray_to_rgb PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_FILLER_SUPPORTED) || defined(PNG_WRITE_FILLER_SUPPORTED)
#define PNG_FILLER_BEFORE 0
#define PNG_FILLER_AFTER 1
/* Add a filler byte to rgb images. */
extern void png_set_filler PNGARG((png_structp png_ptr, int filler,
   int filler_loc));

/* old ways of doing this, still supported through 1.x for backwards
   compatability, but not suggested */

/* Add a filler byte to rgb images after the colors. */
extern void png_set_rgbx PNGARG((png_structp png_ptr));

/* Add a filler byte to rgb images before the colors. */
extern void png_set_xrgb PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
/* Swap bytes in 16 bit depth files. */
extern void png_set_swap PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_PACK_SUPPORTED) || defined(PNG_WRITE_PACK_SUPPORTED)
/* Use 1 byte per pixel in 1, 2, or 4 bit depth files. */
extern void png_set_packing PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED) || defined(PNG_WRITE_SHIFT_SUPPORTED)
/* Converts files to legal bit depths. */
extern void png_set_shift PNGARG((png_structp png_ptr,
   png_color_8p true_bits));
#endif

#if defined(PNG_READ_INTERLACING_SUPPORTED) || \
    defined(PNG_WRITE_INTERLACING_SUPPORTED)
/* Have the code handle the interlacing.  Returns the number of passes. */
extern int png_set_interlace_handling PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
/* Invert monocrome files */
extern void png_set_invert_mono PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
/* Handle alpha and tRNS by replacing with a background color. */
#define PNG_BACKGROUND_GAMMA_SCREEN 0
#define PNG_BACKGROUND_GAMMA_FILE 1
#define PNG_BACKGROUND_GAMMA_UNIQUE 2
#define PNG_BACKGROUND_GAMMA_UNKNOWN 3
extern void png_set_background PNGARG((png_structp png_ptr,
   png_color_16p background_color, int background_gamma_code,
   int need_expand, double background_gamma));
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
/* strip the second byte of information from a 16 bit depth file. */
extern void png_set_strip_16 PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_GRAY_TO_RGB_SUPPORTED)
/* convert a grayscale file into rgb. */
extern void png_set_gray_to_rgb PNGARG((png_structp png_ptr));
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
/* Turn on dithering, and reduce the palette to the number of colors available. */
extern void png_set_dither PNGARG((png_structp png_ptr, png_colorp palette,
   int num_palette, int maximum_colors, png_uint_16p histogram,
   int full_dither));
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
/* Handle gamma correction. */
extern void png_set_gamma PNGARG((png_structp png_ptr, double screen_gamma,
   double default_file_gamma));
#endif

#if defined(PNG_WRITE_FLUSH_SUPPORTED)
/* Set how many lines between output flushes - 0 for no flushing */
extern void png_set_flush PNGARG((png_structp png_ptr, int nrows));

/* Flush the current PNG output buffer */
extern void png_write_flush PNGARG((png_structp png_ptr));
#endif /* PNG_WRITE_FLUSH_SUPPORTED */

/* optional update palette with requested transformations */
extern void png_start_read_image PNGARG((png_structp png_ptr));

/* optional call to update the users info structure */
extern void png_read_update_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));

/* read a one or more rows of image data.*/
extern void png_read_rows PNGARG((png_structp png_ptr,
   png_bytepp row,
   png_bytepp display_row, png_uint_32 num_rows));

/* read a row of data.*/
extern void png_read_row PNGARG((png_structp png_ptr,
   png_bytep row,
   png_bytep display_row));

/* read the whole image into memory at once. */
extern void png_read_image PNGARG((png_structp png_ptr,
   png_bytepp image));

/* write a row of image data */
extern void png_write_row PNGARG((png_structp png_ptr,
   png_bytep row));

/* write a few rows of image data */
extern void png_write_rows PNGARG((png_structp png_ptr,
   png_bytepp row,
   png_uint_32 num_rows));

/* write the image data */
extern void png_write_image PNGARG((png_structp png_ptr, png_bytepp image));

/* writes the end of the png file. */
extern void png_write_end PNGARG((png_structp png_ptr, png_infop info));

/* read the end of the png file. */
extern void png_read_end PNGARG((png_structp png_ptr, png_infop info));

/* free all memory used by the read */
extern void png_read_destroy PNGARG((png_structp png_ptr, png_infop info,
   png_infop end_info));

/* free any memory used in png struct */
extern void png_write_destroy PNGARG((png_structp png_ptr));

/* These functions give the user control over the filtering and
   compression libraries used by zlib.  These functions are mainly
   useful for testing, as the defaults should work with most users.
   Those users who are tight on memory, or are wanting faster
   performance at the expense of compression can modify them.
   See the compression library header file for an explination
   of these functions */
extern void png_set_filtering PNGARG((png_structp png_ptr, int filter));

extern void png_set_compression_level PNGARG((png_structp png_ptr,
   int level));

extern void png_set_compression_mem_level PNGARG((png_structp png_ptr,
   int mem_level));

extern void png_set_compression_strategy PNGARG((png_structp png_ptr,
   int strategy));

extern void png_set_compression_window_bits PNGARG((png_structp png_ptr,
   int window_bits));

extern void png_set_compression_method PNGARG((png_structp png_ptr,
   int method));

/* These next functions are stubs of typical c functions for input/output,
   memory, and error handling.  They are in the file pngio.c, and pngerror.c.
   These functions can be replaced at run time for those applications that
   need to handle I/O in a different manner.  See the file libpng.txt for
   more information */

/* Write the data to whatever output you are using. */
extern void png_write_data PNGARG((png_structp png_ptr, png_bytep data,
   png_uint_32 length));

/* Read data from whatever input you are using */
extern void png_read_data PNGARG((png_structp png_ptr, png_bytep data,
   png_uint_32 length));

/* Initialize the input/output for the png file to the default functions. */
extern void png_init_io PNGARG((png_structp png_ptr, LPSTREAM pstream));

/* Replace the error message and abort, and warning functions with user
   supplied functions.  If no messages are to be printed then you must
   supply replacement message functions. The replacement error_fn should
   still do a longjmp to the last setjmp location if you are using this
   method of error handling.  If error_fn or warning_fn is NULL, the
   default functions will be used. */
extern void png_set_message_fn PNGARG((png_structp png_ptr, png_voidp msg_ptr,
   png_msg_ptr error_fn, png_msg_ptr warning_fn));

/* Return the user pointer associated with the message functions */
extern png_voidp png_get_msg_ptr PNGARG((png_structp png_ptr));

/* Replace the default data output functions with a user supplied one(s).
   If buffered output is not used, then output_flush_fn can be set to NULL.
   If PNG_WRITE_FLUSH_SUPPORTED is not defined at libpng compile time
   output_flush_fn will be ignored (and thus can be NULL). */
extern void png_set_write_fn PNGARG((png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn));

/* Replace the default data input function with a user supplied one. */
extern void png_set_read_fn PNGARG((png_structp png_ptr, png_voidp io_ptr,
   png_rw_ptr read_data_fn));

/* Return the user pointer associated with the I/O functions */
extern png_voidp png_get_io_ptr PNGARG((png_structp png_ptr));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
/* Replace the default push model read functions */
extern void png_set_push_fn PNGARG((png_structp png_ptr, png_voidp push_ptr,
   png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn,
   png_progressive_end_ptr end_fn));

/* returns the user pointer associated with the push read functions */
extern png_voidp png_get_progressive_ptr PNGARG((png_structp png_ptr));
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

extern png_voidp png_large_malloc PNGARG((png_structp png_ptr,
   png_uint_32 size));

/* free's a pointer allocated by png_large_malloc() */
extern void png_large_free PNGARG((png_structp png_ptr, png_voidp ptr));

/* Allocate memory. */
extern void * png_malloc PNGARG((png_structp png_ptr, png_uint_32 size));

/* Reallocate memory. */
extern void * png_realloc PNGARG((png_structp png_ptr, void * ptr,
   png_uint_32 size, png_uint_32 old_size));

/* free's a pointer allocated by png_malloc() */
extern void png_free PNGARG((png_structp png_ptr, void * ptr));

/* Fatal error in libpng - can't continue */ 
extern void png_error PNGARG((png_structp png_ptr, png_const_charp error));

/* Non-fatal error in libpng.  Can continue, but may have a problem. */
extern void png_warning PNGARG((png_structp png_ptr, png_const_charp message));

#ifdef __cplusplus
}
#endif

/* These next functions are used internally in the code.  If you use
   them, make sure you read and understand the png spec.  More information
   about them can be found in the files where the functions are.
   Feel free to move any of these outside the PNG_INTERNAL define if
   you just need a few of them, but if you need access to more, you should
   define PNG_INTERNAL inside your code, so everyone who includes png.h
   won't get yet another definition the compiler has to deal with. */

#ifdef PNG_INTERNAL

/* various modes of operation.  Note that after an init, mode is set to
   zero automatically */
#define PNG_BEFORE_IHDR 0
#define PNG_HAVE_IHDR 1
#define PNG_HAVE_PLTE 2
#define PNG_HAVE_IDAT 3
#define PNG_AT_LAST_IDAT 4
#define PNG_AFTER_IDAT 5
#define PNG_AFTER_IEND 6

/* push model modes */
#define PNG_READ_SIG_MODE 0
#define PNG_READ_CHUNK_MODE 1
#define PNG_READ_IDAT_MODE 2
#define PNG_READ_PLTE_MODE 3
#define PNG_READ_END_MODE 4
#define PNG_SKIP_MODE 5
#define PNG_READ_tEXt_MODE 6
#define PNG_READ_zTXt_MODE 7
#define PNG_READ_DONE_MODE 8
#define PNG_ERROR_MODE 9

/* read modes */
#define PNG_READ_PULL_MODE 0
#define PNG_READ_PUSH_MODE 1

/* defines for the transformations the png library does on the image data */
#define PNG_BGR 0x0001
#define PNG_INTERLACE 0x0002
#define PNG_PACK 0x0004
#define PNG_SHIFT 0x0008
#define PNG_SWAP_BYTES 0x0010
#define PNG_INVERT_MONO 0x0020
#define PNG_DITHER 0x0040
#define PNG_BACKGROUND 0x0080
#define PNG_XRGB 0x0100
#define PNG_16_TO_8 0x0200
#define PNG_RGBA 0x0400
#define PNG_EXPAND 0x0800
#define PNG_GAMMA 0x1000
#define PNG_GRAY_TO_RGB 0x2000
#define PNG_FILLER 0x4000

/* save typing and make code easier to understand */
#define PNG_COLOR_DIST(c1, c2) (abs((int)((c1).red) - (int)((c2).red)) + \
   abs((int)((c1).green) - (int)((c2).green)) + \
   abs((int)((c1).blue) - (int)((c2).blue)))

/* variables defined in png.c - only it needs to define PNG_NO_EXTERN */
#ifndef PNG_NO_EXTERN
/* place to hold the signiture string for a png file. */
extern png_byte png_sig[];

/* version information for c files, stored in png.c. */
extern char png_libpng_ver[];

/* constant strings for known chunk types.  If you need to add a chunk,
   add a string holding the name here.  See png.c for more details */
extern png_byte FARDATA png_IHDR[];
extern png_byte FARDATA png_IDAT[];
extern png_byte FARDATA png_IEND[];
extern png_byte FARDATA png_PLTE[];
#if defined(PNG_READ_gAMA_SUPPORTED) || defined(PNG_WRITE_gAMA_SUPPORTED)
extern png_byte FARDATA png_gAMA[];
#endif
#if defined(PNG_READ_sBIT_SUPPORTED) || defined(PNG_WRITE_sBIT_SUPPORTED)
extern png_byte FARDATA png_sBIT[];
#endif
#if defined(PNG_READ_cHRM_SUPPORTED) || defined(PNG_WRITE_cHRM_SUPPORTED)
extern png_byte FARDATA png_cHRM[];
#endif
#if defined(PNG_READ_tRNS_SUPPORTED) || defined(PNG_WRITE_tRNS_SUPPORTED)
extern png_byte FARDATA png_tRNS[];
#endif
#if defined(PNG_READ_bKGD_SUPPORTED) || defined(PNG_WRITE_bKGD_SUPPORTED)
extern png_byte FARDATA png_bKGD[];
#endif
#if defined(PNG_READ_hIST_SUPPORTED) || defined(PNG_WRITE_hIST_SUPPORTED)
extern png_byte FARDATA png_hIST[];
#endif
#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED)
extern png_byte FARDATA png_tEXt[];
#endif
#if defined(PNG_READ_zTXt_SUPPORTED) || defined(PNG_WRITE_zTXt_SUPPORTED)
extern png_byte FARDATA png_zTXt[];
#endif
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
extern png_byte FARDATA png_pHYs[];
#endif
#if defined(PNG_READ_oFFs_SUPPORTED) || defined(PNG_WRITE_oFFs_SUPPORTED)
extern png_byte FARDATA png_oFFs[];
#endif
#if defined(PNG_READ_tIME_SUPPORTED) || defined(PNG_WRITE_tIME_SUPPORTED)
extern png_byte FARDATA png_tIME[];
#endif
/* Structures to facilitate easy interlacing.  See png.c for more details */
extern int FARDATA png_pass_start[];
extern int FARDATA png_pass_inc[];
extern int FARDATA png_pass_ystart[];
extern int FARDATA png_pass_yinc[];
/* these are not currently used.  If you need them, see png.c
extern int FARDATA png_pass_width[];
extern int FARDATA png_pass_height[];
*/
extern int FARDATA png_pass_mask[];
extern int FARDATA png_pass_dsp_mask[];

#endif /* PNG_NO_EXTERN */

/* Function to allocate memory for zlib. */
extern voidpf png_zalloc PNGARG((voidpf png_ptr, uInt items, uInt size));

/* function to free memory for zlib */
extern void png_zfree PNGARG((voidpf png_ptr, voidpf ptr));

/* reset the crc variable */
extern void png_reset_crc PNGARG((png_structp png_ptr));

/* calculate the crc over a section of data.  Note that while we
   are passing in a 32 bit value for length, on 16 bit machines, you
   would need to use huge pointers to access all that data.  See the
   code in png.c for more information. */
extern void png_calculate_crc PNGARG((png_structp png_ptr, png_bytep ptr,
   png_uint_32 length));

/* default error and warning functions if user doesn't supply them */
extern void png_default_warning PNGARG((png_structp png_ptr,
   png_const_charp message));
extern void png_default_error PNGARG((png_structp png_ptr,
   png_const_charp error));
#if defined(PNG_WRITE_FLUSH_SUPPORTED)
extern void png_flush PNGARG((png_structp png_ptr));
extern void png_default_flush PNGARG((png_structp png_ptr));
#endif

/* place a 32 bit number into a buffer in png byte order.  We work
   with unsigned numbers for convenience, you may have to cast
   signed numbers (if you use any, most png data is unsigned). */
extern void png_save_uint_32 PNGARG((png_bytep buf, png_uint_32 i));

/* place a 16 bit number into a buffer in png byte order */
extern void png_save_uint_16 PNGARG((png_bytep buf, png_uint_16 i));

/* write a 32 bit number */
extern void png_write_uint_32 PNGARG((png_structp png_ptr, png_uint_32 i));

/* write a 16 bit number */
extern void png_write_uint_16 PNGARG((png_structp png_ptr, png_uint_16 i));

/* Write a png chunk.  */
extern void png_write_chunk PNGARG((png_structp png_ptr, png_bytep type,
   png_bytep data, png_uint_32 length));

/* Write the start of a png chunk. */
extern void png_write_chunk_start PNGARG((png_structp png_ptr, png_bytep type,
   png_uint_32 total_length));

/* write the data of a png chunk started with png_write_chunk_start(). */
extern void png_write_chunk_data PNGARG((png_structp png_ptr, png_bytep data,
   png_uint_32 length));

/* finish a chunk started with png_write_chunk_start() */
extern void png_write_chunk_end PNGARG((png_structp png_ptr));

/* simple function to write the signiture */
extern void png_write_sig PNGARG((png_structp png_ptr));

/* write various chunks */

/* Write the IHDR chunk, and update the png_struct with the necessary
   information. */
extern void png_write_IHDR PNGARG((png_structp png_ptr, png_uint_32 width,
   png_uint_32 height,
   int bit_depth, int color_type, int compression_type, int filter_type,
   int interlace_type));

extern void png_write_PLTE PNGARG((png_structp png_ptr, png_colorp palette,
   int number));

extern void png_write_IDAT PNGARG((png_structp png_ptr, png_bytep data,
   png_uint_32 length));

extern void png_write_IEND PNGARG((png_structp png_ptr));

#if defined(PNG_WRITE_gAMA_SUPPORTED)
extern void png_write_gAMA PNGARG((png_structp png_ptr, double gamma));
#endif

#if defined(PNG_WRITE_sBIT_SUPPORTED)
extern void png_write_sBIT PNGARG((png_structp png_ptr, png_color_8p sbit,
   int color_type));
#endif

#if defined(PNG_WRITE_cHRM_SUPPORTED)
extern void png_write_cHRM PNGARG((png_structp png_ptr,
   double white_x, double white_y,
   double red_x, double red_y, double green_x, double green_y,
   double blue_x, double blue_y));
#endif

#if defined(PNG_WRITE_tRNS_SUPPORTED)
extern void png_write_tRNS PNGARG((png_structp png_ptr, png_bytep trans,
   png_color_16p values, int number, int color_type));
#endif

#if defined(PNG_WRITE_bKGD_SUPPORTED)
extern void png_write_bKGD PNGARG((png_structp png_ptr, png_color_16p values,
   int color_type));
#endif

#if defined(PNG_WRITE_hIST_SUPPORTED)
extern void png_write_hIST PNGARG((png_structp png_ptr, png_uint_16p hist,
   int number));
#endif

#if defined(PNG_WRITE_tEXt_SUPPORTED)
extern void png_write_tEXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_uint_32 text_len));
#endif

#if defined(PNG_WRITE_zTXt_SUPPORTED)
extern void png_write_zTXt PNGARG((png_structp png_ptr, png_charp key,
   png_charp text, png_uint_32 text_len, int compression));
#endif

#if defined(PNG_WRITE_pHYs_SUPPORTED)
extern void png_write_pHYs PNGARG((png_structp png_ptr,
   png_uint_32 x_pixels_per_unit,
   png_uint_32 y_pixels_per_unit,
   int unit_type));
#endif

#if defined(PNG_WRITE_oFFs_SUPPORTED)
extern void png_write_oFFs PNGARG((png_structp png_ptr,
   png_uint_32 x_offset,
   png_uint_32 y_offset,
   int unit_type));
#endif

#if defined(PNG_WRITE_tIME_SUPPORTED)
extern void png_write_tIME PNGARG((png_structp png_ptr, png_timep mod_time));
#endif

/* Internal use only.   Called when finished processing a row of data */
extern void png_write_finish_row PNGARG((png_structp png_ptr));

/* Internal use only.   Called before first row of data */
extern void png_write_start_row PNGARG((png_structp png_ptr));

/* callbacks for png chunks */
extern void png_read_IHDR PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 width, png_uint_32 height, int bit_depth,
   int color_type, int compression_type, int filter_type,
   int interlace_type));

extern void png_read_PLTE PNGARG((png_structp png_ptr, png_infop info,
   png_colorp palette, int num));

#if defined(PNG_READ_gAMA_SUPPORTED)
extern void png_read_gAMA PNGARG((png_structp png_ptr, png_infop info,
   double gamma));
#endif

#if defined(PNG_READ_sBIT_SUPPORTED)
extern void png_read_sBIT PNGARG((png_structp png_ptr, png_infop info,
   png_color_8p sig_bit));
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
extern void png_read_cHRM PNGARG((png_structp png_ptr, png_infop info,
   double white_x, double white_y, double red_x, double red_y,
   double green_x, double green_y, double blue_x, double blue_y));
#endif

#if defined(PNG_READ_tRNS_SUPPORTED)
extern void png_read_tRNS PNGARG((png_structp png_ptr, png_infop info,
   png_bytep trans, int num_trans,   png_color_16p trans_values));
#endif

#if defined(PNG_READ_bKGD_SUPPORTED)
extern void png_read_bKGD PNGARG((png_structp png_ptr, png_infop info,
   png_color_16p background));
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
extern void png_read_hIST PNGARG((png_structp png_ptr, png_infop info,
   png_uint_16p hist));
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
extern void png_read_pHYs PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 res_x, png_uint_32 res_y, int unit_type));
#endif

#if defined(PNG_READ_oFFs_SUPPORTED)
extern void png_read_oFFs PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 offset_x, png_uint_32 offset_y, int unit_type));
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
extern void png_read_tIME PNGARG((png_structp png_ptr, png_infop info,
   png_timep mod_time));
#endif

#if defined(PNG_READ_tEXt_SUPPORTED)
extern void png_read_tEXt PNGARG((png_structp png_ptr, png_infop info,
   png_charp key, png_charp text, png_uint_32 text_len));
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
extern void png_read_zTXt PNGARG((png_structp png_ptr, png_infop info,
   png_charp key, png_charp text, png_uint_32 text_len, int compression));
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
void
png_build_gamma_table PNGARG((png_structp png_ptr));
#endif

/* combine a row of data, dealing with alpha, etc. if requested */
extern void png_combine_row PNGARG((png_structp png_ptr, png_bytep row,
   int mask));

#if defined(PNG_READ_INTERLACING_SUPPORTED)
/* expand an interlaced row */
extern void png_do_read_interlace PNGARG((png_row_infop row_info,
   png_bytep row, int pass));
#endif

#if defined(PNG_WRITE_INTERLACING_SUPPORTED)
/* grab pixels out of a row for an interlaced pass */
extern void png_do_write_interlace PNGARG((png_row_infop row_info,
   png_bytep row, int pass));
#endif

/* unfilter a row */
extern void png_read_filter_row PNGARG((png_row_infop row_info,
   png_bytep row, png_bytep prev_row, int filter));
/* filter a row, and place the correct filter byte in the row */
extern void png_write_filter_row PNGARG((png_row_infop row_info,
   png_bytep row, png_bytep prev_row));
/* finish a row while reading, dealing with interlacing passes, etc. */
extern void png_read_finish_row PNGARG((png_structp png_ptr));
/* initialize the row buffers, etc. */
extern void png_read_start_row PNGARG((png_structp png_ptr));
/* optional call to update the users info structure */
extern void png_read_transform_info PNGARG((png_structp png_ptr,
   png_infop info_ptr));

/* these are the functions that do the transformations */
#if defined(PNG_READ_FILLER_SUPPORTED)
extern void png_do_read_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_byte filler, png_byte filler_loc));
#endif

#if defined(PNG_WRITE_FILLER_SUPPORTED)
extern void png_do_write_filler PNGARG((png_row_infop row_info,
   png_bytep row, png_byte filler_loc));
#endif

#if defined(PNG_READ_SWAP_SUPPORTED) || defined(PNG_WRITE_SWAP_SUPPORTED)
extern void png_do_swap PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_PACK_SUPPORTED)
extern void png_do_unpack PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_SHIFT_SUPPORTED)
extern void png_do_unshift PNGARG((png_row_infop row_info, png_bytep row,
   png_color_8p sig_bits));
#endif

#if defined(PNG_READ_INVERT_SUPPORTED) || defined(PNG_WRITE_INVERT_SUPPORTED)
extern void png_do_invert PNGARG((png_row_infop row_info, png_bytep row));
#endif

extern void png_build_grayscale_palette PNGARG((int bit_depth,
   png_colorp palette));

#if defined(PNG_READ_GRAY_TO_RGB_SUPPORTED)
extern void png_do_gray_to_rgb PNGARG((png_row_infop row_info,
   png_bytep row));
#endif

#if defined(PNG_READ_16_TO_8_SUPPORTED)
extern void png_do_chop PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_READ_DITHER_SUPPORTED)
extern void png_do_dither PNGARG((png_row_infop row_info,
   png_bytep row, png_bytep palette_lookup, png_bytep dither_lookup));
#endif

#if defined(PNG_READ_BGR_SUPPORTED) || defined(PNG_WRITE_BGR_SUPPORTED)
extern void png_do_bgr PNGARG((png_row_infop row_info, png_bytep row));
#endif

#if defined(PNG_WRITE_PACK_SUPPORTED)
extern void png_do_pack PNGARG((png_row_infop row_info,
   png_bytep row, png_byte bit_depth));
#endif

#if defined(PNG_WRITE_SHIFT_SUPPORTED)
extern void png_do_shift PNGARG((png_row_infop row_info, png_bytep row,
   png_color_8p bit_depth));
#endif

#if defined(PNG_READ_BACKGROUND_SUPPORTED)
extern void png_do_background PNGARG((png_row_infop row_info, png_bytep row,
   png_color_16p trans_values, png_color_16p background,
   png_color_16p background_1,
   png_bytep gamma_table, png_bytep gamma_from_1, png_bytep gamma_to_1,
   png_uint_16pp gamma_16, png_uint_16pp gamma_16_from_1,
   png_uint_16pp gamma_16_to_1, int gamma_shift));
#endif

#if defined(PNG_READ_GAMMA_SUPPORTED)
extern void png_do_gamma PNGARG((png_row_infop row_info, png_bytep row,
   png_bytep gamma_table, png_uint_16pp gamma_16_table,
   int gamma_shift));
#endif

#if defined(PNG_READ_EXPAND_SUPPORTED)
extern void png_do_expand_palette PNGARG((png_row_infop row_info,
   png_bytep row, png_colorp palette, png_bytep trans, int num_trans));
extern void png_do_expand PNGARG((png_row_infop row_info,
   png_bytep row, png_color_16p trans_value));
#endif

/* unpack 16 and 32 bit values from a string */
extern png_uint_32 png_get_uint_32 PNGARG((png_bytep buf));
extern png_uint_16 png_get_uint_16 PNGARG((png_bytep buf));

/* read bytes into buf, and update png_ptr->crc */
extern void png_crc_read PNGARG((png_structp png_ptr, png_bytep buf,
   png_uint_32 length));
/* skip length bytes, and update png_ptr->crc */
extern void png_crc_skip PNGARG((png_structp png_ptr, png_uint_32 length));

/* the following decodes the appropriate chunks, and does error correction,
   then calls the appropriate callback for the chunk if it is valid */

/* decode the IHDR chunk */
extern void png_handle_IHDR PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
extern void png_handle_PLTE PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#if defined(PNG_READ_gAMA_SUPPORTED)
extern void png_handle_gAMA PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_sBIT_SUPPORTED)
extern void png_handle_sBIT PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
extern void png_handle_cHRM PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tRNS_SUPPORTED)
extern void png_handle_tRNS PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_bKGD_SUPPORTED)
extern void png_handle_bKGD PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
extern void png_handle_hIST PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
extern void png_handle_pHYs PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_oFFs_SUPPORTED)
extern void png_handle_oFFs PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
extern void png_handle_tIME PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_tEXt_SUPPORTED)
extern void png_handle_tEXt PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

#if defined(PNG_READ_zTXt_SUPPORTED)
extern void png_handle_zTXt PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
#endif

/* handle the transformations for reading and writing */
extern void png_do_read_transformations PNGARG((png_structp png_ptr));
extern void png_do_write_transformations PNGARG((png_structp png_ptr));

extern void png_init_read_transformations PNGARG((png_structp png_ptr));

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
extern void png_push_read_chunk PNGARG((png_structp png_ptr, png_infop info));
extern void png_push_read_sig PNGARG((png_structp png_ptr));
extern void png_push_check_crc PNGARG((png_structp png_ptr));
extern void png_push_crc_skip PNGARG((png_structp png_ptr, png_uint_32 length));
extern void png_push_skip PNGARG((png_structp png_ptr));
extern void png_push_fill_buffer PNGARG((png_structp png_ptr, png_bytep buffer,
   png_uint_32 length));
extern void png_push_save_buffer PNGARG((png_structp png_ptr));
extern void png_push_restore_buffer PNGARG((png_structp png_ptr, png_bytep buffer,
   png_uint_32 buffer_length));
extern void png_push_read_idat PNGARG((png_structp png_ptr));
extern void png_process_IDAT_data PNGARG((png_structp png_ptr,
   png_bytep buffer, png_uint_32 buffer_length));
extern void png_push_process_row PNGARG((png_structp png_ptr));
extern void png_push_handle_PLTE PNGARG((png_structp png_ptr,
   png_uint_32 length));
extern void png_push_read_plte PNGARG((png_structp png_ptr, png_infop info));
extern void png_push_handle_tRNS PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
extern void png_push_handle_hIST PNGARG((png_structp png_ptr, png_infop info,
   png_uint_32 length));
extern void png_push_have_info PNGARG((png_structp png_ptr, png_infop info));
extern void png_push_have_end PNGARG((png_structp png_ptr, png_infop info));
extern void png_push_have_row PNGARG((png_structp png_ptr, png_bytep row));
extern void png_push_read_end PNGARG((png_structp png_ptr, png_infop info));
extern void png_process_some_data PNGARG((png_structp png_ptr,
   png_infop info));
extern void png_read_push_finish_row PNGARG((png_structp png_ptr));
#if defined(PNG_READ_tEXt_SUPPORTED)
extern void png_push_handle_tEXt PNGARG((png_structp png_ptr,
   png_uint_32 length));
extern void png_push_read_text PNGARG((png_structp png_ptr, png_infop info));
#endif
#if defined(PNG_READ_zTXt_SUPPORTED)
extern void png_push_handle_zTXt PNGARG((png_structp png_ptr,
   png_uint_32 length));
extern void png_push_read_ztxt PNGARG((png_structp png_ptr, png_infop info));
#endif

#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

#endif /* PNG_INTERNAL */

#ifdef PNG_PROGRESSIVE_READ_SUPPORTED
extern void png_process_data PNGARG((png_structp png_ptr, png_infop info,
   png_bytep buffer, png_uint_32 buffer_size));
extern void png_set_progressive_read_fn PNGARG((png_structp png_ptr,
   png_voidp progressive_ptr,
   png_progressive_info_ptr info_fn, png_progressive_row_ptr row_fn,
   png_progressive_end_ptr end_fn));
extern void png_progressive_combine_row PNGARG((png_structp png_ptr,
   png_bytep old_row, png_bytep new_row));
#endif /* PNG_PROGRESSIVE_READ_SUPPORTED */

/* do not put anything past this line */
#endif /* _PNG_H */

