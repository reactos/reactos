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

MtDefine(SolidColorBrushResource, MILRender, "SolidColorBrush Resource");

MtDefine(CMilSolidColorBrushDuce, SolidColorBrushResource, "CMilSolidColorBrushDuce");

CMilSolidColorBrushDuce::~CMilSolidColorBrushDuce()
{
    UnRegisterNotifiers();
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CMilSolidColorBrushDuce::GetBrushRealizationInternal
//
//  Synopsis:
//      Realizes each property of the brush and sets it on the
//       cached realization
//

HRESULT
CMilSolidColorBrushDuce::GetBrushRealizationInternal(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    HRESULT hr = S_OK;


    FLOAT opacity;
    MilColorF realizedColor;

    IFC(GetOpacity(
        m_data.m_Opacity,
        m_data.m_pOpacityAnimation,
        &opacity
        ));

    realizedColor = *(GetColor(&m_data.m_Color, m_data.m_pColorAnimation));

    realizedColor.a *= opacity;

    m_solidBrushRealization.SetColor(reinterpret_cast<MilColorF*>(&realizedColor));

    *ppBrushRealizationNoRef = &m_solidBrushRealization;

Cleanup:
    RRETURN(hr);
}




