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
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(PenResource, MILRender, "Pen Resource");

MtDefine(CMilPenDuce, PenResource, "CMilPenDuce");

CMilPenDuce::~CMilPenDuce()
{
    UnRegisterNotifiers();
}

MtDefine(CMilPenData, MILRender, "CMilPenData");

//+-----------------------------------------------------------------------------
//
//  Function:
//      GetPen
//
//  Synopsis:
//      Creates (realizes) a pen that can be used for immediate rendering from a
//      set of potentially animate data.
//
//------------------------------------------------------------------------------
HRESULT
CMilPenDuce::GetPen(
    __deref_out_ecount(1) CMilPenRealization **ppRealization)
{
    HRESULT hr = S_OK;

    Assert(ppRealization);
    *ppRealization = NULL;

    if (IsDirty())
    {
        m_penRealization.Reset();

        //
        // Set non-animate properties
        //

        m_pen.SetStartCap(m_data.m_StartLineCap);
        m_pen.SetEndCap(m_data.m_EndLineCap);
        m_pen.SetDashCap(m_data.m_DashCap);
        m_pen.SetJoin( static_cast<MilLineJoin::Enum>(m_data.m_LineJoin) );
        m_pen.SetMiterLimit( static_cast<float>(m_data.m_MiterLimit) );

        //
        // Set thickness
        //

        // Initialize thickness to inline value
        double thickness = m_data.m_Thickness;

        // Override inline value with thickness resource, if one exists
        if (m_data.m_pThicknessAnimation)
        {
            thickness = *m_data.m_pThicknessAnimation->GetValue();
        }

        // Set thickness on pen
        m_pen.Set(static_cast<float>(thickness), static_cast<float>(thickness), 0);

        //
        // Dashes
        //

        if (m_data.m_pDashStyle)
        {
            IFC(m_data.m_pDashStyle->SetDashes(&m_pen));
        }
        else
        {
            IFC(m_pen.SetDashStyle(MilDashStyle::Solid));
        }

        //
        // Set new pen into realization
        //

        m_penRealization.SetPlainPen(&m_pen);
        m_penRealization.SetBrush(m_data.m_pBrush);

        SetDirty(FALSE);

    }
    *ppRealization = &m_penRealization;

Cleanup:
    Assert(FAILED(hr) ||  *ppRealization);
    RRETURN(hr);
}

