// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Module Name:
*
*   This module contains common utility functionality for the API and
*   associated proxy (wrapper) classes. It also contains utilities for mapping
*   between v1 and v2 api structures and enums.
*
* Created:
*
*   12/03/2001 asecchia
*      Created it.
*
**************************************************************************/

#pragma once

HRESULT HrValidateInitializeCall(
    __in_opt HWND hwnd,
    MilWindowLayerType::Enum eWindowLayerType,
    MilRTInitialization::Flags dwFlags
    );


