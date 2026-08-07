// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Look-up tables for gamma conversion (converting a color channel
//      from sRGB to scRGB, or vice versa).
//

const UINT GammaLUT_scRGB_to_sRGB_size = 3355;
const REAL GammaLUT_scRGB_to_sRGB_scale = 3354.0f;

extern const BYTE GammaLUT_scRGB_to_sRGB [GammaLUT_scRGB_to_sRGB_size];

//+----------------------------------------------------------------------------
//
//  Function:  Convert_scRGB_Channel_To_sRGB_Byte
//
//  Synopsis:  Convert a color channel from scRGB to sRGB byte (0 to 255)
//
//  Returns:   Converted color channel
//
//-----------------------------------------------------------------------------

// WARNING!
//
// This file is generated! Please don't edit it by hand - instead
// edit the script and rerun it. See top of file for script location.

inline BYTE Convert_scRGB_Channel_To_sRGB_Byte(
    float rColorComponent
    )
{
    if (!(rColorComponent > 0.0f))   // Handles the NaN case
        return 0;
    else if (rColorComponent < 1.0f)
    {
        // GammaLUT_scRGB_to_sRGB_scale is generated so we do know that index is correct
        // and hence can tell "__bound" to satisfy PreFast.
        __bound UINT index = CFloatFPU::RoundWithHalvesUp(GammaLUT_scRGB_to_sRGB_scale * rColorComponent);
        return GammaLUT_scRGB_to_sRGB[index];
    }
    else
        return 255;
}


//+----------------------------------------------------------------------------
//
//  Function:  Get_Smallest_scRGB_Significant_for_sRGB
//
//  Synopsis:  Return smallest argument for Convert_scRGB_Channel_To_sRGB_Byte
//             that causes nonzero result
//-----------------------------------------------------------------------------

// WARNING!
//
// This file is generated! Please don't edit it by hand - instead
// edit the script and rerun it. See top of file for script location.

inline float Get_Smallest_scRGB_Significant_for_sRGB()
{
    float rColor = 0.5f / GammaLUT_scRGB_to_sRGB_scale;
    
    Assert(Convert_scRGB_Channel_To_sRGB_Byte(rColor) == 1);
    Assert(Convert_scRGB_Channel_To_sRGB_Byte(CFloatFPU::NextSmaller(rColor)) == 0);
    
    return rColor;
}

// WARNING!
//
// This file is generated! Please don't edit it by hand - instead
// edit the script and rerun it. See top of file for script location.

extern const REAL GammaLUT_sRGB_to_scRGB [256];



