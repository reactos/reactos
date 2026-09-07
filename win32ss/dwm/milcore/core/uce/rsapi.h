// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      The graphics stream APIs for accessibility. 
//
//------------------------------------------------------------------------------

#pragma once

// Enumerates available graphics stream clients.
HRESULT GetGraphicsStreamClient(
    __in UINT client,
    __out_ecount(1) UUID *pUuid
    );




