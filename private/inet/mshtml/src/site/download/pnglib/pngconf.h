
/* pngconf.c - machine configurable file for libpng

   libpng 1.0 beta 2 - version 0.88
   For conditions of distribution and use, see copyright notice in png.h
   Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
   January 25, 1996
   */

/* Any machine specific code is near the front of this file, so if you
   are configuring libpng for a machine, you may want to read the section
   starting here down to where it starts to typedef png_color, png_text,
   and png_info */

#ifndef PNGCONF_H
#define PNGCONF_H

/* this is the size of the compression buffer, and thus the size of
   an IDAT chunk.  Make this whatever size you feel is best for your
   machine.  One of these will be allocated per png_struct.  When this
   is full, it writes the data to the disk, and does some other
   calculations.  Making this an extreamly small size will slow
   the library down, but you may want to experiment to determine
   where it becomes significant, if you are concerned with memory
   usage.  Note that zlib allocates at least 32Kb also.  For readers,
   this describes the size of the buffer available to read the data in.
   Unless this gets smaller then the size of a row (compressed),
   it should not make much difference how big this is.  */

#define PNG_ZBUF_SIZE 8192

/* While libpng currently uses zlib for it's compression, it has been designed
   to stand on it's own.  Towards this end, there are two defines that are
   used to help portability between machines.  To make it simpler to
   setup libpng on a machine, this currently uses zlib's definitions, so
   any changes should be made in zlib.  Libpng will check zlib's settings
   and adjust it's own accordingly. */

/* if you are running on a machine where you cannot allocate more then
   64K of memory, uncomment this.  While libpng will not normally need
   that much memory in a chunk (unless you load up a very large file),
   zlib needs to know how big of a chunk it can use, and libpng thus
   makes sure to check any memory allocation to verify it will fit
   into memory.
#define PNG_MAX_ALLOC_64K
*/
#ifdef MAXSEG_64K
#define PNG_MAX_ALLOC_64K
#endif

/* this protects us against compilers which run on a windowing system
   and thus don't have or would rather us not use the stdio types:
   stdin, stdout, and stderr.  The only one currently used is stderr
   in png_error() and png_warning().  #defining PNG_NO_STDIO will
   prevent these from being compiled and used. */

/* #define PNG_NO_STDIO */

/* this macro protects us against machines that don't have function
   prototypes.  If your compiler does not handle function prototypes,
   define this macro.  I've always been able to use _NO_PROTO as the
   indicator, but you may need to drag the empty declaration out in
   front of here, or change the ifdef to suit your own needs. */
#ifndef PNGARG

#ifdef OF
#define PNGARG(arglist) OF(arglist)
#else

#ifdef _NO_PROTO
#define PNGARG(arglist) ()
#else
#define PNGARG(arglist) arglist
#endif /* _NO_PROTO */

#endif /* OF */

#endif /* PNGARG */

/* enough people need this for various reasons to include it here */
#ifndef MACOS
#include <sys/types.h>
#endif
/* need the time information for reading tIME chunks */
#include <time.h>

/* for FILE.  If you are not using standard io, you don't need this */
//#include <stdio.h>

/* include setjmp.h for error handling */
#include <setjmp.h>

#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

/* other defines for things like memory and the like can go here.  These
   are the only files included in libpng, so if you need to change them,
   change them here.  They are only included if PNG_INTERNAL is defined. */
#ifdef PNG_INTERNAL
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

/* other defines specific to compilers can go here.  Try to keep
   them inside an appropriate ifdef/endif pair for portability */

/* for some reason, Borland C++ defines memcmp, etc. in mem.h, not
   stdlib.h like it should (I think).  Or perhaps this is a C++
   feature */
#ifdef __TURBOC__
#include <mem.h>
#include "alloc.h"
#endif

#ifdef _MSC_VER
//#include <malloc.h>
#endif

/* this controls how fine the dithering gets.  As this allocates
   a largish chunk of memory (32K), those who are not as concerned
   with dithering quality can decrease some or all of these */
#define PNG_DITHER_RED_BITS 5
#define PNG_DITHER_GREEN_BITS 5
#define PNG_DITHER_BLUE_BITS 5

/* this controls how fine the gamma correction becomes when you
   are only interested in 8 bits anyway.  Increasing this value
   results in more memory being used, and more pow() functions
   being called to fill in the gamma tables.  Don't get this
   value less then 8, and even that may not work (I haven't tested
   it). */

#define PNG_MAX_GAMMA_8 11

#endif /* PNG_INTERNAL */

/* the following uses const char * instead of char * for error
   and warning message functions, so some compilers won't complain.
   If you want to use const, define PNG_USE_CONST here.  It is not
   normally defined to make configuration easier, as it is not a
   critical part of the code.
   */

#ifdef PNG_USE_CONST
#  define PNG_CONST const
#else
#  define PNG_CONST
#endif

/* The following defines give you the ability to remove code
   from the library that you will not be using.  I wish I
   could figure out how to automate this, but I can't do
   that without making it seriously hard on the users.  So
   if you are not using an ability, change the #define to
   and #undef, and that part of the library will not be
   compiled.  If your linker can't find a function, you
   may want to make sure the ability is defined here.
   Some of these depend upon some others being defined.
   I haven't figured out all the interactions here, so
   you may have to experiment awhile to get everything
   to compile.
   */

/* Any transformations you will not be using can be undef'ed here */
#define PNG_PROGRESSIVE_READ_SUPPORTED
#define PNG_READ_INTERLACING_SUPPORTED
#define PNG_READ_EXPAND_SUPPORTED
#define PNG_READ_SHIFT_SUPPORTED
#define PNG_READ_PACK_SUPPORTED
#define PNG_READ_BGR_SUPPORTED
#define PNG_READ_SWAP_SUPPORTED
#define PNG_READ_INVERT_SUPPORTED
#define PNG_READ_DITHER_SUPPORTED
#define PNG_READ_BACKGROUND_SUPPORTED
#define PNG_READ_16_TO_8_SUPPORTED
#define PNG_READ_FILLER_SUPPORTED
#define PNG_READ_GAMMA_SUPPORTED
#define PNG_READ_GRAY_TO_RGB_SUPPORTED

//#define PNG_WRITE_INTERLACING_SUPPORTED
//#define PNG_WRITE_SHIFT_SUPPORTED
//#define PNG_WRITE_PACK_SUPPORTED
//#define PNG_WRITE_BGR_SUPPORTED
//#define PNG_WRITE_SWAP_SUPPORTED
//#define PNG_WRITE_INVERT_SUPPORTED
//#define PNG_WRITE_FILLER_SUPPORTED
//#define PNG_WRITE_FLUSH_SUPPORTED

/* any chunks you are not interested in, you can undef here.  The
   ones that allocate memory may be expecially important (hIST,
   tEXt, zTXt, tRNS) Others will just save time and make png_info
   smaller.  OPT_PLTE only disables the optional palette in RGB
   and RGB Alpha images. */

#define PNG_READ_gAMA_SUPPORTED
#define PNG_READ_sBIT_SUPPORTED
#define PNG_READ_cHRM_SUPPORTED
#define PNG_READ_tRNS_SUPPORTED
#define PNG_READ_bKGD_SUPPORTED
#define PNG_READ_hIST_SUPPORTED
#define PNG_READ_pHYs_SUPPORTED
#define PNG_READ_oFFs_SUPPORTED
#define PNG_READ_tIME_SUPPORTED
#define PNG_READ_tEXt_SUPPORTED
#define PNG_READ_zTXt_SUPPORTED
#define PNG_READ_OPT_PLTE_SUPPORTED

//#define PNG_WRITE_gAMA_SUPPORTED
//#define PNG_WRITE_sBIT_SUPPORTED
//#define PNG_WRITE_cHRM_SUPPORTED
//#define PNG_WRITE_tRNS_SUPPORTED
//#define PNG_WRITE_bKGD_SUPPORTED
//#define PNG_WRITE_hIST_SUPPORTED
//#define PNG_WRITE_pHYs_SUPPORTED
//#define PNG_WRITE_oFFs_SUPPORTED
//#define PNG_WRITE_tIME_SUPPORTED
//#define PNG_WRITE_tEXt_SUPPORTED
//#define PNG_WRITE_zTXt_SUPPORTED

/* some typedefs to get us started.  These should be safe on most of the
   common platforms.  The typedefs should be at least as large
   as the numbers suggest (a png_uint_32 must be at least 32 bits long),
   but they don't have to be exactly that size. */

typedef unsigned long png_uint_32;
typedef long png_int_32;
typedef unsigned short png_uint_16;
typedef short png_int_16;
typedef unsigned char png_byte;

/* this is usually size_t. it is typedef'ed just in case you need it to
   change (I'm not sure if you will or not, so I thought I'd be safe) */
typedef size_t png_size_t;

/* The following is needed for medium model support. It cannot be in the
   PNG_INTERNAL section. Needs modification for other compilers besides
   MSC. Model independent support declares all arrays that might be very
   large using the far keyword. The Zlib version used must also support
   model independent data. As of version Zlib .95, the necessary changes
   have been made in Zlib. The USE_FAR_KEYWORD define triggers other
   changes that are needed. Most of the far keyword changes are hidden
   inside typedefs with suffix "f". Tim Wegner */

/* SJT: Separate compiler dependencies */
/* SJT: problem here is that zlib.h always defines FAR */
#ifdef __BORLANDC__
#if defined(__LARGE__) || defined(__HUGE__) || defined(__COMPACT__)
#define LDATA 1
#else
#define LDATA 0
#endif

#if !defined(__WIN32__) && !defined(__FLAT__)
#define PNG_MAX_MALLOC_64K
#if (LDATA != 1)
#ifndef FAR
#define FAR __far
#endif
#define USE_FAR_KEYWORD
#endif   /* LDATA != 1 */

/* SJT:  Possibly useful for moving data out of default segment.
   Uncomment it if you want. Could also define FARDATA as const
   if your compiler supports it.
#  define FARDATA FAR
*/
#endif  /* __WIN32__, __FLAT__ */

#endif   /* __BORLANDC__ */


/* SJT:  Suggest testing for specific compiler first before
   testing for FAR.  The Watcom compiler defines both __MEDIUM__
   and M_I86MM, making reliance oncertain keywords suspect
*/

/* MSC Medium model */
#if defined(FAR)
#  if defined(M_I86MM)
#     define USE_FAR_KEYWORD
#     define FARDATA FAR /* SJT: added */
#  endif
#endif

/* SJT: default case */
#ifndef FAR
#   define FAR
#endif

/* SJT: At this point FAR is always defined */

/* not used anymore, but kept for compatability */
typedef unsigned char FAR png_bytef;

/* SJT: */
#ifndef FARDATA
#define FARDATA
#endif

/* End medium model changes to be in zconf.h */

/* SJT: More typedefs */
typedef void FAR *   png_voidp;


/* SJT: Add typedefs for pointers */
typedef png_byte        FAR * png_bytep;
typedef png_uint_32     FAR * png_uint_32p;
typedef png_int_32      FAR * png_int_32p;
typedef png_uint_16     FAR * png_uint_16p;
typedef png_int_16      FAR * png_int_16p;
typedef PNG_CONST char  FAR * png_const_charp;
typedef char            FAR * png_charp;

/*  SJT: Pointers to pointers; i.e. arrays */
typedef png_byte        FAR * FAR * png_bytepp;
typedef png_uint_32     FAR * FAR * png_uint_32pp;
typedef png_int_32      FAR * FAR * png_int_32pp;
typedef png_uint_16     FAR * FAR * png_uint_16pp;
typedef png_int_16      FAR * FAR * png_int_16pp;
typedef PNG_CONST char  FAR * FAR * png_const_charpp;
typedef char            FAR * FAR * png_charpp;


/* SJT: libpng typedefs for types in zlib. If Zlib changes
   or another compression library is used, then change these.
   Eliminates need to change all the source files.
*/
typedef charf *         png_zcharp;
typedef charf * FAR *   png_zcharpp;
typedef z_stream *      png_zstreamp; /* zlib won't accept far z_stream */


/* User may want to use these so not in PNG_INTERNAL. Any library functions
   that are passed far data must be model independent. */
#if defined(USE_FAR_KEYWORD)  /* memory model independent fns */
#   define png_strcpy _fstrcpy
#   define png_strcat _fstrcat
#   define png_strlen _fstrlen
#   define png_strcmp _fstrcmp
#   define png_memcmp _fmemcmp      /* SJT: added */
#   define png_memcpy _fmemcpy
#   define png_memset _fmemset
#else /* use the usual functions */
#   define png_strcpy strcpy
#   define png_strcat strcat
#   define png_strlen strlen
#   define png_strcmp strcmp
#   define png_memcmp memcmp     /* SJT: added */
#   define png_memcpy memcpy
#   define png_memset memset
#endif
/* End of memory model independent support */

#pragma warning(disable:4244)   /* conversion; possible loss of data */

#endif /* PNGCONF_H */

