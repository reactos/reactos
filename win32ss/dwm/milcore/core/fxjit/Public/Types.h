// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Platform independent data types and macros (rough)
//
//-----------------------------------------------------------------------------
#pragma once

#if !defined(_BASETSD_H_)

typedef __int32 BOOL;


typedef __int8  INT8 ;
typedef __int16 INT16;
typedef __int32 INT32;
typedef __int64 INT64;

typedef unsigned short WCHAR;

typedef unsigned __int8  UINT8 ;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;

#if defined(_WIN64)
    typedef __int64 INT_PTR;
    typedef unsigned __int64 UINT_PTR;
    typedef unsigned __int64 size_t;
#else
    typedef __w64 __int32 INT_PTR;
    typedef __w64 unsigned __int32 UINT_PTR;
    typedef __w64 unsigned __int32 size_t;
#endif

#define NULL    0

#define FALSE   0
#define TRUE    1

#define UINT_MAX                    0xffffffff
#define UNREFERENCED_PARAMETER(x)   (x)

#ifndef HRESULT
#define HRESULT long
#endif

#ifndef S_OK
#define S_OK                        HRESULT(0x00000000L)
#endif

#ifndef S_FALSE
#define S_FALSE                     HRESULT(0x00000001L)
#endif

#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY               HRESULT(0x8007000EL)
#endif

#ifndef E_FAIL
#define E_FAIL                      HRESULT(0x80004005L)
#endif

#ifndef FAILED
#define FAILED(hr)  (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#endif

#ifndef RRETURN
#define RRETURN(hr) return (hr)
#endif

#ifndef IFC
#define IFC(x) { hr = (x); if (FAILED(hr)) goto Cleanup; }
#endif

#ifndef IFCOOM
#define IFCOOM(x) if ((x) == NULL) {hr = E_OUTOFMEMORY; goto Cleanup;}
#endif

#ifndef ReleaseInterface
#define ReleaseInterface(p) if ((p)) {(p)->Release(); (p) = NULL;}
#endif

#endif //_BASETSD_H_

#ifndef __STDCALL
#define __STDCALL  __stdcall
#endif


