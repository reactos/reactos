// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Software clipping objects
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CRectClipper, MILRender, "CRectClipper");



void CRectClipper::SetClip(
    __in_ecount(1) CMILSurfaceRect const &rc
    )
{
    Assert(rc.IsWellOrdered());

    m_rcClip = rc;
}

void CRectClipper::OutputSpan(INT y, INT xMin, INT xMax)
{
    Assert(m_pOutputSpan);

    INT xMinCur = xMin;
    INT xMaxCur = xMax;

    // do simple clip test to bounding rectangle

    if ((xMin < m_rcClip.right) &&
        (xMax > m_rcClip.left) &&
        (y >= m_rcClip.top) &&
        (y < m_rcClip.bottom))
    {
        if (xMin < m_rcClip.left)
        {
            xMinCur = m_rcClip.left;
        }

        if (xMax > m_rcClip.right)
        {
            xMaxCur = m_rcClip.right;
        }

        m_pOutputSpan->OutputSpan(y, xMinCur, xMaxCur);
    }
}

void CRectClipper::GetClipBounds(__in_ecount(1) CMILSurfaceRect *prc) const
{
    Assert(prc);

    *prc = m_rcClip;
}


void CRectClipper::SetOutputSpan(__in_ecount(1) COutputSpan *pSpan)
{
    m_pOutputSpan = pSpan;
}




