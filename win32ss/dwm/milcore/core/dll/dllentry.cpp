// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore.dll entry point
//
//

#include "precomp.hpp"

extern "C"
BOOL
__stdcall
DllMain(
    HINSTANCE   dllHandle,
    ULONG       reason,
    __in_ecount(1) CONTEXT* /* context */
    )
{
    return MILCoreDllMain(
        dllHandle,
        reason
        );
}



