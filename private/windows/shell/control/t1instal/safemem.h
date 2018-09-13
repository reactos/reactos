/* Prototypes for "safe" (but slow) malloc/free routines to be used
 * in development of Large model Windows applications.
 *
 * lenoxb  5/28/93
 */

#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif



/***********
 ** Debug version of memory management functions.
 **
 */

#if TRACEMEM

void* SafeMalloc        _ARGS((INOUT   size_t,
                               INOUT   char *,
                               INOUT   short));
void* SafeReAlloc       _ARGS((INOUT   void*,
                               INOUT   size_t,
                               INOUT   char *,
                               INOUT   short));
void  SafeFree          _ARGS((INOUT   void*));
void  SafeListMemLeak   _ARGS((INOUT   void));
char* SafeStrdup        _ARGS((IN      char*,
                               INOUT   char *,
                               INOUT   short));

#define Malloc(size)          SafeMalloc(size, __FILE__, __LINE__)
#define Realloc(ptr, size)    SafeReAlloc(ptr, size, __FILE__, __LINE__)
#define Free(ptr)             SafeFree(ptr)
#define Strdup(ptr)           SafeStrdup(ptr, __FILE__, __LINE__)
#define ListMemLeak           SafeListMemLeak




#else
/***********
 ** Run-time version of memory management functions.
 **
 */


/*#include <stddef.h>*/
#include <stdlib.h>

#define Malloc(size)       malloc(size)
#define Realloc(ptr,size)  realloc(ptr, (size_t)(size))
#define Free               free
#define Strdup             _strdup
#define ListMemLeak()      ;

#endif
