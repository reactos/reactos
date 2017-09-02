#ifndef _TIF_CONFIG_H_
#define _TIF_CONFIG_H_

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define as 0 or 1 according to the floating point format suported by the
   machine */
#define HAVE_IEEEFP 1

/* Define to 1 if you have the `jbg_newlen' function. */
#define HAVE_JBG_NEWLEN 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* Define to 1 if you have the <search.h> header file. */
#define HAVE_SEARCH_H 1

/* Define to 1 if you have the `setmode' function. */
#define HAVE_SETMODE 1

/* Define to 1 if you have the declaration of `optarg', and to 0 if you don't. */
#define HAVE_DECL_OPTARG 0

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Signed 64-bit type formatter */
#define TIFF_INT64_FORMAT "%I64d"

/* Signed 64-bit type */
#define TIFF_INT64_T signed __int64

/* Unsigned 64-bit type formatter */
#define TIFF_UINT64_FORMAT "%I64u"

/* Unsigned 64-bit type */
#define TIFF_UINT64_T unsigned __int64

#if _WIN64
/*
  Windows 64-bit build
*/

/* Pointer difference type */
#  define TIFF_PTRDIFF_T TIFF_INT64_T

/* The size of `size_t', as computed by sizeof. */
#  define SIZEOF_SIZE_T 8

/* Size type formatter */
#  define TIFF_SIZE_FORMAT TIFF_INT64_FORMAT

/* Unsigned size type */
#  define TIFF_SIZE_T TIFF_UINT64_T

/* Signed size type formatter */
#  define TIFF_SSIZE_FORMAT TIFF_INT64_FORMAT

/* Signed size type */
#  define TIFF_SSIZE_T TIFF_INT64_T

#else
/*
  Windows 32-bit build
*/

/* Pointer difference type */
#  define TIFF_PTRDIFF_T signed int

/* The size of `size_t', as computed by sizeof. */
#  define SIZEOF_SIZE_T 4

/* Size type formatter */
#  define TIFF_SIZE_FORMAT "%u"

/* Size type formatter */
#  define TIFF_SIZE_FORMAT "%u"

/* Unsigned size type */
#  define TIFF_SIZE_T unsigned int

/* Signed size type formatter */
#  define TIFF_SSIZE_FORMAT "%d"

/* Signed size type */
#  define TIFF_SSIZE_T signed int

#endif

/* Set the native cpu bit order */
#define HOST_FILLORDER FILLORDER_LSB2MSB

/* Visual Studio 2015 / VC 14 / MSVC 19.00 finally has snprintf() */
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#else
#define HAVE_SNPRINTF 1
#endif

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
# ifndef inline
#  define inline __inline
# endif
#endif

#define lfind _lfind

#pragma warning(disable : 4996) /* function deprecation warnings */

#endif /* _TIF_CONFIG_H_ */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
