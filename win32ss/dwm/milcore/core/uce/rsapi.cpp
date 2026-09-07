// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  File:   rsapi.cpp
//
//  Description:
//      Implementation of the graphics stream APIs for accessibility. 
//

#include "precomp.hpp"

typedef HRESULT (WINAPI *PFNDWMGETGRAPHICSSTREAMTRANSFORMHINT)(
    __in UINT uIndex,
    __out_ecount(1) MilMatrix3x2D *pTransform
    );

typedef HRESULT (WINAPI *PFNDWMGETGRAPHICSSTREAMCLIENT)(
    __in UINT uIndex,
    __out_ecount(1) UUID *pClientUuid
    );

typedef HRESULT (WINAPI *PFNDWMPATTACHMILCONTENT)(HWND hwnd);
typedef HRESULT (WINAPI *PFNDWMPDETACHMILCONTENT)(HWND hwnd);

//
// Critical section to synchronize access to the graphics stream globals
// 

CCriticalSection g_csGraphicsStream;


//+-----------------------------------------------------------------------------
//
//    Function: 
//        GetGraphicsStreamClient
//
//    Synopsis:
//        Enumerates graphics stream clients registered with 
//        the current session.
//
//------------------------------------------------------------------------------

HRESULT
GetGraphicsStreamClient(
    __in UINT uIndex,
    __out_ecount(1) UUID* pUuid
    )
{
    HRESULT hr = S_OK;

    //
    // Don't break on E_INVALIDARG -- this error code is used to report that
    // there are no more graphics streams to be enumerated.
    //

    BEGIN_MILINSTRUMENTATION_HRESULT_LIST
        E_INVALIDARG
    END_MILINSTRUMENTATION_HRESULT_LIST


    //
    // Do not attempt to load dwmapi.dll on down-level platforms.
    //

    if (!DWMAPI::CheckOS())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    //
    // Call the DWM to enumerate graphics streams.
    //
    
    IFC(DWMAPI::Load());
    
    PFNDWMGETGRAPHICSSTREAMCLIENT pfnGetGraphicsStreamClient = NULL;
    
    IFCW32(pfnGetGraphicsStreamClient = 
           reinterpret_cast<PFNDWMGETGRAPHICSSTREAMCLIENT>(
               DWMAPI::GetProcAddress(
                   "DwmGetGraphicsStreamClient"
                   )));
    
    IFC(pfnGetGraphicsStreamClient(
            uIndex,
            pUuid
            ));

Cleanup:
    if (FAILED(hr) && hr != E_INVALIDARG)
    {   
        TraceTag((tagMILWarning, 
                  "MilGraphicsStream_Enum: failed with HRESULT 0x%08x", 
                  hr
                  ));
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method: MilGraphicsContent_AttachToHwnd
//
//------------------------------------------------------------------------------
HRESULT WINAPI 
MilContent_AttachToHwnd(HWND hwnd)
{
    HRESULT hr = S_OK;
    
    //
    // We send hint only if we are not running on the downlevel platform. 
    // Otherwise, succeed and return.
    //
    if (DWMAPI::CheckOS())
    {
        //
        // Hint the DWM about the presence of MIL content in this window
        //
        
        if (SUCCEEDED(DWMAPI::Load()))
        {
            PFNDWMPATTACHMILCONTENT pfnAttachMilContent = 
                reinterpret_cast<PFNDWMPATTACHMILCONTENT>(DWMAPI::GetProcAddress(
                    "DwmAttachMilContent"
                    ));
            
            if (pfnAttachMilContent)
            {
                IGNORE_HR((*pfnAttachMilContent)(hwnd));
            }
        }    
    }
   
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method: MilGraphicsContent_DetachFromHwnd
//
//------------------------------------------------------------------------------
HRESULT WINAPI 
MilContent_DetachFromHwnd(HWND hwnd)
{
    HRESULT hr = S_OK;
    
    //
    // We send hint only if we are not running on the downlevel platform. 
    // Otherwise, succeed and return.
    //
    if (DWMAPI::CheckOS())
    {
        //
        // Hint the DWM about the absence of MIL content in this window
        //

        if (SUCCEEDED(DWMAPI::Load()))
        {
            PFNDWMPDETACHMILCONTENT pfnDetachMilContent = 
                reinterpret_cast<PFNDWMPDETACHMILCONTENT>(DWMAPI::GetProcAddress(
                    "DwmDetachMilContent"
                    ));
            
            if (pfnDetachMilContent)
            {
                IGNORE_HR((*pfnDetachMilContent)(hwnd));
            }
        }    
    }
    
    RRETURN(hr);
}




