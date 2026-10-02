// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//  
//  Description
//     Graphic Hardware Acceleration Tier declarations
//------------------------------------------------------------------------------

#pragma once

#define MIL_TIER(_Major, _Minor)    ((_Major << 16) | _Minor)

typedef int TierType;

class CDisplay;

namespace GraphicsAccelerationTier
{
    TierType
    GetTier(
        UINT uMemorySize,
        __in_ecount(1) const D3DCAPS9 &caps
        );
}


