// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Description:
*
*   This module contains utilities for mapping between v1 and v2 api structures
*   and enums.
*
* Created:
*
*   12/03/2001 asecchia
*      Created it.
*
**************************************************************************/

#include "precomp.hpp"

/**************************************************************************
*
* Function Description:
*
*   Validate that an Initialize method is expected at this time and
*   that the common parameters are meaningful:
*    - The HWND may not be NULL unless we are in full screen mode
*    - dwFlags may only specify known flags.
*
*
**************************************************************************/

HRESULT HrValidateInitializeCall(
    __in_opt HWND hwnd,
    MilWindowLayerType::Enum eWindowLayerType,
    MilRTInitialization::Flags dwFlags
    )
{
    HRESULT hr = S_OK;

    // Check parameters

    // Make sure the window is valid if this is not full screen
    // or the hwnd is not NULL
    if (hwnd && !IsWindow(hwnd))
    {
        IFC(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
    }

    // Make sure that no more than one of MilRTInitialization::UseRefRast,
    // MilRTInitialization::UseRgbRast, or MilRTInitialization::SoftwareOnly is set
    DWORD dwMask = dwFlags & (MilRTInitialization::SoftwareOnly | MilRTInitialization::UseRefRast | MilRTInitialization::UseRgbRast);

    if (dwMask & (dwMask - 1))
    {
        IFC(E_INVALIDARG);
    }

    // Ensure only these flags are set
    if (dwFlags & ~(
        MilRTInitialization::TypeMask |
        MilRTInitialization::PresentImmediately |
        MilRTInitialization::PresentRetainContents |
        MilRTInitialization::NeedDestinationAlpha |
        MilRTInitialization::SingleThreadedUsage |
        MilRTInitialization::RenderNonClient |
        MilRTInitialization::DisableDisplayClipping |
        MilRTInitialization::DisableMultimonDisplayClipping |
        MilRTInitialization::IsDisableMultimonDisplayClippingValid |
        MilRTInitialization::UseRefRast |
        MilRTInitialization::UseRgbRast |
        MilRTInitialization::PresentUsingMask
        ))
    {
        IFC(E_INVALIDARG);
    }

Cleanup:
    RRETURN(hr);
}




