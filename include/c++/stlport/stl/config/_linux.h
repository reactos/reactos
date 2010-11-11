#ifndef __stl_config__linux_h
#define __stl_config__linux_h

#define _STLP_PLATFORM "Linux"

#include <features.h>

/* This is defined wether library in use is glibc or not.
   This may be treated as presence of GNU libc compatible
   header files (these define is not really intended to check
   for the presence of a particular library, but rather is used
   to define an INTERFACE.) */
#ifndef _STLP_USE_GLIBC
#  define _STLP_USE_GLIBC 1
#endif

#ifndef _STLP_USE_STDIO_IO
#  define _STLP_USE_UNIX_IO
#endif

/* #define _STLP_USE_STDIO_IO */

/* If not explicitly specified otherwise, work with threads
 */
#if !defined(_STLP_NO_THREADS) && !defined(_REENTRANT)
#  define _REENTRANT
#endif

#if defined(_REENTRANT) && !defined(_PTHREADS)
# define _PTHREADS
#endif

#ifdef __UCLIBC__ /* uClibc 0.9.27 */
#  define _STLP_USE_UCLIBC 1
#  if !defined(__UCLIBC_HAS_WCHAR__)
#    ifndef _STLP_NO_WCHAR_T
#      define _STLP_NO_WCHAR_T
#    endif
#    ifndef _STLP_NO_NATIVE_MBSTATE_T
#      define _STLP_NO_NATIVE_MBSTATE_T
#    endif
#    ifndef _STLP_NO_NATIVE_WIDE_STREAMS
#      define _STLP_NO_NATIVE_WIDE_STREAMS
#    endif
#  endif /* __UCLIBC_HAS_WCHAR__ */
   /* Hmm, bogus _GLIBCPP_USE_NAMESPACES seems undefined... */
#  define _STLP_VENDOR_GLOBAL_CSTD 1
#endif


#if defined(_PTHREADS)
#  define _STLP_THREADS
#  define _STLP_PTHREADS
/*
#  ifndef __USE_UNIX98
#    define __USE_UNIX98
#  endif
*/
/* This feature exist at least since glibc 2.2.4 */
/* #  define __FIT_XSI_THR */ /* Unix 98 or X/Open System Interfaces Extention */
#  ifdef __USE_XOPEN2K
/* The IEEE Std. 1003.1j-2000 introduces functions to implement spinlocks. */
#   ifndef __UCLIBC__ /* There are no spinlocks in uClibc 0.9.27 */
#     define _STLP_USE_PTHREAD_SPINLOCK
#   else
#     ifndef _STLP_DONT_USE_PTHREAD_SPINLOCK
        /* in uClibc (0.9.26) pthread_spinlock* declared in headers
         * but absent in library */
#       define _STLP_DONT_USE_PTHREAD_SPINLOCK
#     endif
#   endif
#   ifndef _STLP_DONT_USE_PTHREAD_SPINLOCK
#     define _STLP_USE_PTHREAD_SPINLOCK
#     define _STLP_STATIC_MUTEX _STLP_mutex
#   endif
/* #   define __FIT_PSHARED_MUTEX */
#  endif
#endif

/* Endiannes */
#include <endian.h>
#if !defined(__BYTE_ORDER) || !defined(__LITTLE_ENDIAN) || !defined(__BIG_ENDIAN)
#  error "One of __BYTE_ORDER, __LITTLE_ENDIAN and __BIG_ENDIAN undefined; Fix me!"
#endif

#if ( __BYTE_ORDER == __LITTLE_ENDIAN )
#  define _STLP_LITTLE_ENDIAN 1
#elif ( __BYTE_ORDER == __BIG_ENDIAN )
#  define _STLP_BIG_ENDIAN 1
#else
#  error "__BYTE_ORDER neither __BIG_ENDIAN nor __LITTLE_ENDIAN; Fix me!"
#endif

#if defined(__GNUC__) && (__GNUC__ < 3)
#  define _STLP_NO_NATIVE_WIDE_FUNCTIONS 1
#endif

#ifdef __GLIBC__
#  if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 3) || (__GLIBC__ > 2)
/* From glibc 2.3.x default allocator is malloc_alloc, if was not defined other */
#    if !defined(_STLP_USE_MALLOC) && !defined(_STLP_USE_NEWALLOC) && !defined(_STLP_USE_PERTHREAD_ALLOC) && !defined(_STLP_USE_NODE_ALLOC)
#      define _STLP_USE_MALLOC 1
#    endif
#  endif
/* Following platforms has no long double:
 *   - Alpha
 *   - PowerPC
 *   - SPARC, 32-bits (64-bits platform has long double)
 *   - MIPS, 32-bits
 *   - ARM
 *   - SH4
 */
#  if defined(__alpha__) || \
      defined(__ppc__) || defined(PPC) || defined(__powerpc__) || \
      ((defined(__sparc) || defined(__sparcv9) || defined(__sparcv8plus)) && !defined ( __WORD64 ) && !defined(__arch64__)) /* ? */ || \
      (defined(_MIPS_SIM) && (_MIPS_SIM == _ABIO32)) || \
      defined(__arm__) || \
      defined(__sh__)
 /* #  if defined(__NO_LONG_DOUBLE_MATH) */
#    define _STLP_NO_LONG_DOUBLE
#  endif
#endif


#endif /* __stl_config__linux_h */
