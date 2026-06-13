// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains HW Startup/Shutdown routine implementations
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


//+-----------------------------------------------------------------------------
//
//  Function:
//      HwStartup
//
//  Synopsis:
//      Initialize global resources needed by HW code
//
//------------------------------------------------------------------------------
HRESULT
HwStartup()
{
    HRESULT hr = S_OK;

    IFC(CD3DDeviceManager::Create());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      HwShutdown
//
//  Synopsis:
//      Release global resources needed by HW code
//
//------------------------------------------------------------------------------
void
HwShutdown()
{
    CD3DDeviceManager::Delete();
}



