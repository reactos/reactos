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

//typedef long long LONGLONG;
//typedef unsigned long long ULONGLONG;
//typedef long long *PLONGLONG;
//typedef unsigned long long *PULONGLONG;
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
#define __asm__
#define __volatile__(a)
#define __attribute__(a)
struct _KTHREAD { int foobar; };
struct _ETHREAD { int foobar; };
struct _EPROCESS { int foobar; };

#ifndef _DEBUG
#pragma function(_disable,_enable)
#pragma function(_inp,_inpw,_outp,_outpw)
#pragma function(_lrotl,_lrotr,_rotl,_rotr)
#pragma function(abs,fabs,labs)
#pragma function(memcpy,memcmp,memset)
#pragma function(strcat,strcmp,strcpy,strlen,_strset)
#pragma function(fmod,sqrt)
#pragma function(log,log10,pow,exp)
#pragma function(tan,atan,atan2,tanh)
#pragma function(cos,acos,cosh)
#pragma function(sin,asin,sinh)
#endif

#endif /*__GNUC__*/


#endif /* __CRT_TYPES__ */
