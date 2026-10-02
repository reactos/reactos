// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        Contains implementation for failed Asserts and RIPs.  See
//                   Assert.inl for more.
//

#include "Pch.h"

// Include actual implementation for AssertA/W which this library provides. An
// inlined source file is used because the debug DLL also needs the logic, but
// can't link to this library.
#include "Assert.inl"



