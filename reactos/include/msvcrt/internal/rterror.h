/* rterror.h */

#ifndef __MSVCRT_INTERNAL_RTERROR_H
#define __MSVCRT_INTERNAL_RTERROR_H


#define _RT_STACK       0   /* stack overflow */
#define _RT_NULLPTR     1   /* null pointer assignment */
#define _RT_FLOAT       2   /* floating point not loaded */
#define _RT_INTDIV      3   /* integer divide by 0 */
#define _RT_SPACEARG    4   /* not enough space for arguments */
#define _RT_SPACEENV    5   /* not enough space for environment */
#define _RT_ABORT       6   /* abnormal program termination */
#define _RT_THREAD      7   /* not enough space for thread data */
#define _RT_LOCK        8   /* unexpected multi-thread lock error */
#define _RT_HEAP        9   /* unexpected heap error */
#define _RT_OPENCON     10  /* unable to open console device */
#define _RT_NONCONT     11  /* non-continuable exception */
#define _RT_INVALDISP   12  /* invalid disposition of exception */
#define _RT_ONEXIT      13  /* insufficient heap to allocate
                             * initial table of function pointers
                             * used by _onexit()/atexit(). */
#define _RT_PUREVIRT    14  /* pure virtual function call attempted
                             * (C++ error) */
#define _RT_STDIOINIT   15  /* not enough space for stdio initialization */
#define _RT_LOWIOINIT   16  /* not enough space for lowio initialization */

void _amsg_exit (int errnum);


#endif  /* __MSVCRT_INTERNAL_RTERROR_H */
