// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  File:       StrUtil.h
//-----------------------------------------------------------------------------

#pragma once

// Compare strings -- case insensitive only via ascii
// This is the same as shlwapi's StrCmpICW
int AvalonStrCmpICW(LPCWSTR pch1, LPCWSTR pch2);

//+----------------------------------------------------------------------------
//
//  Function:
//      DuplicateStringW
//
//  Synopsis:
//      Overflow and signedness-safe duplicate string.  Just enough memory will
//      be allocated to duplicate the given string.
//
//-----------------------------------------------------------------------------

HRESULT DuplicateStringW(
    __range(1, STRSAFE_MAX_CCH) size_t cchMax,
    __in STRSAFE_LPCWSTR pSource,
    __deref_out STRSAFE_LPWSTR *ppDest,
    PERFMETERTAG mt
    );

//+----------------------------------------------------------------------------
//
//  Function:
//      DuplicateStringWAnnotationHelper
//
//  Synopsis:
//      Since DuplicateStringW is often wrapped by a macro like IFC it is
//      impossible to just mark the end of a call to DuplicateStringW in macro.
//      This helper method is forced inline and has a further special
//      annotation marking that the prior annotation holds the most detailed
//      tag.
//
//  Notes:
//      Do not call directly.  Use DuplicateStringW macro.
//
//-----------------------------------------------------------------------------

__forceinline HRESULT DuplicateStringWAnnotationHelper(
    __range(1, STRSAFE_MAX_CCH) size_t cchMax,
    __in STRSAFE_LPCWSTR pSource,
    __deref_out STRSAFE_LPWSTR *ppDest,
    PERFMETERTAG mt
    )
{
    return DuplicateStringW(cchMax, pSource, ppDest, mt);
    // Direct reader to look at prior Allocate annotation.
    __annotation(L"Allocate", L"<END>", L"<UsePriorAnnotation>", L"DuplicateStringW");
}


#define DuplicateStringW(cchMax, pSource, ppDest, mt)                   \
    ( __annotation(L"Allocate", LSTRINGIZE(mt), L"DuplicateStringW"),   \
      DuplicateStringWAnnotationHelper(cchMax, pSource, ppDest, mt)   )



