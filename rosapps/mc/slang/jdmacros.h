#ifndef _JD_MACROS_H_
#define _JD_MACROS_H_

/* This file defines some macros that I use with programs that link to 
 * the slang library.
 */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif 

#ifndef SLMEMSET
# ifdef HAVE_MEMSET
#  define SLMEMSET memset
# else
#  define SLMEMSET SLmemset
# endif
#endif

#ifndef SLMEMCHR
# ifdef HAVE_MEMCHR
#  define SLMEMCHR memchr
# else
#  define SLMEMCHR SLmemchr
# endif
#endif

#ifndef SLMEMCPY
# ifdef HAVE_MEMCPY
#  define SLMEMCPY memcpy
# else
#  define SLMEMCPY SLmemcpy
# endif
#endif

/* Note:  HAVE_MEMCMP requires an unsigned memory comparison!!!  */
#ifndef SLMEMCMP
# ifdef HAVE_MEMCMP
#  define SLMEMCMP memcmp
# else
#  define SLMEMCMP SLmemcmp
# endif
#endif

#if SLANG_VERSION < 9934
# define SLmemcmp jed_memcmp
# define SLmemcpy jed_memcpy
# define SLmemset jed_memset
# define SLmemchr jed_memchr
#endif

#ifndef SLFREE
# define SLFREE free
#endif

#ifndef SLMALLOC
# define SLMALLOC malloc
#endif

#ifndef SLCALLOC
# define SLCALLOC calloc
#endif

#ifndef SLREALLOC
# define SLREALLOC realloc
#endif

#endif				       /* _JD_MACROS_H_ */
