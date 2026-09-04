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
//      Geometry: Some 2D geometry helper routines.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

const UINT FLOAT_QNAN_UINT = 0xffffffff;

const float FLOAT_QNAN = *reinterpret_cast<const float *>(&FLOAT_QNAN_UINT);

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Implementation of GpPointR methods
//
//------------------------------------------------------------------------------
GpPointR::GpPointR(
    __in_ecount(1) const MilPoint2F &point,
        // Raw point
    __in_ecount_opt(1) const CMILMatrix* pMatrix)
        // Transformation to apply to it (NULL OK)
    :X(point.X), Y(point.Y)
{
    if (pMatrix)
    {
        TransformPoint(*pMatrix, point, *this);
    }
} 

GpPointR::GpPointR(
    __in_ecount(1) const     GpPointR &point,       // Raw point
    __in_ecount_opt(1) const CMILMatrix* pMatrix)   // transformation matrix
    :X(point.X), Y(point.Y)
{
    if (pMatrix)
    {
        TransformPoint(*pMatrix, point, OUT *this);
    }
} 


GpReal GpPointR::Norm() const
{
    return HYPOT(X, Y);
}
    
GpPointR GpPointR::operator/(IN GpReal k) const
{
    k = 1 / k;
    return GpPointR(k * X, k * Y);
}
    
VOID GpPointR::operator/=(IN GpReal k)
{
    k = 1 / k;
    X *= k;
    Y *= k;
}

#ifdef DBG
// For debug - check if the length is as expected
// Treat NaNs as success
bool GpPointR::DbgIsOfLength(
    GpReal r,                 // In: The expected length
    GpReal rTolerance) const  // In: Tolerated relative error
{
    r *= r;
    return !(fabs(X * X + Y * Y - r) > r * rTolerance);
}
#endif

VOID
TransformPoint(
    __in_ecount(1) const CMILMatrix &mat,
        // In: Transformation matrix
    __in_ecount(1) const MilPoint2F &ptF,
        // In: Input point
    __out_ecount(1) GpPointR & ptR
        // Out: The transformed point 
    )   
{
    ptR.X = mat.GetM11() * ptF.X + mat.GetM21() * ptF.Y + mat.GetDx();
    ptR.Y = mat.GetM12() * ptF.X + mat.GetM22() * ptF.Y + mat.GetDy();
}

VOID
TransformPoint(
    __in_ecount(1) const CMILMatrix &mat,
        // In: Transformation matrix
    __in_ecount(1) const MilPoint2D &pt,
        // In: Input point
    __out_ecount(1) MilPoint2D & ptOut
        // Out: The transformed point 
    )   
{
    double x = mat.GetM11() * pt.X + mat.GetM21() * pt.Y + mat.GetDx();
    ptOut.Y = mat.GetM12() * pt.X + mat.GetM22() * pt.Y + mat.GetDy();
    ptOut.X = x;
}

VOID
TransformPoint(
    __in_ecount(1) const CMILMatrix &mat,  // Transformation matrix
    __in_ecount(1) const GpPointR &pt,     // Input point
    __out_ecount(1) GpPointR      &ptOut   // The transformed point 
    )   
{
    double x = mat.GetM11() * pt.X + mat.GetM21() * pt.Y + mat.GetDx();
    ptOut.Y = mat.GetM12() * pt.X + mat.GetM22() * pt.Y + mat.GetDy();
    ptOut.X = x;
}

VOID
TransformPoint(
    __in_ecount(1) const CBaseMatrix &mat,
        // In: Transformation matrix
    __inout_ecount(1) MilPoint2F &pt
        // Out/In: Input point
    )   
{
        REAL x = pt.X;
        pt.X = mat.GetM11() * x + mat.GetM21() * pt.Y + mat.GetDx();
        pt.Y = mat.GetM12() * x + mat.GetM22() * pt.Y + mat.GetDy();
}

VOID
TransformPoints(
    __in_ecount(1) const CMILMatrix &mat,
        // In: Transformation matrix
    int count,
        // In: Number of points
    __in_ecount(count) const MilPoint2F *pptF,
        // In: Input points
    __out_ecount_full(count) GpPointR *pptR
        // Out: The transformed points 
    )   
{
    for (int i = 0;  i < count;  i++)
    {
        REAL x = pptF[i].X;
        pptR[i].X = mat.GetM11() * x + mat.GetM21() * pptF[i].Y + mat.GetDx();
        pptR[i].Y = mat.GetM12() * x + mat.GetM22() * pptF[i].Y + mat.GetDy();
    }
}

VOID
TransformPoints(
    __in_ecount(1) const CMILMatrix &mat,
        // In: Transformation matrix
    int count,
        // In: Number of points
    __inout_ecount_full(count) MilPoint2F *pptF
        // In/Out: Input points
    )   
{
    for (int i = 0;  i < count;  i++)
    {
        REAL x = pptF[i].X;
        pptF[i].X = mat.GetM11() * x + mat.GetM21() * pptF[i].Y + mat.GetDx();
        pptF[i].Y = mat.GetM12() * x + mat.GetM22() * pptF[i].Y + mat.GetDy();
    }
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get an upper bound on the squared scaling factor in all direction
//
//  Notes:
//      The exact maximal scale factor is expensive to compute, so here we
//      compute a bound on the cheap. In the worst case this bound is twice the
//      true squared maximum of the scale factor.
// 
//      Ignore the translation part, it's irrelevant. Let alpha be the
//      direction where the transformation scales most.  Then an arbitrary unit
//      vector (cos(t), sin(t)) is scaled by a factor f(t), equal to the length
//      of the vector (a*cos(t-alpha), b*sin(t-alpha)), where a and b are the
//      maximal and minimal scale factors, respectively. We don't know a, b or
//      alpha, but we can probe f(t) and estimate a from the results.  Since
//      f(t) repeats itself every 180 degrees, either f(0) or f(90) is off the
//      maximum by no more than 45 degrees.  In other words, (t-alpha) <= 45
//      degrees, hence f(t) >= |a*cos(45)| = a*sqrt(1/2). So square(a) >= 2 *
//      (max(f(0), f(90))). 
//
//------------------------------------------------------------------------------
REAL GetSqScaleBound(
    __in_ecount(1) const CMILMatrix & mat
    )
{
    // f(0) = length of (1,0)*matrix
    REAL r = mat.GetM11()*mat.GetM11() + mat.GetM12()*mat.GetM12(); 

    // f(90) = length of (0,1)*matrix
    REAL s = mat.GetM21()*mat.GetM21() + mat.GetM22()*mat.GetM22(); 

    if (r > s)
    {
        r = s;
    }
    return 2 * r;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the distance from a circular arc's endpoints to the control points
//      of the Bezier arc that approximates it, as a fraction of the arc's
//      radius.
//
//      Since the result is relative to the arc's radius, it depends strictly on
//      the arc's angle. The arc is assumed to be of 90 degrees of less, so the
//      angle is determined by the cosine of that angle, which is derived from
//      rDot = the dot product of two radius vectors.  We need the Bezier curve
//      that agrees with the arc's points and tangents at the ends and midpoint. 
//      Here we compute the distance from the curve's endpoints to its control
//      points.
//
//      Since we are looking for the relative distance, we can work on the unit
//      circle. Place the center of the circle at the origin, and put the X axis
//      as the bisector between the 2 vectors.  Let a be the angle between the
//      vectors.  Then the X coordinates of the 1st & last points are cos(a/2). 
//      Let x be the X coordinate of the 2nd & 3rd points.  At t=1/2 we have a
//      point at (1,0). But the terms of the polynomial there are all equal:
//
//                (1-t)^3 = t*(1-t)^2 = t^2*(1-t) = t^3 = 1/8,
//
//      so from the Bezier formula there we have:
//
//                1 = (1/8) * (cos(a/2) + 3x + 3x + cos(a/2)), 
//
//      hence
//
//                x = (4 - cos(a/2)) / 3
//
//      The X difference between that and the 1st point is:
//      
//                DX = x - cos(a/2) = 4(1 - cos(a/2)) / 3.
//
//      But DX = distance / sin(a/2), hence the distance is
//
//                dist = (4/3)*(1 - cos(a/2)) / sin(a/2).
//
//      Rather than the angle a, we are given rDot = R^2 * cos(a), so we
//      multiply top and bottom by R:
// 
//                dist = (4/3)*(R - Rcos(a/2)) / Rsin(a/2)
// 
//      and use some trig:
//                               ________________
//                cos(a/2)   = \/(1 + cos(a)) / 2
//                               ______________________
//                R*cos(a/2) = \/(R^2 + R^2 cos(a)) / 2 
//                               ________________
//                           = \/(R^2 + rDot) / 2
//
//      Let A = (R^2 + rDot)/2.
//                               ____________________
//                R*sin(a/2) = \/R^2 - R^2 cos^2(a/2)
//                               _______
//                           = \/R^2 - A
//
//      so:
//                                          _
//                             4      R - \/A
//                      dist = - * ------------
//                             3      _______
//                                  \/R^2 - A
//
//
//      Future Consideration:  Possible perf optimization.
//      It's actually not too hard to see that dist is really a function of one
//      variable: B = rDot/rRadius^2. We convert this into a function of two
//      variables (rDot and rRadius) because it saves us a divide. However,
//      treating it as a function of 1 variable does have its advantages.
//      Treating dist as a function of B:
//
//                                             _________
//                                4      1 - \/(1 + B)/2
//                      dist(B) = - * ---------------------
//                                3         ____________
//                                        \/1 - (1+B)/4
//
//      Since our domain of interest is [0,1], and the function is well-
//      behaved on this domain, we could approximate this function via a
//      table-lookup or a spline-approximation.
//
//------------------------------------------------------------------------------
GpReal
GetBezierDistance(  // Return the distance as a fraction of the radius
    GpReal rDot,    // In: The dot product of the two radius vectors
    GpReal rRadius) // In: The radius of the arc's circle (optional=1)
{
    GpReal rRadSquared = rRadius * rRadius;  // Squared radius
    GpReal rNumerator;
    GpReal rDenominatorSquared, rDenominator;

    // Ignore NaNs
    Assert(!(rDot < -rRadSquared*.1));  // angle < 90 degrees
    Assert(!(rDot > rRadSquared*1.1));  // as dot product of 2 radius vectors

    GpReal rDist = 0.0;   // Acceptable fallback value
    
    GpReal rA = 0.5f * (rRadSquared + rDot);
    if (rA < 0.0)
    {
        // Shouldn't happen but dist=0 will work
        goto exit;
    }

    rDenominatorSquared = rRadSquared - rA;

    if (rDenominatorSquared <= 0)
        // 0 angle, we shouldn't be rounding the corner, but dist=0 is OK
        goto exit;

    rDenominator = sqrt(rDenominatorSquared);
    rNumerator = FOUR_THIRDS * (rRadius - sqrt(rA));

    if (rNumerator <= rDenominator * FUZZ)
    {
        //
        // Dist is very close to 0, so we'll snap it to 0 and save ourselves a
        // divide.
        //
        rDist = 0.0;
    }
    else
    {
        rDist = rNumerator/rDenominator;
    }

exit:
    return rDist;
}

//+-----------------------------------------------------------------------------
//  Function:
//      GetArcAngle
//
//  Synopsis:
//      Get the number of Bezier arcs, and sine & cosine of each.
//
//  Notes:
//      This is a private utility used by ArcToBezier.  We break the arc into
//      pieces so that no piece will span more than 90 degrees.  The input
//      points are on the unit circle.
//
//      Future Consideration:  Possible perf optimization.
//      There's no real reason why we have to divide the arc into equal pieces.
//      It may be more efficient to divide the arc into (up to 3) 90-degree
//      pieces and one remainder piece.
//
//+-----------------------------------------------------------------------------
void
GetArcAngle(
    __in_ecount(1) const CMilPoint2F &ptStart,
        // Start point
    __in_ecount(1) const CMilPoint2F &ptEnd,
        // End point
    BOOL fLargeArc,
        // Choose the larger of the 2 possible arcs if TRUE
    BOOL fSweepUp,
        // Sweep the arc while increasing the angle if TRUE
    __out_ecount(1) FLOAT &rCosArcAngle,
        // Cosine of a the sweep angle of one arc piece
    __out_ecount(1) FLOAT &rSinArcAngle,
        // Sine of a the sweep angle of one arc piece
    __deref_out_range(1,4) int &cPieces)      // Out: The number of pieces
{
    float rAngle;

    // The points are on the unit circle, so:
    rCosArcAngle = ptStart * ptEnd;
    rSinArcAngle = Determinant(ptStart, ptEnd);

    if (rCosArcAngle >= 0)
    {
        if (fLargeArc)
        {
            // The angle is between 270 and 360 degrees, so
            cPieces = 4;
        }
        else
        {
            // The angle is between 0 and 90 degrees, so
            cPieces = 1;
            goto Cleanup; // We already have the cosine and sine of the angle
        }
    }
    else
    {
        if (fLargeArc)
        {
            // The angle is between 180 and 270 degrees, so
            cPieces = 3;
        }
        else
        {
            // The angle is between 90 and 180 degrees, so
            cPieces = 2;
        }
    }

    //
    // We have to chop the arc into the computed number of pieces.  For
    // cPieces=2 and 4 we could have uses the half-angle trig formulas, but for
    // cPieces=3 it requires solving a cubic equation; the performance
    // difference is not worth the extra code, so we'll get the angle, divide
    // it, and get its sine and cosine.
    //

    Assert(cPieces > 0);
    rAngle = atan2f(rSinArcAngle, rCosArcAngle);
    if (fSweepUp)
    {
        if (rAngle < 0)
        {
            rAngle += static_cast<float>(TWO_PI);
        }
    }
    else
    {
        if (rAngle > 0)
        {
            rAngle -= static_cast<float>(TWO_PI);
        }
    }
    rAngle /= cPieces;
    rCosArcAngle = cosf(rAngle);
    rSinArcAngle = sinf(rAngle);

 Cleanup:;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      AcceptRadius
//
//  Synopsis:
//      Accept one radius
//
//  Notes:
//      This is a private utility used by ArcToBezier
//
//  Return:
//      false if the radius is too small compared to the chord length (returns
//      true on NaNs)
//
//+-----------------------------------------------------------------------------
bool
AcceptRadius(
    IN FLOAT rHalfChord2,
        // (1/2 chord length)squared
    IN FLOAT rFuzz2,
        // Squared fuzz
    __inout_ecount(1) FLOAT &rRadius)
        // The radius to accept (or not)
{
    // Ignore NaNs
    Assert(!(rHalfChord2 < rFuzz2));
    
    //
    // If the above assert is not true, we have no guarantee that the radius is
    // not 0, and we need to divide by the radius.
    //
    
    //
    // Accept NaN here, because otherwise we risk forgetting we encountered
    // one.
    //
    bool fAccept = !(rRadius * rRadius <= rHalfChord2 * rFuzz2);
    if (fAccept)
    {
        if (rRadius < 0)
        {
            rRadius = -rRadius;
        }
    }
    return fAccept;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ArcToBezier
//
//  Synopsis:
//      Compute the Bezier approximation of an arc
//
//  Notes:
//      This utility computes the Bezier approximation for an elliptical arc as
//      it is defined in the SVG arc spec. The ellipse from which the arc is
//      carved is axis-aligned in its own coordinates, and defined there by its
//      x and y radii. The rotation angle defines how the ellipse's axes are
//      rotated relative to our x axis. The start and end points define one of 4
//      possible arcs; the sweep and large-arc flags determine which one of
//      these arcs will be chosen. See SVG spec for details.
//
//      Returning cPieces = 0 indicates a line instead of an arc
//                cPieces = -1 indicates that the arc degenerates to a point 
//
//------------------------------------------------------------------------------
void
ArcToBezier(
    IN FLOAT        xStart,     // X coordinate of the last point
    IN FLOAT        yStart,     // Y coordinate of the last point
    IN FLOAT        xRadius,    // The ellipse's X radius
    IN FLOAT        yRadius,    // The ellipse's Y radius
    IN FLOAT        rRotation,  // Rotation angle of the ellipse's x axis
    IN BOOL         fLargeArc,  // Choose the larger of the 2 possible arcs if TRUE
    IN BOOL         fSweepUp,   // Sweep the arc while increasing the angle if TRUE
    IN FLOAT        xEnd,       // X coordinate of the last point
    IN FLOAT        yEnd,       // Y coordinate of the last point
    __out_ecount(12) MilPoint2F    *pPt,       // An array of size 12 receiving the Bezier points
    __deref_out_range(-1,4) int         &cPieces)   // The number of output Bezier curves
{
    FLOAT x, y, rHalfChord2, rCos, rSin, rCosArcAngle, rSinArcAngle,
         xCenter, yCenter, rBezDist;
    CMilPoint2F ptStart, ptEnd, vecToBez1, vecToBez2;
    MILMatrix3x2 matToEllipse;

    FLOAT rFuzz2 = FLOAT(FUZZ * FUZZ);
    bool fZeroCenter = false;
    int i, j;

    Assert(pPt);
    cPieces = -1;

    // In the following, the line segment between between the arc's start and 
    // end points is referred to as "the chord".
    
    // Transform 1: Shift the origin to the chord's midpoint
    x = 0.5f * (xEnd - xStart);
    y = 0.5f * (yEnd - yStart);

    rHalfChord2 = x * x + y * y;     // (half chord length)^2
    
    // Degenerate case: single point
    if (rHalfChord2 < rFuzz2)
    {
        // The chord degenerates to a point, the arc will be ignored
        goto Cleanup;
    }

    // Degenerate case: straight line
    if (!AcceptRadius(rHalfChord2, rFuzz2, xRadius)    ||  
        !AcceptRadius(rHalfChord2, rFuzz2, yRadius))
    {
        // We have a zero radius, add a straight line segment instead of an arc
        cPieces = 0;
        goto Cleanup;
    }
    
    // Transform 2: Rotate to the ellipse's coordinate system
    if (fabs(rRotation) < FUZZ)
    {
        // 
        // rRotation will almost always be 0 and Sin/Cos are expensive
        // functions. Let's not call them if we don't have to.
        //
        rCos = 1.0f;
        rSin = 0.0f;
    }
    else
    {
        rRotation = -rRotation * FLOAT(PI_OVER_180);

        rCos = cosf(rRotation);
        rSin = sinf(rRotation);

        float rTemp = x * rCos - y * rSin; 
        y = x * rSin + y * rCos;
        x = rTemp;
    }

    // Transform 3: Scale so that the ellipse will become a unit circle
    x /= xRadius;
    y /= yRadius;

    // We get to the center of that circle along a vector perpendicular to the chord   
    // from the origin, which is the chord's midpoint. By Pythagoras, the length of that
    // vector is sqrt(1 - (half chord)^2).

    rHalfChord2 = x * x + y * y;   // now in the circle coordinates   
    if (rHalfChord2 > 1.0f)
    {
        // The chord is longer than the circle's diameter; we scale the radii uniformly so 
        // that the chord will be a diameter. The center will then be the chord's midpoint,
        // which is now the origin.
        float rTemp = sqrtf(rHalfChord2);

        xRadius *= rTemp;
        yRadius *= rTemp;
        xCenter = yCenter = 0;
        fZeroCenter = true;

        // Adjust the unit-circle coordinates x and y
        x /= rTemp;
        y /= rTemp;
    }
    else
    {
        // The length of (-y,x) or (x,-y) is sqrt(rHalfChord2), and we want a vector
        // of length sqrt(1 - rHalfChord2), so we'll multiply it by:
        float rTemp = sqrtf((1.0f - rHalfChord2) / rHalfChord2);
        if (fLargeArc != fSweepUp)
        // Going to the center from the origin=chord-midpoint
        {
            // in the direction of (-y, x)
            xCenter = -rTemp * y;
            yCenter = rTemp * x;
        }
        else
        {
            // in the direction of (y, -x)
            xCenter = rTemp * y;
            yCenter = -rTemp * x;
        }
    }

    // Transformation 4: shift the origin to the center of the circle, which then becomes
    // the unit circle. Since the chord's midpoint is the origin, the start point is (-x, -y)
    // and the endpoint is (x, y).
    ptStart = CMilPoint2F(-x - xCenter, -y - yCenter);
    ptEnd = CMilPoint2F(x - xCenter, y - yCenter);

    // Set up the matrix that will take us back to our coordinate system.  This matrix is
    // the inverse of the combination of transformation 1 thru 4.
    matToEllipse.Set(rCos * xRadius,              -rSin * xRadius,
                     rSin * yRadius,               rCos * yRadius,
                     0.5f * (xEnd + xStart),       0.5f * (yEnd + yStart));
    if (!fZeroCenter)
    {
        // Prepend the translation that will take the origin to the circle's center
        matToEllipse.m_20 += (matToEllipse.m_00 * xCenter + matToEllipse.m_10 * yCenter);
        matToEllipse.m_21 += (matToEllipse.m_01 * xCenter + matToEllipse.m_11 * yCenter);
    }
       
    // Get the sine & cosine of the angle that will generate the arc pieces
    GetArcAngle(ptStart, ptEnd, fLargeArc, fSweepUp, rCosArcAngle, rSinArcAngle, cPieces);

    // Get the vector to the first Bezier control point
    rBezDist = FLOAT(GetBezierDistance(rCosArcAngle));
    if (!fSweepUp)
    {
        rBezDist = -rBezDist;
    }
    vecToBez1 = CMilPoint2F(-rBezDist * ptStart.Y, rBezDist * ptStart.X);

    // Add the arc pieces, except for the last
    j = 0;
    for (i = 1;   i < cPieces;  i++)
    {
        // Get the arc piece's endpoint
        CMilPoint2F ptPieceEnd(ptStart.X * rCosArcAngle - ptStart.Y * rSinArcAngle, 
                              ptStart.X * rSinArcAngle + ptStart.Y * rCosArcAngle);
        vecToBez2 = CMilPoint2F(-rBezDist * ptPieceEnd.Y, rBezDist * ptPieceEnd.X);

        matToEllipse.TransformPoint(pPt[j++], ptStart + vecToBez1);
        matToEllipse.TransformPoint(pPt[j++], ptPieceEnd - vecToBez2);
        matToEllipse.TransformPoint(pPt[j++], ptPieceEnd);

        // Move on to the next arc
        ptStart = ptPieceEnd;
        vecToBez1 = vecToBez2;
    }
    
    // Last arc - we know the endpoint
    vecToBez2 = CMilPoint2F(-rBezDist * ptEnd.Y, rBezDist * ptEnd.X);

    matToEllipse.TransformPoint(pPt[j++], ptStart + vecToBez1);
    matToEllipse.TransformPoint(pPt[j++], ptEnd - vecToBez2);

    // j is less than 3*cPieces
    __analysis_assume(j < 12);
    Assert(j < 12);
    pPt[j] = CMilPoint2F(xEnd, yEnd);

Cleanup:;
}
//////////////////////////////////////////////////////////

// Implementation of CBounds
    
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Obtain the results
//
//------------------------------------------------------------------------------
HRESULT
CBounds::SetRect(
    __out_ecount(1) CMilRectF &rect)   // Out: The bounds as a RectF_RB
{
    HRESULT hr = S_OK;

    if (m_fEncounteredNaN)
    {
        rect.left = FLOAT_QNAN;
        rect.top = FLOAT_QNAN;
        rect.right = FLOAT_QNAN;
        rect.bottom = FLOAT_QNAN;
    }
    else if (m_xMin <= m_xMax  &&  m_yMin <= m_yMax)
    {
        rect.left = TOREAL(m_xMin);
        rect.top = TOREAL(m_yMin);
        rect.right = TOREAL(m_xMax);
        rect.bottom = TOREAL(m_yMax);
    }
    else
    {
        // It's an empty rectangle
        rect.SetEmpty();
    }

    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Update with one point
//
//------------------------------------------------------------------------------
void 
CBounds::UpdateWithPoint(
    __in_ecount(1) const GpPointR & pt)
        // In: A point to update with
{
    if (pt.X < m_xMin)
        m_xMin = pt.X;

    if (pt.X > m_xMax)
        m_xMax = pt.X;

    if (pt.Y < m_yMin)
        m_yMin = pt.Y;

    if (pt.Y > m_yMax)
        m_yMax = pt.Y;

    UpdateNaN(pt);
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Update with a Bezier segment
//
//------------------------------------------------------------------------------
void 
CBounds::UpdateWithBezier(
    __in_ecount(1) const GpPointR &pt0,
        // In: First point
    __in_ecount(1) const GpPointR &pt1,
        // In: First control point
    __in_ecount(1) const GpPointR &pt2,
        // In: Second control point
    __in_ecount(1) const GpPointR &pt3)
        // In: Last point
{
    // Update with the curve's last point
    UpdateWithPoint(pt3);
        
    // The below computations can absorb Nans, so we do a check now.
    UpdateNaN(pt1);
    UpdateNaN(pt2);

    int j;
    GpReal r[2];

    // Update x bounds where the derivative of x is 0
    int nZeros = GetDerivativeZeros(pt0.X, pt1.X, pt2.X, pt3.X, r);
    for (j = 0;  j < nZeros;  j++)
    {
        GpReal x = GetBezierPolynomValue(pt0.X, pt1.X, pt2.X, pt3.X, r[j]);
        if (x < m_xMin)
        {
            m_xMin = x;
        }
        else if (x > m_xMax)
        {
            m_xMax = x;
        }

        UpdateNaN(x);
    }

    // Update y bounds where the derivative of y is 0
    nZeros = GetDerivativeZeros(pt0.Y, pt1.Y, pt2.Y, pt3.Y, r);
    for (j = 0;  j < nZeros;  j++)
    {
        GpReal y = GetBezierPolynomValue(pt0.Y, pt1.Y, pt2.Y, pt3.Y, r[j]);
        if (y < m_yMin)
        {
            m_yMin = y;
        }
        else if (y > m_yMax)
        {
            m_yMax = y;
        }

        UpdateNaN(y);
    }
}
//+-------------------------------------------------------------------------------------------------

//
//  Member:   CBounds::UpdateWithArc
//
//  Synopsis: Update the bounds with an elliptical arc
//
//  Notes:    See the header of ArcToBezier for the interpretation of the arc's defining paramters.
//
//--------------------------------------------------------------------------------------------------
void
CBounds::UpdateWithArc(
    IN FLOAT   xStart,     // X coordinate of the last point
    IN FLOAT   yStart,     // Y coordinate of the last point
    IN FLOAT   xRadius,    // The ellipse's X radius
    IN FLOAT   yRadius,    // The ellipse's Y radius
    IN FLOAT   rRotation,  // Rotation angle of the ellipse's x axis
    IN BOOL    fLargeArc,  // Choose the larger of the 2 possible arcs if TRUE
    IN BOOL    fSweepUp,   // Sweep the arc while increasing the angle if TRUE
    IN FLOAT   xEnd,       // X coordinate of the last point
    IN FLOAT   yEnd)       // Y coordinate of the last point
{    
    MilPoint2F pt[13];
    int cPieces;
    int i;

    // The array pt will be a complete representation of the bezier curves
    // that represent the arc, including the start-point.
    pt[0].X = xStart;
    pt[0].Y = yStart;
    ArcToBezier(xStart, 
                yStart, 
                xRadius, 
                yRadius, 
                rRotation, 
                fLargeArc, 
                fSweepUp, 
                xEnd, 
                yEnd, 
                &pt[1], 
                cPieces);

    //
    // If ArcToBezier was passed in any NaN values, then ArcToBezier is guaranteed to
    // returns pieces with NaNs.
    //

    if (0 == cPieces)
    {
        UpdateWithPoint(GpPointR(xEnd, yEnd));
    }
    else
    {
        // Total number of bezier points.
        cPieces *= 3;
        
        for (i = 0; i < cPieces; i += 3)
        {
            UpdateWithBezier(GpPointR(pt[i]), 
                             GpPointR(pt[i+1]), 
                             GpPointR(pt[i+2]), 
                             GpPointR(pt[i+3]));
        }
    }
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Find the real positive roots of the equation a*x^2 + 2*b*x + c
//
//      This is a quick and dirty solver; it should not be used as a general
//      purpose quadratic equation solver because:  
//        * It returns only positive roots.
//        * It assumes that a and c are not 0 or very small compared to b.
//        * It doesn't check if the roots are distinct.
//        * It may miss a double root or 2 roots that are very close
//        * It uses the high school formula, not recommended numerically
//
//------------------------------------------------------------------------------
int                 // Return the number of relevant roots
CBounds::SolveSpecialQuadratic( 
    GpReal a,      // In: Coefficient of x^2
    GpReal b,      // In: Coefficient of 2*x
    GpReal c,      // In: Constant term
    __out_ecount_part(2, return) GpReal *r)     // Out: An array of size 2 to receive the zeros
{
    int nZeros = 0;
    GpReal d = b * b - a * c;
    
    UpdateNaN(d);
/* The exact comparison below are appropriate, so don't change them! 
   
   * If d<=0 because of a computational error where is should be >0 then the 
     roots are equal or very close together, and this is essentially an
     inflection point, not a min or max.
     
   * If r[j] <=0 because of a computational error where it should be >0 then
     it is very close to 0, which corresponds to t very close to 0 or 1; but
     these are the curve's endpoints, which we examine anyay */
     
    if (d > 0)
    {
        // Use the formula: x = (-b +-sqrt(b^2 - ac))/a    
        d = sqrt(d);
        b = - b;
        r[nZeros] = (b - d) / a;
        UpdateNaN(r[nZeros]);
        if (r[nZeros] > 0)
          nZeros++;
        r[nZeros] = (b + d) / a;
        UpdateNaN(r[nZeros]);
        if (r[nZeros] > 0)
          nZeros++;
    }

    return nZeros;
} 
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the relevant zeros of the derivative of a cubic Bezier polynomial
//
//------------------------------------------------------------------------------
int                     // Return the number of relevant zeros
CBounds::GetDerivativeZeros( 
    GpReal a,           // In: Bezier coefficient of (1-t)^3
    GpReal b,           // In: Bezier coefficient of 3t(1-t)^2
    GpReal c,           // In: Bezier coefficient of 3(1-t)t^2
    GpReal d,           // In: Bezier coefficient of t^3
    __out_ecount_part(2, return) GpReal *r) // Out: An array of size 2 to receive the zeros
{
    int nZeros = 0;

    // Exact comparison is appropriate here
    if ((b - a) * (d - b) >= 0  &&  (c - a) * (d - c) >= 0)
    {
        // b and c lie between a and b.  By the convex hull property, all the 
        // values lie between a and b, which we're considering anyway as the
        // endpoints, so derivative zeros are irrelevant 
        return nZeros;
    }

    /* The derivative of 
                a(1-t)^3 + 3bt(1-t)^2 + 3c(1-t)t^2 + dt^3 
    is 
            3   ((b-a)(1-t)^2 + 2(c-b)t(1-t) + (d-c)t^2)), 
    so: */
    
    a = b - a;
    b = c - b;
    c = d - c;
    GpReal fa = fabs(a);
    GpReal fb = fabs(b);
    GpReal fc = fabs(c);
    GpReal fuzz = fb * FUZZ;

    if (fa < fuzz && fc < fuzz)
    {
        // The equation is essentially b*t(1-t)=0, and its roots
        // are approximately 0 and 1, so we are not interested                  
        return nZeros;
    }
    
    // The general case
    if (fa > fc)    // Exact comparison is appropriate
    {
        // Solve the quadratic a*s^2 + 2*b*s + c = 0, where s = (1-t)/t
        nZeros = SolveSpecialQuadratic(a, b, c, r);

        // Prefast : Somehow, Prefast gets into a state where
        // it fails to recognize that i is bounded by nZeros.
        #pragma prefast(push)
        #pragma prefast(disable: 22102)

        // Now s = (1-t)/t,  hence t = 1/(1+s)
        for (int i = 0;  i < nZeros;  i++)
        {
            r[i] = 1.0 / (1 + r[i]);
        }
        #pragma prefast(pop)
    }
    else
    {
        // Solve the quadratic c + 2*b*s + a*s^2 = 0, where s = t/(1-t)
        nZeros = SolveSpecialQuadratic(c, b, a, r);

        // Prefast : Somehow, Prefast gets into a state where
        // it fails to recognize that i is bounded by nZeros.
        #pragma prefast(push)
        #pragma prefast(disable: 22102)

        // Now s = t / (1-t), hence s = s /(1+s)
        for (int i = 0;  i < nZeros;  i++)
        {
            r[i] = r[i] / (1 + r[i]);
        }
        #pragma prefast(pop)
    }
    return nZeros;
}
//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Get the value at t of a given Bezier polynomial
//
//------------------------------------------------------------------------------
GpReal          // Return a(1-t)^3 + 3bt(1-t)^2 + 3c(1-t)t^2 + dt^3
CBounds::GetBezierPolynomValue(
    GpReal a,   // In: Coefficient of (1-t)^3
    GpReal b,   // In: Coefficient of 3t(1-t)^2
    GpReal c,   // In: Coefficient of 3(1-t)t^2
    GpReal d,   // In: Coefficient of t^3
    GpReal t)   // In: Parameter value t
{
    // Ignore NaNs
    Assert(!(-FUZZ >= t)     &&     !(t >= 1 + FUZZ));
    GpReal t2 = t * t;
    GpReal s = 1 - t;
    GpReal s2 = s * s;

    return a * s * s2  +  3 * b * t * s2  +  3 * c * t2 * s  +  d * t * t2;
}

//+-------------------------------------------------------------------------------------------------

HRESULT
CMilPoint2F::Unitize()
{
    HRESULT hr;
    REAL rLength = Norm();

    if (rLength >= FUZZ)
    {
        rLength = 1 / rLength;
        X *= rLength;
        Y *= rLength;
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    RRETURN(hr);
}


//--------------------------------------------------------------------------------------------------

                        // Implementation of CRealFunction

// CRealFunction defines a real valued function and supports solving the equation f(x)=0.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRealFunction::SolveNewtonRaphson
//
//  Synopsis:
//      Solve the equation f(x)=0 for this functionu Using the Newton-Raphson
//      algorithm
//
//  Returns:
//      true if converged and found a root
//
//  Notes:
//      Look at the class definition for instructions.
//
//------------------------------------------------------------------------------
bool
CRealFunction::SolveNewtonRaphson(
    __in double from,
        // The start of the search interval
    __in double to,
        // The end of the search interval
    __in double seed,
        // Initial guess
    __in double   delta,
        // Convergence is assumed when consecutive guesses are less than this
    __in double   epsilon,
        // Convergence is assumed when the function value is less than this
    __out_ecount(1) double &root
        // The root
    ) const
{
    bool fTopClamped = false;
    bool fBottomClamped = false;
    double f, df, rAbs = 0;
    double correction;
    int iter;

    // Ignore NaNs
    Assert(!(from > seed));
    Assert(!(seed > to));

    root = seed;

    for (iter = 1;  iter < 100;  iter++)
    {
        GetValueAndDerivative(root, f, df);
        rAbs = fabs(f);
        if (rAbs < epsilon)
        {
            break;  // We have a root
        }

        if (fabs(df) <= rAbs * FUZZ)
        {
            // Cannot divide f / df to obtain the next gess, so give up.
            break;
        }

        // Get the next guess
        correction = -f / df;
        if (fabs(correction) < delta)
        {
            break;
        }
        root += correction;

        // Clamp to the domain
        if (root < from)
        {
            root = from;
            if (fBottomClamped)
            {
                // Clamped twice in a row, no convergence
                break;
            }
            fBottomClamped = true;
        }
        else if (root > to)
        {
            root = to;
            if (fTopClamped)
            {
                // Clamped twice in a row, no convergence
                break;
            }
            fTopClamped = true;
        }
    }

    // Return false when NaN encountered.
    return rAbs < epsilon;
}
//--------------------------------------------------------------------------------------------------

                        // Implementation of CIncreasingFunction

// CRealFunction defines a real valued increasing function and supports solving the equation f(x)=0.
// Since the function is increasing, there can be only one solution.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CIncreasingFunction::SolveNewtonRaphson
//
//  Synopsis:
//      Solve the equation f(x)=0 for this functionu Using the Newton-Raphson
//      algorithm
//
//  Returns:
//      true if converged and found a root
//
//  Notes:
//      Since this is an increasing function, we can bracket the solution
//      between 2 abscissas where the function has different signs.  The Newton
//      Raphson algorithm computes a new guess as -f(previous guess) /
//      (derivative there).  If the denominator is too small to divide, or if
//      the new guess is outside the brackets then we take the new guess as the
//      midpoint between the brackets.
//
//------------------------------------------------------------------------------
bool
CIncreasingFunction::SolveNewtonRaphson(
    __in double from,
        // The start of the search interval
    __in double to,
        // The end of the search interval
    __in double seed,
        // Initial guess
    __in double   delta,
        // Convergence is assumed when consecutive guesses are less than this
    __in double   epsilon,
        // Convergence is assumed when the function value is less than this
    __out_ecount(1) double &root
        // The root
    ) const
{
    double top = to;
    double bottom = from;
    double f, df;
    double rAbs = DBL_MAX;
    int iter;

#ifdef DBG
    // The function will spin its wheels without converging if it is not increasing,
    // or if both its end-values have the same sign.
    GetValueAndDerivative(from, f, df);
    // Ignore NaNs
    Assert(!(f > 0));
    Assert(!(df < 0));

    GetValueAndDerivative(to, f, df);
    // Ignore NaNs
    Assert(!(f < 0));
    Assert(!(df < 0));
#endif

    root = seed;

    // Limit to 100 iterations to avoid an infinite loop
    for (iter = 1;  top - bottom > delta  &&  iter < 100;  iter++)
    {
        // function value and derivative at the current guess
        GetValueAndDerivative(root, f, df);
        // Ignore NaNs
        Assert(!(df < 0));    // Should be an increasing function
        rAbs = fabs(f);
        if(rAbs < epsilon)
        {
            break;  // We have a root
        }

        // Update the brackets
        if (f > 0)
        {
            top = root;
        }
        else
        {
            bottom = root;
        }

        // Compute the new guess
        if (fabs(df) <= rAbs * FUZZ)
        {
            // Can't divide, take the Bracket's midpoint
            root = (bottom + top) / 2;
        }
        else
        {
            // The Newton-Raphson guess
            root -= f / df;

            if (root < from  ||  root > to)
            {
                // The N-R guess falls outside the brackets, so take their midpoint instead
                root = (bottom + top) / 2;
            }
        }
    }

    // Return FALSE on NaN
    return rAbs < epsilon;
}





