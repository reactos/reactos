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
//      Hardware capability check routines
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


namespace HwCaps
{
    HRESULT
    CheckDeviceLevel1(
        __in_ecount(1) const D3DCAPS9 &caps
        );


    MIL_FORCEINLINE bool
    IsLDDMDevice(
        __in_ecount(1) const D3DCAPS9 &caps
        )
    {
        return (caps.Caps2 & D3DCAPS2_CANSHARERESOURCE) != 0;
    }

    MIL_FORCEINLINE bool
    IsSWDevice(
        __in_ecount(1) const D3DCAPS9 &caps)
    {
        return caps.DeviceType == D3DDEVTYPE_SW;
    }

    MIL_FORCEINLINE bool
    CanMaskColorChannels(
        __in_ecount(1) const D3DCAPS9 &caps
        )
    {
        return (caps.PrimitiveMiscCaps & D3DPMISCCAPS_COLORWRITEENABLE) != 0;
    }

    MIL_FORCEINLINE bool
    CanHandleBlendFactor(
        __in_ecount(1) const D3DCAPS9 &caps
        )
    {
        return (caps.SrcBlendCaps & D3DPBLENDCAPS_BLENDFACTOR) != 0;
    }
}



