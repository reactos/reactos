// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Common definitions for Graphics Control dll.
//
//-----------------------------------------------------------------------------

#pragma once

#undef IFC
#undef IFCW32
#define IFC(x) { hr = (x); if (FAILED(hr)) goto Cleanup; }
#define IFCW32(x) { if (!x) { hr = HRESULT_FROM_WIN32(GetLastError()); goto Cleanup; } }

