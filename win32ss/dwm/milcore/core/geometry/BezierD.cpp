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
//      Double precision Bezier curve with basic services
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

/////////////////////////////////////////////////////////////////////////////////
//
//              Implementation of CBezier
//+-------------------------------------------------------------------------------------------------
//
//  Member: CBezier::GetPoint
//
//  Synopsis: Get the point at a given parameter
//
//--------------------------------------------------------------------------------------------------
void
CBezier::GetPoint(
    _In_ double       t,
        // Parameter value
    __out_ecount(1) GpPointR &pt) const
        // Point there
{
    double s = 1 - t;
    double s2 = s * s;
    double t2 = t * t;
    
    pt =  m_ptB[0] * (s * s2) + m_ptB[1] * (3 * s2 * t) + 
          m_ptB[2] * (3 * s * t2) + m_ptB[3] * (t * t2);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBezier::GetPointAndDerivatives
//
//  Synopsis:
//      Get the point and 2 derivatives at a given parameter
//
//------------------------------------------------------------------------------
void
CBezier::GetPointAndDerivatives(
    __in double t,
        // Parameter value
    __out_ecount(3) GpPointR *pValues) const
        // Point, first derivative and second derivative there
{
    double s = 1 - t;
    double s2 = s * s;
    double t2 = t * t;
    double st = 2 * s * t;

    Assert(pValues);

   // Point
    pValues[0] = m_ptB[0] * (s * s2) +          // s^3
                 m_ptB[1] * (3 * s2 * t) +
                 m_ptB[2] * (3 * s * t2) +
                 m_ptB[3] * (t * t2);           // t^3

    // The derivatives are computed by differentiating the expressions above, w.r.t. t.
    // using the chain rule with ds/dt = -1.  The points are constant.
    
    // First derivative
    pValues[1] = (m_ptB[0] * (-s2) + 
                  m_ptB[1] * (s2 - st) + 
                  m_ptB[2] * (st - t2) +
                  m_ptB[3] * t2) * 3;

    // Second derivative
    pValues[2] = (m_ptB[0] * s + 
                 m_ptB[1] * (t - 2 * s) + 
                 m_ptB[2] * (s - 2 * t) + 
                 m_ptB[3] * t) * 3;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBezier::TrimToStartAt
//
//  Synopsis:
//      Set this curve as a portion of itself with a piece trimmed away from its
//      start.
//
//  Notes:
//      The original curve is defined on [0,1].  Here we compute the
//      coefficients of the restriction of that curves to the interval [t,1] as
//      a new Bezier curve.
//
//------------------------------------------------------------------------------
void
CBezier::TrimToStartAt(
    IN double t)              // Parameter value
{
    Assert(t > 0  &&  t < 1);
    double s = 1 - t;
    
    // The conventional De Casteljau algorithm (described in any book on Bezier curves) splits a 
    // curve at t and computes coefficients for both pieces as independed Bezier curves. 
    // Here we are only computing coefficients for the piece that corresponds to [t,1].

    m_ptB[0] = m_ptB[0] * s + m_ptB[1] * t;
    m_ptB[1] = m_ptB[1] * s + m_ptB[2] * t;
    m_ptB[2] = m_ptB[2] * s + m_ptB[3] * t;

    m_ptB[0] = m_ptB[0] * s + m_ptB[1] * t;
    m_ptB[1] = m_ptB[1] * s + m_ptB[2] * t;

    m_ptB[0] = m_ptB[0] * s + m_ptB[1] * t;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBezier::TrimToEndAt
//
//  Synopsis:
//      Set this curve as a portion of itself curve with a piece trimmed away
//      from its end.
//
//  Notes:
//      The original curve is defined on [0,1].  Here we compute the
//      coefficients of the restriction of that curves to the interval [0,t] as
//      a new Bezier curve.
//
//------------------------------------------------------------------------------
void
CBezier::TrimToEndAt(
    IN double t)              // Parameter value
{
    Assert(t > 0  &&  t < 1);
    double s = 1 - t;
    
    // The conventional De Casteljau algorithm (described in any book on Bezier curves) splits a 
    // curve at t and computes coefficients for both pieces as independed Bezier curves. 
    // Here we are only computing coefficients for the piece that corresponds to [t,1].

    m_ptB[3] = m_ptB[2] * s + m_ptB[3] * t;
    m_ptB[2] = m_ptB[1] * s + m_ptB[2] * t;
    m_ptB[1] = m_ptB[0] * s + m_ptB[1] * t;

    m_ptB[3] = m_ptB[2] * s + m_ptB[3] * t;
    m_ptB[2] = m_ptB[1] * s + m_ptB[2] * t;

    m_ptB[3] = m_ptB[2] * s + m_ptB[3] * t;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBezier::TrimBetween
//
//  Synopsis:
//      Trim the curve possibly at both ends
//
//  Returns:
//      true if trimmed normally, false if the resulting curve is degenerate.
//
//  Note:
//      When returning false, the control points are still being set.
//
//------------------------------------------------------------------------------
bool
CBezier::TrimBetween(
    __in double rStart,
        // Parameter value for the new start, must be between 0 and 1
    __in double rEnd)
        // Parameter value for the new end, must be between 0 and 1
{
    Assert(0 <= rStart);
    Assert(rStart <= rEnd);
    Assert(rEnd <= 1);
    
    if (rEnd - rStart < FUZZ)
    {
        // The trimmed curve degenerates to a point
        GetPoint(rStart, m_ptB[0]);
        m_ptB[1] = m_ptB[2] = m_ptB[3] = m_ptB[0];
        return false;
    }

    if (rEnd < 1)
    {
        TrimToEndAt(rEnd);
    }

    if (rStart > 0)
    {
        // It is safe to divide by rEnd since rEnd >= FUZZ
        TrimToStartAt(rStart / rEnd);
    }

    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBezierFragment::TryExtend
//
//  Synopsis:
//      Attempt to extend this fragment to include the given fragment (but only
//      if the two abut).
//
//  Returns:
//      true if this and the other fragment belong to the same (non-null) Bezier
//      and abut. false otherwise.
//
//------------------------------------------------------------------------------
bool
CBezierFragment::TryExtend(
    __in_ecount(1) const CBezierFragment &other,
        // The proposed extension
    bool fAppend
        // Should the extension be appended or prepended?
    )
{
    bool fAppended = false;

    if (Assigned() &&
        m_pBezierNoRef == other.m_pBezierNoRef)
    {
        if (fAppend)
        {
            if (m_end == other.m_start)
            {
                m_end = other.m_end;
                fAppended = true;
            }
        }
        else
        {
            if (m_start == other.m_end)
            {
                m_start = other.m_start;
                fAppended = true;
            }
        }
    }

    return fAppended;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBezierFragment::ConstructBezier
//
//  Synopsis:
//      Construct a CBezier that is equivalent to the given Bezier fragment.
//
//  Returns:
//      false if the Bezier is degenerate. true otherwise.
//
//------------------------------------------------------------------------------
bool
CBezierFragment::ConstructBezier(
    __out_ecount(1) CBezier *pBezier
    ) const
{
    pBezier->Copy(*m_pBezierNoRef);

    return pBezier->TrimBetween(m_start, m_end);
}


