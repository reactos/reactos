//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  muldiv32.h
//
//  Description: math routines for 32 bit signed and unsiged numbers.
//
//      MulDiv32(a,b,c) = (a * b) / c         (round down, signed)
//
//      MulDivRD(a,b,c) = (a * b) / c         (round down, unsigned)
//      MulDivRN(a,b,c) = (a * b + c/2) / c   (round nearest, unsigned)
//      MulDivRU(a,b,c) = (a * b + c-1) / c   (round up, unsigned)
//
//  Description:
//
//  History:
//       9/21/93    cjp     [curtisp] 
//       9/23/93    stl     [toddla]
//
//==========================================================================;

#ifndef _INC_MULDIV32
#define _INC_MULDIV32

#ifdef __cplusplus
extern "C"
{
#endif

extern LONG  FAR PASCAL MulDiv32(LONG  a,LONG  b,LONG  c);
extern DWORD FAR PASCAL MulDivRN(DWORD a,DWORD b,DWORD c);
extern DWORD FAR PASCAL MulDivRD(DWORD a,DWORD b,DWORD c);
extern DWORD FAR PASCAL MulDivRU(DWORD a,DWORD b,DWORD c);

#if defined(WIN32) || defined(_WIN32)
    // GDI32s MulDiv is the same as MulDivRN
    #define MulDivRN(a,b,c)   (DWORD)MulDiv((LONG)(a),(LONG)(b),(LONG)(c))
#endif

//
//  some code references these by other names.
//
#define muldiv32    MulDivRN
#define muldivrd32  MulDivRD
#define muldivru32  MulDivRU

#ifdef __cplusplus
}
#endif
#endif  // _INC_MULDIV32
