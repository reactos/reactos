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

#define HAVE_LONGLONG
#define LONGLONG_DEFINED
#define LONGLONG    long long
#define ULONGLONG   unsigned long long

#else

#define LONGLONG_DEFINED
#define LONGLONG    long
#define ULONGLONG   unsigned long
#define PLONGLONG   long*
#define PULONGLONG  unsigned long*
#define __attribute__(a)
#define __volatile

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
