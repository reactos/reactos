// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Platform dependent support for run time SIMD code generator.
//
//-----------------------------------------------------------------------------
#pragma once

class CProgram;

//+------------------------------------------------------------------------------
//
//  Class:
//      CJitterSupport
//
//  Synopsis:
//      Definitions for platform dependent routines used in jitter.
//      These routines should be implemented by jitter client.
//
//-------------------------------------------------------------------------------
class CJitterSupport
{
public:
    static CProgram* __STDCALL GetCurrentProgram();
    static __checkReturn HRESULT __STDCALL CodeAllocate(__in UINT32 cbSize, __out UINT8 **ppAddress);
    static void __STDCALL CodeFree(__in void *pAddress);
    static UINT8* __STDCALL MemoryAllocate(__in UINT32 cbSize, __out UINT32 & cbActualSize);
    static void __STDCALL MemoryFree(__in void *pAddress);
};

