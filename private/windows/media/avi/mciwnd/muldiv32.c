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
//  muldiv32.c
//
//  Description: math routines for 32 bit signed and unsiged numbers.
//
//      MulDiv32(a,b,c) = (a * b) / c         (round down, signed)
//
//      MulDivRD(a,b,c) = (a * b) / c         (round down, unsigned)
//      MulDivRN(a,b,c) = (a * b + c/2) / c   (round nearest, unsigned)
//      MulDivRU(a,b,c) = (a * b + c-1) / c   (round up, unsigned)
//
//  History:
//       9/21/93    cjp     [curtisp]
//       9/23/93    stl     [toddla]
//
//==========================================================================;

#include <windows.h>
#include "muldiv32.h"

#if !defined(_WIN32) && !defined(_WIN32)

#pragma warning(disable:4035 4704)

#define ASM66 _asm _emit 0x66 _asm
#define DB    _asm _emit

#define EAX_TO_DXAX \
    DB      0x66    \
    DB      0x0F    \
    DB      0xA4    \
    DB      0xC2    \
    DB      0x10

//--------------------------------------------------------------------------;
//
//  LONG MulDiv32(a,b,c)    = (a * b) / c
//
//--------------------------------------------------------------------------;

LONG FAR PASCAL MulDiv32(LONG a,LONG b,LONG c)
{
    ASM66   mov     ax,word ptr a   //  mov  eax, a
    ASM66   mov     bx,word ptr b   //  mov  ebx, b
    ASM66   mov     cx,word ptr c   //  mov  ecx, c
    ASM66   imul    bx              //  imul ebx
    ASM66   idiv    cx              //  idiv ecx
    EAX_TO_DXAX

} // MulDiv32()

//--------------------------------------------------------------------------;
//
//  DWORD MulDivRN(a,b,c)   = (a * b + c/2) / c
//
//--------------------------------------------------------------------------;

DWORD FAR PASCAL MulDivRN(DWORD a,DWORD b,DWORD c)
{
    ASM66   mov     ax,word ptr a   //  mov  eax, a
    ASM66   mov     bx,word ptr b   //  mov  ebx, b
    ASM66   mov     cx,word ptr c   //  mov  ecx, c
    ASM66   mul     bx              //  mul  ebx
    ASM66   mov     bx,cx           //  mov  ebx,ecx
    ASM66   shr     bx,1            //  sar  ebx,1
    ASM66   add     ax,bx           //  add  eax,ebx
    ASM66   adc     dx,0            //  adc  edx,0
    ASM66   div     cx              //  div  ecx
    EAX_TO_DXAX

} // MulDiv32()

//--------------------------------------------------------------------------;
//
//  DWORD MulDivRU(a,b,c)   = (a * b + c-1) / c
//
//--------------------------------------------------------------------------;

DWORD FAR PASCAL MulDivRU(DWORD a,DWORD b,DWORD c)
{
    ASM66   mov     ax,word ptr a   //  mov  eax, a
    ASM66   mov     bx,word ptr b   //  mov  ebx, b
    ASM66   mov     cx,word ptr c   //  mov  ecx, c
    ASM66   mul     bx              //  mul  ebx
    ASM66   mov     bx,cx           //  mov  ebx,ecx
    ASM66   dec     bx              //  dec  ebx
    ASM66   add     ax,bx           //  add  eax,ebx
    ASM66   adc     dx,0            //  adc  edx,0
    ASM66   div     cx              //  div  ecx
    EAX_TO_DXAX

} // MulDivRU32()

//--------------------------------------------------------------------------;
//
//  DWORD MulDivRD(a,b,c)   = (a * b) / c
//
//--------------------------------------------------------------------------;

DWORD FAR PASCAL MulDivRD(DWORD a,DWORD b,DWORD c)
{
    ASM66   mov     ax,word ptr a   //  mov  eax, a
    ASM66   mov     bx,word ptr b   //  mov  ebx, b
    ASM66   mov     cx,word ptr c   //  mov  ecx, c
    ASM66   mul     bx              //  mul  ebx
    ASM66   div     cx              //  div  ecx
    EAX_TO_DXAX

} // MulDivRD32()

#pragma warning(default:4035 4704)

#else   // _WIN32

#include <largeint.h>

//--------------------------------------------------------------------------;
//
//  LONG MulDiv32(a,b,c)    = (a * b) / c
//
//--------------------------------------------------------------------------;

LONG FAR PASCAL MulDiv32(LONG a,LONG b,LONG c)
{
    LARGE_INTEGER lRemain;

    return LargeIntegerDivide(
        EnlargedIntegerMultiply(a,b),
        ConvertLongToLargeInteger(c),
        &lRemain).LowPart;

} // MulDiv32()

//--------------------------------------------------------------------------;
//
//  DWORD MulDivRD(a,b,c)   = (a * b) / c
//
//--------------------------------------------------------------------------;

DWORD FAR PASCAL MulDivRD(DWORD a,DWORD b,DWORD c)
{
    return ExtendedLargeIntegerDivide(
        EnlargedUnsignedMultiply(a,b), c, &a).LowPart;

} // MulDivRD()

//--------------------------------------------------------------------------;
//
//  DWORD MulDivRU(a,b,c)   = (a * b + c-1) / c
//
//--------------------------------------------------------------------------;

DWORD FAR PASCAL MulDivRU(DWORD a,DWORD b,DWORD c)
{
    return ExtendedLargeIntegerDivide(
        LargeIntegerAdd(
            EnlargedUnsignedMultiply(a,b),
            ConvertUlongToLargeInteger(c-1)),
        c,&a).LowPart;

} // MulDivRU()

#if 0 // we use Win32 GDI MulDiv function, not this.
//--------------------------------------------------------------------------;
//
//  DWORD MulDivRN(a,b,c)   = (a * b + c/2) / c
//
//--------------------------------------------------------------------------;

DWORD FAR PASCAL MulDivRN(DWORD a,DWORD b,DWORD c)
{
    return ExtendedLargeIntegerDivide(
        LargeIntegerAdd(
            EnlargedUnsignedMultiply(a,b),
            ConvertUlongToLargeInteger(c/2)),
        c,&a).LowPart;

} // MulDivRN()

#endif

#endif  // _WIN32
