// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains generic render utility routines.
//

#include "precomp.hpp"

DeclareTag(tagMILApiCalls, "MIL", "MIL API Calls log");

// API warnings turned on by default (DBG builds).
DeclareTagEx(tagMILApiCallWarnings, "MIL", "MIL API Calls warnings", TRUE);

#if DBG
void DbgCheckAPI(HRESULT hr)
{
    if(FAILED(hr))
    {
        // Convenient location to put a breakpoint when an API fails in debug
        // builds.
        TraceTag((tagMILVerbose, "API call failed."));;
    }
}
#endif


