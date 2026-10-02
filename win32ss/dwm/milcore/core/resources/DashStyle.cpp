// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      Implementation of of CMilDashStyleDuce
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(DashStyleResource, MILRender, "DashStyle Resource");

MtDefine(CMilDashStyleDuce, DashStyleResource, "CMilDashStyleDuce");

CMilDashStyleDuce::~CMilDashStyleDuce()
{
    UnRegisterNotifiers();
}

MtDefine(CMilDashStyleData, MILRender, "CMilDashStyleData");

//+-----------------------------------------------------------------------------
//
//  Function:
//      SetDashStyle
//
//  Synopsis:
//      Set the dash array on a given pen.
//
//------------------------------------------------------------------------------
HRESULT
CMilDashStyleDuce::SetDashes(
    __inout_ecount(1) CPlainPen *pPen)  // The pen to set dashes on
{
    HRESULT hr;

    Assert(pPen);

    // Initialize dash offset to inline value
    double offset = m_data.m_Offset;

    // Override inline value with dash offset resource, if one exists
    if (m_data.m_pOffsetAnimation)
    {
        offset = *m_data.m_pOffsetAnimation->GetValue();
    }

    // Set dash offset 
    pPen->SetDashOffset(static_cast<float>(offset));

    // Set dash array
    IFC(SetPenDoubleDashArray(pPen, 
                              m_data.m_pDashesData, 
                              m_data.m_cbDashesSize / sizeof(m_data.m_pDashesData[0])));

Cleanup:
    RRETURN(hr);
}



