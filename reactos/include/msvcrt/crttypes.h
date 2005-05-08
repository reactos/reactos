/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/msvcrt/crttypes.h
 * PURPOSE:         
 * PROGRAMMER:      
 * UPDATE HISTORY: 
 *                
 */

#ifndef __CRT_TYPES__
#define __CRT_TYPES__


#ifdef  __GNUC__

/*
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long *PLONGLONG;
typedef unsigned long long *PULONGLONG;
*/

#define HAVE_LONGLONG
#define LONGLONG_DEFINED
#define LONGLONG    long long
#define ULONGLONG   unsigned long long
#define PLONGLONG   long long *
#define PULONGLONG  unsigned long long *

#else /*__GNUC__*/

#define LONGLONG_DEFINED
#define LONGLONG    __int64
#define ULONGLONG   unsigned __int64
#define PLONGLONG   __int64*
#define PULONGLONG  unsigned __int64*
#define __attribute__(a)
#define __volatile

#define inline __inline
#define __asm__ #error
#define __volatile__(a)
#define __attribute__(a)
#if 0
/* TMN: What on earth do these structures do in a user-mode header?! */
struct _KTHREAD { int foobar; };
struct _ETHREAD { int foobar; };
struct _EPROCESS { int foobar; };
#endif

#endif /*__GNUC__*/


#endif /* __CRT_TYPES__ */
