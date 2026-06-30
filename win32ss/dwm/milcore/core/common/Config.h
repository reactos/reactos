// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//
//  Contents:  Abstraction of Avalon services that enables geometry algorithms
//             to be built outside Avalon.
//
//-----------------------------------------------------------------------------

#pragma once

// Memory allocation
#define AllocateBytes(size) WPFAllocType(BYTE *,ProcessHeap, Mt(CMemBlockData), size)
#define FreeBytes(p) WPFFree(ProcessHeap, p)


