// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//      Contains exported methods of visual target service.
//

#include "precomp.hpp"

DynArray<HWND> g_hwndMap;

//+-----------------------------------------------------------------------------
//
//  Method:     MilVisualTarget_AttachToHwnd
//
//------------------------------------------------------------------------------
HRESULT
WINAPI MilVisualTarget_AttachToHwnd(
    HWND hwnd
    )
{
    HRESULT hr = S_OK;

    g_csCompositionEngine.Enter();
    
    //
    // Check if specified hwnd is in the map. 
    //
    UINT idx = g_hwndMap.Find(0, hwnd);

    if (idx >= g_hwndMap.GetCount())
    {
        //
        // Specified hwnd was not found, add it to the map.
        // This means that hwnd was "free" for target connection 
        // and this request will succeed.
        //
        MIL_THR(g_hwndMap.Add(hwnd));
    }
    else
    {
        //
        // Specified hwnd is already in the map, which means
        // that another target already connected to this hwnd; 
        // connection request will fail.
        //
        MIL_THR(E_ACCESSDENIED);
    }
    
    g_csCompositionEngine.Leave();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:     MilVisualTarget_DetachFromHwnd
//
//------------------------------------------------------------------------------
HRESULT
WINAPI MilVisualTarget_DetachFromHwnd(
    HWND hwnd
    )
{
    HRESULT hr = S_OK;
    
    g_csCompositionEngine.Enter();
    
    //
    // Check if specified hwnd is in the map. 
    //
    UINT idx = g_hwndMap.Find(0, hwnd);

    if (idx >= g_hwndMap.GetCount())
    {
        //
        // This request was made for the window that is not in the map.
        //
        MIL_THR(E_INVALIDARG);
    }
    else
    {
        //
        // Remove hwnd from the map; this will make this hwnd
        // "free" for subsequent hosting.
        //
        g_hwndMap.Remove(hwnd);
    }

    g_csCompositionEngine.Leave();

    RRETURN(hr);
}




