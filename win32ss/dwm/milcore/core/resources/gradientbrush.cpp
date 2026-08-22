// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(GradientBrushResource, MILRender, "GradientBrush Resource");
MtDefine(CMilGradientBrushDuce, GradientBrushResource, "CMilGradientBrushDuce");


//+----------------------------------------------------------------------------
//
//  Member:
//      CMilGradientBrushDuce::GetSolidColorRealization
//
//  Synopsis:
//      Updates the realization with a solid color brush for gradient brushes
//      that have only one gradient stop (and thus are equivalent to a solid
//      color brush)
//

HRESULT
CMilGradientBrushDuce::GetSolidColorRealization(
    __in_ecount(1) CGradientColorData *pGradientStops,
    __inout_ecount(1) CMILBrushSolid *pBrushRealization
    )
{
    HRESULT hr = S_OK;

    Assert(pGradientStops);
    Assert(pGradientStops->GetCount() == 1);

    // Solid color
    const MilColorF *pColor = pGradientStops->GetColorsPtr();
    Assert(pColor);
    
    pBrushRealization->SetColor(pColor);

    RRETURN(hr);
}


