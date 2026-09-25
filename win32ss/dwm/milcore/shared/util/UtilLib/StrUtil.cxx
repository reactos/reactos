// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  File:       strutil.cxx
//-----------------------------------------------------------------------------

#include "Pch.h"

//------------------------------------------------------------------------------
//  Function:   AvalonStrCmpICW
//------------------------------------------------------------------------------
int AvalonStrCmpICW(LPCWSTR pch1, LPCWSTR pch2)
{
    int ch1, ch2;

    do {

        ch1 = *pch1++;
        if (ch1 >= L'A' && ch1 <= L'Z')
            ch1 += L'a' - L'A';

        ch2 = *pch2++;
        if (ch2 >= L'A' && ch2 <= L'Z')
            ch2 += L'a' - L'A';

    } while (ch1 && (ch1 == ch2));

    return ch1 - ch2;
}

//+----------------------------------------------------------------------------
//
//  Function:
//      DuplicateStringW
//
//  Synopsis:
//      Overflow and signedness-safe duplicate string.  Just enough memory will
//      be allocated to duplicate the given string.
//

// Undefine the macro that sets up allocation annotation tags
#undef DuplicateStringW

HRESULT DuplicateStringW(
    __range(1, STRSAFE_MAX_CCH) size_t cchMax,
    __in STRSAFE_LPCWSTR pSource,
    __deref_out STRSAFE_LPWSTR *ppDest,
    PERFMETERTAG mt
    )
{
    HRESULT hr = S_OK;
    size_t length;

    IFC(StringCchLengthW(pSource, cchMax, &length));
    IFC(HrMalloc(mt, length + 1, sizeof(WCHAR), reinterpret_cast<void**>(ppDest)));
    RtlCopyMemory(*ppDest, pSource, length * sizeof(WCHAR));
    (*ppDest)[length] = L'\0';

Cleanup:
    RRETURN(hr);
}



