////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*
Module Name:

    tools.cpp

Abstract:

    This module contains some useful functions for data manipulation.

Environment:

 */
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

//----------------

#include "tools.h"

//----------------

#ifdef _X86_

__declspec (naked)
void
__fastcall
_MOV_DD_SWP(
    void* a, // ECX
    void* b  // EDX
    )
{
  _asm {
    mov   eax,[edx]
    bswap eax
    mov   [ecx],eax
    ret
  }
}

__declspec (naked)
void
__fastcall
_MOV_DW_SWP(
    void* a, // ECX
    void* b  // EDX
    )
{
  _asm {
    mov   ax,[edx]
    rol   ax,8
    mov   [ecx],ax
    ret
  }
}

__declspec (naked)
void
__fastcall
_REVERSE_DD(
    void* a  // ECX
    )
{
  _asm {
    mov   eax,[ecx]
    bswap eax
    mov   [ecx],eax
    ret
  }
}

__declspec (naked)
void
__fastcall
_REVERSE_DW(
    void* a  // ECX
    )
{
  _asm {
    mov   ax,[ecx]
    rol   ax,8
    mov   [ecx],ax
    ret
  }
}

__declspec (naked)
void
__fastcall
_MOV_DW2DD_SWP(
    void* a, // ECX
    void* b  // EDX
    )
{
  _asm {
    mov   ax,[edx]
    rol   ax,8
    mov   [ecx+2],ax
    mov   [ecx],0
    ret
  }
}

__declspec (naked)
void
__fastcall
_MOV_MSF(
    void* a, // ECX
    void* b  // EDX
    )
{
  _asm {
    mov   eax,[edx]
    mov   [ecx],ax
    shr   eax,16
    mov   [ecx+2],al
    ret
  }
}

__declspec (naked)
void
__fastcall
_MOV_MSF_SWP(
    void* a, // ECX
    void* b  // EDX
    )
{
  _asm {
    mov   eax,[edx]
    mov   [ecx+2],al
    bswap eax
    shr   eax,8
    mov   [ecx],ax
    ret
  }
}

__declspec (naked)
void
__fastcall
_XCHG_DD(
    void* a, // ECX
    void* b  // EDX
    )
{
  _asm {
    mov   eax,[edx]
    xchg  eax,[ecx]
    mov   [edx],eax
    ret
  }
}

#endif _X86_

#ifdef __cplusplus
};
#endif
