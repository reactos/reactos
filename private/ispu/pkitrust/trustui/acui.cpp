//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       acui.cpp
//
//  Contents:   Entry point for the Authenticode UI Provider
//
//  History:    08-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <stdpch.h>
//+---------------------------------------------------------------------------
//
//  Function:   ACUIProviderInvokeUI
//
//  Synopsis:   Authenticode UI invokation entry point (see acui.h)
//
//  Arguments:  [pInvokeInfo] -- ACUI invoke information
//
//  Returns:    S_OK if the subject is trusted
//              TRUST_E_SUBJECT_NOT_TRUSTED if the subject is not trusted
//              Any other valid HRESULT
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT WINAPI ACUIProviderInvokeUI (PACUI_INVOKE_INFO pInvokeInfo)
{
    HRESULT hr;
    HWND    hDisplay;

    //
    // Initialize rich edit control DLL
    //
    if ( LoadLibrary(TEXT("riched32.dll")) == NULL )
    {
        return( E_FAIL );
    }

    //
    // Validate the invoke info structure
    //
    if (!(pInvokeInfo) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(ACUI_INVOKE_INFO, pInvokeInfo->cbSize, pPersonalTrustDB)))
    {
        return( E_INVALIDARG );
    }

    //
    // Pull out the display window handle and make sure it's valid
    //

    hDisplay = pInvokeInfo->hDisplay;
    if ( hDisplay == NULL )
    {
        if ( (hDisplay = GetDesktopWindow()) == NULL )
        {
            return( HRESULT_FROM_WIN32(GetLastError()) );
        }
    }

    //
    // Instantiate an invoke helper
    //

    CInvokeInfoHelper iih(pInvokeInfo, hr);
    IACUIControl*     pUI = NULL;

    if ( hr != S_OK )
    {
        return( hr );
    }

    //
    // Get the UI control and invoke the UI
    //

    hr = iih.GetUIControl(&pUI);
    if ( hr == S_OK )
    {
        hr = pUI->InvokeUI(hDisplay);
        iih.ReleaseUIControl(pUI);
    }

    return( hr );
}

