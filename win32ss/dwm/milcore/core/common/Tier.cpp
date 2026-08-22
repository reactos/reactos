// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics
//      $Keywords:
//
//  $Description:
//      Graphic Hardware Acceleration Tier definitions including tier
//      requirements and methods to determine tier of a given display
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


const TierType kDefaultTier = MIL_TIER(0, 0);


//+-----------------------------------------------------------------------------
//
//  Structure:
//      TierRequirements
//
//  Synopsis:
//      Collection of basic Tier minimum requirements
//
//------------------------------------------------------------------------------

struct TierRequirements
{
    TierType Tier;
    UINT MemorySize;
    bool IgnoreMemorySizeIfLDDM;
    DWORD PixelShaderVersion;
    DWORD VertexShaderVersion;
    bool (* pfnCheckSpecificCaps)(__in_ecount(1) const D3DCAPS9 &);
    bool CheckedRegistry;
};


bool CheckTier1_0SpecificCaps(__in_ecount(1) const D3DCAPS9 &caps);
bool CheckTier2_0SpecificCaps(__in_ecount(1) const D3DCAPS9 &caps);


TierRequirements g_rgTierRequirements[] = {
    // Tier 0 - no requirements
    {
        MIL_TIER(0, 0),
        0*1024*1024,            // 0 MB
        FALSE,                  // => LDDM doesn't override mem req
        0,                      // No Pixel Shader
        0,                      // No Vertex Shader
        NULL,
        true    // No need to check to registry overrides
    },
    // Tier 1 - 2005 advanced D3D hardware
    // 
    // Starting with WPF 4.0, we are now requiring PS2.0 support for hardware acceleration. 
    // For more details, see CD3DDeviceLevel1::Init.
    //
    {
        MIL_TIER(1, 0),
        60*1024*1024,           // 60 MB
        // Temporarily allow LDDM to override
        //  memory settings for Tier 1 as some drivers do not correctly report memory size.
        TRUE,                   // => LDDM does override mem req
        D3DPS_VERSION(2, 0),    // Pixel Shader 2.0
        0,                      // No Vertex shader
        CheckTier1_0SpecificCaps,
        false
    },
    // Tier 2 - 2005 advanced D3D hardware > 120MB
    {
        MIL_TIER(2, 0),
        120*1024*1024,          // 120 MB
        TRUE,                   //  => LDDM overrides mem req
        D3DPS_VERSION(2, 0),    // Pixel Shader 2.0
        D3DVS_VERSION(2, 0),    // Vertex Shader 2.0
        CheckTier2_0SpecificCaps,
        false
    },
};


//+-----------------------------------------------------------------------------
//
//  Member:
//      GraphicsAccelerationTier::GetTier
//
//  Synopsis:
//      Interate through tier requirement and find maximum tier that is
//      supported.
//

TierType
GraphicsAccelerationTier::GetTier(
    UINT uMemorySize,
    __in_ecount(1) const D3DCAPS9 &caps
    )
{
    Assert(g_rgTierRequirements[0].Tier == 0);

    UINT uTierIndex;

    //
    // We check the tiers from 1 onwards, and the last tier whose requirements
    // are met is the tier we report.  We do this to ensure that the
    // requirements for a given tier implicitly include requirements from all
    // previous tiers, and thus these requirements needn't be explicitly
    // described in each successive tier.
    //

    for (uTierIndex = 1;
         uTierIndex < ARRAY_SIZE(g_rgTierRequirements);
         uTierIndex++)
    {
        const TierRequirements &req = g_rgTierRequirements[uTierIndex];

        //
        // Check basic, common requirements for memory and shader versions
        //

        if (   (   (uMemorySize < req.MemorySize)
                && !(req.IgnoreMemorySizeIfLDDM && HwCaps::IsLDDMDevice(caps)))
            || (caps.PixelShaderVersion < req.PixelShaderVersion)
            || (caps.VertexShaderVersion < req.VertexShaderVersion)
           )
        {
            break;
        }

        //
        // Check level specific caps (if any)
        //

        if (   req.pfnCheckSpecificCaps
            && !req.pfnCheckSpecificCaps(caps))
        {
            break;
        }
    }

    // Current index is too high for given properties so return the next lower
    // tier
    return g_rgTierRequirements[uTierIndex-1].Tier;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      CheckTier1_0SpecificCaps
//
//  Synopsis:
//      Check minimum Hw required caps using common DeviceLevel1 check
//
//------------------------------------------------------------------------------

bool CheckTier1_0SpecificCaps(
    __in_ecount(1) const D3DCAPS9 &caps
    )
{   
    return (SUCCEEDED(HwCaps::CheckDeviceLevel1(caps)));
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CheckTier2_0SpecificCaps
//
//  Synopsis:
//      Check second tier caps requirements assuming first tier requirements are
//      already met.
//
//------------------------------------------------------------------------------

bool CheckTier2_0SpecificCaps(
    __in_ecount(1) const D3DCAPS9 &caps
    )
{
    // PS Version should already be at least 2.0 - this assert is to document
    // text requirements
    Assert(caps.PixelShaderVersion >= D3DPS_VERSION(1,1));

    // Other text requirements:
    //      1) sufficient blend stages
    //      2) blend factor support
    return (   (caps.MaxTextureBlendStages >= 4)
            && (HwCaps::CanHandleBlendFactor(caps)));
}




