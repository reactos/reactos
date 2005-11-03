#ifndef __OT_LAYOUT_CONFIG_H__
#define __OT_LAYOUT_CONFIG_H__

 /************************************************************************/
 /************************************************************************/
 /*****                                                              *****/
 /*****                    CONFIGURATION MACROS                      *****/
 /*****                                                              *****/
 /************************************************************************/
 /************************************************************************/

#ifdef __cplusplus
#  define OTL_BEGIN_HEADER  extern "C" {
#else
#  define OTL_BEGIN_HEADER  /* nothing */
#endif

#ifdef __cplusplus
#  define OTL_END_HEADER   }
#else
#  define OTL_END_HEADER   /* nothing */
#endif

#ifndef OTL_API
#  ifdef __cplusplus
#    define  OTL_API( x )   extern "C"
#  else
#    define  OTL_API( x )   extern x
#  endif
#endif

#ifndef OTL_APIDEF
#  define  OTL_APIDEF( x )  x
#endif

#ifndef OTL_LOCAL
#  define OTL_LOCAL( x )   extern x
#endif

#ifndef OTL_LOCALDEF
#  define OTL_LOCALDEF( x )  x
#endif

#define  OTL_BEGIN_STMNT  do {
#define  OTL_END_STMNT    } while (0)
#define  OTL_DUMMY_STMNT  OTL_BEGIN_STMNT OTL_END_STMNT

#define  OTL_UNUSED( x )       (x)=(x)
#define  OTL_UNUSED_CONST(x)   (void)(x)


#include <limits.h>
#if UINT_MAX == 0xFFFFU
#  define OTL_SIZEOF_INT  2
#elif UINT_MAX == 0xFFFFFFFFU
#  define OTL_SIZEOF_INT  4
#elif UINT_MAX > 0xFFFFFFFFU && UINT_MAX == 0xFFFFFFFFFFFFFFFFU
#  define OTL_SIZEOF_INT  8
#else
#  error  "unsupported number of bytes in 'int' type!"
#endif

#if ULONG_MAX == 0xFFFFFFFFU
#  define  OTL_SIZEOF_LONG  4
#elif ULONG_MAX > 0xFFFFFFFFU && ULONG_MAX == 0xFFFFFFFFFFFFFFFFU
#  define  OTL_SIZEOF_LONG  8
#else
#  error  "unsupported number of bytes in 'long' type!"
#endif

#include <setjmp.h>
#define  OTL_jmp_buf   jmp_buf
#define  otl_setjmp    setjmp
#define  otl_longjmp   longjmp

/* */

#endif /* __OT_LAYOUT_CONFIG_H__ */
