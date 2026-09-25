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
//      Getting points on a path at a given fraction of path length
//
//  $ENDTAG
//
//  Classes:
//      CAnimationPath and supporting classes
//
//------------------------------------------------------------------------------
/*
                                        OVERVIEW

The purpose of this code is to compute a point at a given fraction of length along a path.  The path
is a sequence of segments, so upon Setup we compute the accummualtive length at each segment, and
at GetPoint time we find the segment on which we should look for the given length.  Finding the
point where the length equals the target length on a line segment is easy.  On a Bezier curve is
where we spend most of the work.

Some terminology:  The velocity of the curve is its first derivative vector.  The acceleration is
the second derivative vector.  The speed is the length of the velocity vector.  Given a target
length, we need to find the parameter t so that the length at t = target length.  Length is the
integral of speed, So at GetPointAtLength we solve the equation:

        Integral of speed at t = target length

We approximate the integral by the Gauss-Legendre quadrature (see below).  To minimize the error,
we integrate from the nearest pre-set break,  where we have cached the approximate length at Setup
time.  We solve the equation using Newton-Raphson.  Since we know that the function is increasing
(as the integral of a positive function), we use the CIncreasingFunction version.

The approximation of the integral proves to be tighter if we set breaks at points of minimum and
maximum speed.  (This is an empirical heuristic with no theoretical proof).  So at Setup time we
look for speed minimum and maximum and set breaks there if we find them.  The minimum and maximum
of speed are at zeros of the derivative of the speed.  That is why at setup time we look for
solutions of the equation:

        Derivative of (speed squared) = 0.

(We use squared speed because it is easier, no sqrt).  In this case  we don't know that the function
is increasing, so we use the standard CRealFunction version.

*/
//--------------------------------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(CAnimationPath, MILRender, "CAnimationPath");
MtDefine(CAnimationSegment, MILRender, "CAnimationSegment");
MtDefine(MilPoint2F, MILRender, "MilPoint2F");

// Gauss-LeGendre integration sample points for sample size 2.  The Gauss-LeGendre quadrature
// approximates the integral of a function f(x) from a to b by the (b-a)(sum wi*f(si)), where si
// are sample points between a and b and wi are weights. The sample points and weights for [0,1]
// are defined in the literature for various sample-sizes.  For sample-size 2 the weights are
// wi = 0.5, and the sample points si are the the following.  A larger sample can tighten the
// appriximation error, but here experimentation has shown that it doesn't make much difference.

const REAL g_rgGaussSample[2] = {0.2113248654051875f, 0.7886751345948125f};

// The minimal difference between curve-domain breaks.  If too few breaks are found then the
// interval will be artificially divided anyway, so it is better to miss a break than to get bogus
// ones.  We therefore use a very loose tolerance.  Using a tighter tolerance produces bogus breaks
// near 0 on the curve with Bezier points (0,0), (0,0), (0,0), (1,0). Tightening epsilon in
// SolveNewtonRaphson (to pin down the breaks more accurately) elimiates them, but that causes the
// solver to miss the important breaks on the curve (0,0), (1,0), (-1,0), (0,0).
// Tolerances may be adjustded if/when we switch to doubles.
const REAL g_rFuzzBreaks = .01f;

//--------------------------------------------------------------------------------------------------

                        // Implementation of CSquaredSpeedDerivative

// This is the derivative of the squared velocity of a Bezier curve


//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::CSquaredSpeedDerivative::GetValueAndDerivative
//
//  Synopsis:
//      Get the speed of the curve and the speed's derivative at a given point
//
//  Notes:
//      This method is called back from SolveNewtonRaphson when solving for
//      min/max of the squared speed.  The speed squared of the curve is
//      velocity * velocity.  Its first derivative is 2 * velocity *
//      acceleration; we work with 1/2 of that.  The derivative of that is
//      (third derivative) * velocity + acceleration * acceleration.
//
//------------------------------------------------------------------------------
void
CAnimationSegment::CSquaredSpeedDerivative::GetValueAndDerivative(
    __in double t,
        // Where on the curve
    __out_ecount(1) double &f,
        // The derivative of the distance at t
    __out_ecount(1) double &df) const
        // The derivative of f there
{
    Assert(_isnan(t) || ((0 <= t)  &&  (t <= 1)));

    CMilPoint2F vecVelocity, vecAcceleration;
    m_refCurve.Get2Derivatives(static_cast<REAL>(t), vecVelocity, vecAcceleration);

    f = vecAcceleration * vecVelocity;
    df = m_refCurve.GetThirdDerivative() * vecVelocity + vecAcceleration * vecAcceleration;
}

//--------------------------------------------------------------------------------------------------

                        // Implementation of CAnimationSegment


//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetPointAndTangentOnLine
//
//  Synopsis:
//      Get a point and a unit tangent at a given parameter on a line segment
//
//------------------------------------------------------------------------------
void
CAnimationSegment::GetPointAndTangentOnLine(
    REAL       rLength,
        // Length along the segment
    __out_ecount(1) MilPoint2F &pt,
        // The point there
    __out_ecount_opt(1) MilPoint2F *pvecTangent) const
        // The unit tangent there
{
    if (0 == rLength)
    {
        pt = m_ppt[0];
    }
    else
    {
        pt = m_ppt[0] + m_vecTangent * rLength;
    }

    if (pvecTangent)
    {
        *pvecTangent = m_vecTangent;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetSpeed
//
//  Synopsis:
//      Get the curve's speed at a given parameter
//
//  Returns:
//      The speed
//
//  Notes:
//      The speed is the magnitude of the velocity (=first derivative).
//
//------------------------------------------------------------------------------
REAL
CAnimationSegment::GetSpeed(
    IN REAL t) const  // Parameter
{
    REAL s = 1 - t;

    Assert(_isnan(t) || ((0 <= t)  &&  (t <= 1)));

    // Get the derivative vector
    CMilPoint2F vecVelocity = m_vecD1[0] * (s*s) + m_vecD1[1] * (s*t) + m_vecD1[2] * (t*t);

    // Return its magnitude
    return vecVelocity.Norm();
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetPointAndTangentOnCurve
//
//  Synopsis:
//      Get a point and a unit tangent at a given parameter on the curve
//
//------------------------------------------------------------------------------
void
CAnimationSegment::GetPointAndTangentOnCurve(
    IN REAL       rLength,
        // Length along the curve
    __out_ecount(1) MilPoint2F &pt,
        // The point there
    __out_ecount_opt(1) MilPoint2F *pvecTangent) const
        // The unit tangent there
{
    // Compute the curve parameter that corresponds to this portion of the length
    REAL t = GetParameterFromLength(rLength);

    if (t <= 0)
    {
        pt = m_ppt[0];
        if (pvecTangent)
        {
            *pvecTangent = m_vecD1[0];
        }
    }
    else if (t >= 1)
    {
        pt = m_ppt[3];
        if (pvecTangent)
        {
            *pvecTangent = m_vecD1[2];
        }
    }
    else
    {
        REAL s = 1 - t;
        REAL s2 = s * s;
        REAL t2 = t * t;

        // The point
        pt = m_ppt[0] * (s2 * s)   +   m_ppt[1] * (3 * s2 * t) +
             m_ppt[2] * (3 * s * t2) + m_ppt[3] * (t * t2);

        // Unit tangent vector if requested
        if (pvecTangent)
        {
            *pvecTangent = m_vecD1[0] * s2 + m_vecD1[1] * (s * t) + m_vecD1[2] * t2;
        }
    }

    if (pvecTangent)
    {
        // Unit tangent vector is requested
        if (SUCCEEDED((static_cast<CMilPoint2F*>(pvecTangent))->Unitize()))
        {
            // Record as the last good tangent
            m_vecTangent = *pvecTangent;
        }
        else
        {
            // Use the last good tangent
            *pvecTangent = m_vecTangent;
        }
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::Get2Derivatives
//
//  Synopsis:
//      Get the curve's first and second derivatives at a given parameter
//
//------------------------------------------------------------------------------
void
CAnimationSegment::Get2Derivatives(
    IN REAL        t,
        // Parameter
    __out_ecount(1) CMilPoint2F &vecD1,
        // The first derivative there
    __out_ecount(1) CMilPoint2F &vecD2) const
        // The second derivative there
{
    REAL s = 1 - t;

    Assert(_isnan(t) || ((0 <= t)  &&  (t <= 1)));

    // Compute the first and second derivatives
    vecD1 = m_vecD1[0] * (s * s) + m_vecD1[1] * (s * t) + m_vecD1[2] * (t * t);
    vecD2 = m_vecD2[0] * s + m_vecD2[1] * t;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetSpeedAndDerivative
//
//  Synopsis:
//      Get the curve's speed and its derivative at a given parameter
//
//  Notes:
//      The speed is the magnitude of the velocity (=first derivative).
//
//------------------------------------------------------------------------------
void
CAnimationSegment::GetSpeedAndDerivative(
    IN REAL   t,
        // Parameter
    __out_ecount(1) REAL  &speed,
        // The speed there
    __out_ecount(1) REAL  &derivative) const
        // The derivative of the speed there
{
    Assert(_isnan(t) || ((0 <= t)  &&  (t <= 1)));

    // Compute the first and second derivatives
    CMilPoint2F vecVelocity, vecAcceleration;
    Get2Derivatives(t, vecVelocity, vecAcceleration);

    // The speed is sqrt(C'*C'), where * stands for the dot product.
    speed = vecVelocity.Norm();

    // The derivative of that is C"*C' / sqrt(C'*C') = C"*C' / speed
    derivative = vecVelocity * vecAcceleration;

    if (speed > fabs(derivative) * FUZZ)
    {
        derivative /= speed;
    }
    else
    {
        derivative = 0;
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetLength
//
//  Synopsis:
//      Get the Approximate length of a portion of the curve
//
//  Returns:
//      The approximate length
//
//  Notes:
//      The length is the integral of the speed, and we approximate the integral by:
//      (b-a) * Sum(wi * f((1-si)*a + si*b)) a=from, b=to, wi are weights and si are
//      Gauss-Legendre sample points for [0,1].  Look at the the definition of
//      g_rgGaussSample at the top of this file for more theory.
//
//------------------------------------------------------------------------------
REAL
CAnimationSegment::GetLength(
    IN REAL from,     // Parameter of segment start
    IN REAL to) const // Parameter of segment end
{
    REAL integral = GetSpeed((1 - g_rgGaussSample[0]) * from + g_rgGaussSample[0] * to) +
                    GetSpeed((1 - g_rgGaussSample[1]) * from + g_rgGaussSample[1] * to);

    return integral * (to - from) * 0.5f;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetExtent
//
//  Synopsis:
//      Get a loose bound on the curve's extent
//
//  Returns:
//      max(width, height) of the curve's loose bounding box
//
//  Notes:
//      The bounding box for this purpose is that of the defining points.
//
//------------------------------------------------------------------------------
REAL
CAnimationSegment::GetExtent() const
{
    // Compute a loose bounding box
    REAL xMin = m_ppt[0].X;
    REAL xMax = xMin;
    REAL yMin = m_ppt[0].Y;
    REAL yMax = yMin;

    for (int i = 1;  i < 4;  i++)
    {
        if (m_ppt[i].X < xMin)
        {
            xMin = m_ppt[i].X;
        }
        else if (m_ppt[i].X > xMax)
        {
            xMax = m_ppt[i].X;
        }
        if (m_ppt[i].Y < yMin)
        {
            yMin = m_ppt[i].Y;
        }
        else if (m_ppt[i].Y > yMax)
        {
            yMax = m_ppt[i].Y;
        }
    }

    // Get the extent
    xMax -= xMin;
    yMax -= yMin;
    return max(xMax, yMax);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::InitAsCurve
//
//  Synopsis:
//      Initialize as a curve segment
//
//  Notes:
//      Prepare the curve segment to produce points at a given fraction of curve
//      length. We are breaking the curve's [0,1] domain to 3-4 segments,
//      preferably at min/max of curve speed, and caching the length and at
//      these breaks and reference parameter between them. When we'll get a
//      length fraction we'll locate the appropriate span (between breaks) and
//      find the parameter in [0,1]for which Length(t)=Desired length by solving
//      the equation.  For all this we'll need the curve's 3 derivatives.
//
//------------------------------------------------------------------------------
HRESULT
CAnimationSegment::InitAsCurve(
    __in_ecount(4) const CMilPoint2F *ppt,
        // Segment's defining points
    __inout_ecount(1) REAL &rLength)
        // Path length so far, updated here
{
    HRESULT hr = S_OK;

    m_rBaseLength = rLength;

    // Init fields to correspond to a valid - but uninteresting - default value, in case this method
    // returns before fully intiallizing.
    m_cBreaks = 2;
    m_rgBreak[0] = 0;
    m_rgBreak[1] = 1;
    m_rgLength[0] = 0;
    m_rgLength[1] = 0;
    m_rgMid[0] = 0;

    m_bType = MilCoreSeg::TypeBezier;
    m_ppt = ppt;

    // Bezier coefficients of the first derivative
    m_vecD1[0] = (ppt[1] - ppt[0]) * 3;
    m_vecD1[1] = (ppt[2] - ppt[1]) * 3;
    m_vecD1[2] = (ppt[3] - ppt[2]) * 3;

    // Bezier coefficients of the second derivative
    m_vecD2[0] = (m_vecD1[1] - m_vecD1[0]) * 2;
    m_vecD2[1] = (m_vecD1[2] - m_vecD1[1]) * 2;

    // The constant third derivative
    m_vecD3 = m_vecD2[1] - m_vecD2[0];

    m_vecD1[1] *= 2;  // To avoid the need to multiply by 2 when evaluating

    // Get a good tangent
    m_vecTangent = m_vecD1[0];
    if (FAILED(m_vecTangent.Unitize()))
    {
        // The first derivative is no good, try the second
        m_vecTangent = ppt[2] - ppt[1];
        if (FAILED(m_vecTangent.Unitize()))
        {
            // The second derivative is no good, try the third
            m_vecTangent = ppt[3] - ppt[1];
            if (FAILED(hr = m_vecTangent.Unitize()))
            {
                // The curve is degenerate and w'll skip it - no IFC
                goto Cleanup;
            }
        }
    }

    // Set breaks, preferably at the speed's min/max but possibly elsewhere
    SetBreaks();

    // Set lengths and references half way between breaks
    for (UINT i = 1;  i < m_cBreaks;  i++)
    {
        // this code is bound by the fact that 'i' can only be between 1 and m_cBreaks
        m_rgMid[i-1] = (m_rgBreak[i-1] + m_rgBreak[i]) / 2;
        m_rgLength[i] = m_rgLength[i-1] + GetLength(m_rgBreak[i-1], m_rgMid[i-1]) +
                                          GetLength(m_rgMid[i-1], m_rgBreak[i]) ;
    }

    // Update the path's accummulative length
    Assert(0 < m_cBreaks && m_cBreaks <= ARRAY_SIZE(m_rgLength));
    rLength += m_rgLength[m_cBreaks - 1];

Cleanup:
    return hr;  // Not RRETURN, failure is an option if the curve is degenerate
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::InitAsLine
//
//  Synopsis:
//      Initialize a line segment for computing a point at a given fraction of
//      its length.
//
//------------------------------------------------------------------------------
HRESULT
CAnimationSegment::InitAsLine(
    __in_ecount(2) const CMilPoint2F *ppt,
        // Line's points
    __inout_ecount(1) REAL &rLength)
        // Path length so far, updated here
{
    HRESULT hr = S_OK;

    m_bType = MilCoreSeg::TypeLine;
    m_cBreaks = 2;
    m_rBaseLength = rLength;
    m_rgLength[0] = 0;

    m_ppt = ppt;

    // Compute a unit direction vector
    m_vecTangent = ppt[1] - ppt[0];
    m_rgLength[1] = m_vecTangent.Norm();
    if (m_rgLength[1] < FUZZ)
    {
        // This line segment degenerates to a point, we'll just skip it - no IFC
        hr = E_FAIL;
        goto Cleanup;
    }
    else
    {
        m_vecTangent *= (1 / m_rgLength[1]);
    }

    rLength += m_rgLength[1];

Cleanup:
    return hr;  // Not RRETURN, failure is an option if the line is degenerate
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetValueAndDerivative
//
//  Synopsis:
//      Get the value and derivatives of the approximate length function
//
//  Notes:
//      This method gets the value and derivative of the function
//      Length(t)-target for the purpose of finding t such that Length(t) -
//      target = 0.  It is called back from the Newton-Raphson solver of the
//      equation Length(t) - target = 0.
//
//      The actual length is the integral of the curve speed from 0 to t.  Here
//      we use the Gauss-LeGendre quadrature to approximate the integral.  To
//      minimize the approximation error, we integrate from the nearest
//      reference point, where the integral has been pre-computed.
//
//      The derivative of the integral of the speed is the speed, but here we
//      compute the derivtive of the approximation.  This is more theoretically
//      sound as well as cheaper to compute. The integral over the interval
//      [a,t] is approximated by:
//
//                  I(t) = (t-a) * Sum(wi * f((1-si)*a + si*t)),
//
//      where wi are weights and si are sample points for the integral over
//      [0,1].  The derivative of that is, as the derivative of a product:
//
//                  dI/dt = Sum(wi * f((1-si)*a + si*t))) + (t-a) * d/dt(Sum(wi * f((1-si)*a + si*t)))).
//
//      By the chain rule, the second term equals:
//
//                  (t-a) * Sum(wi * f'((1-si)*a + si*t))*si).
//
//      f is the speed and f' is the derivative of the speed. In the code f is
//      "speed" and f' is "derivative".
//
//------------------------------------------------------------------------------
void
CAnimationSegment::GetValueAndDerivative(
    __in double t,
        // Where on the curve
    __out_ecount(1) double &f,
        // The derivative of the distance at t
    __out_ecount(1) double &df) const
        // The derivative of f there
{
    UINT reference = 0;

    Assert(_isnan(t) || ((0 <= t)  &&  (t <= 1)));

    // Choose the nearest reference point
    if (t > m_rgMid[m_uiCurrentSpan])
    {
        reference = m_uiCurrentSpan + 1;
    }
    else
    {
        reference = m_uiCurrentSpan;
    }

    // Compute the approximate integral and its derivative
    f = df = 0;

    for (UINT i = 0;  i < 2;  i++)
    {
        REAL speed, derivative;
        REAL tF = static_cast<REAL>(t);
        REAL r = (1 - g_rgGaussSample[i]) * m_rgBreak[reference] + g_rgGaussSample[i] * tF;
        GetSpeedAndDerivative(r, speed, derivative);
        derivative *= g_rgGaussSample[i];

        // Add with the appropriate weight
        f += speed;
        df += derivative;
    }

    // Multiply by the Gauss weights.  Note that this is possible on for sample-size < 3.  For
    // a larger sample size there will be different weights, and they have factor the entries
    // inside the sum.
    f  *= .5;
    df *= .5;

    t -= m_rgBreak[reference];  // Now = (t - a)
    df = t * df + f;
    f = m_rgLength[reference] + t * f - m_rTargetLength;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetParameterFromLength
//
//  Synopsis:
//      Get the parameter on the curve that corresponds to given length
//
//  Returns:
//      The parameter where CurveLength(t) = rLength.
//
//  Notes:
//      A side effect is caching the latest paramete as initial guess for next
//      time.
//
//------------------------------------------------------------------------------
REAL
CAnimationSegment::GetParameterFromLength(
    IN REAL rLength) const   // The length on this curve
{
    double rLatest;

    // Restrict to [0, 1]
    if (rLength <= 0)
    {
        m_rLatest = 0;
    }
    else if (rLength >= m_rgLength[m_cBreaks - 1])
    {
        m_rLatest = 1;
    }
    else
    {
        // This caps m_uiCurrentSpan to less than 4 so that the addition of 1 does not cause a wrong location lookup
        // Find the nearest reference point
        m_rTargetLength = rLength;
        while (m_rTargetLength < m_rgLength[m_uiCurrentSpan])
            m_uiCurrentSpan--;
        // if m_uiCurrentSpan == 3, then m_rgLength[m_uiCurrentSpan + 1] >= m_rTargetLength.
        while (m_uiCurrentSpan < 3 && m_rTargetLength > m_rgLength[m_uiCurrentSpan + 1])
            m_uiCurrentSpan++;

        // Assert that the second condition is false to confirm the fact that m_uiCurrentSpan < 3
        // isn't strictly required as a termination condition.  We maintain it in any case to be
        // doubly sure that the array index ahead will be safe.
        Assert(m_rTargetLength <= m_rgLength[m_uiCurrentSpan + 1]);

        // Clip the initial guess to the current span
        if (m_rLatest < m_rgBreak[m_uiCurrentSpan])
        {
            m_rLatest = m_rgBreak[m_uiCurrentSpan];
        }
        else if (m_rLatest > m_rgBreak[m_uiCurrentSpan + 1])
        {
            m_rLatest = m_rgBreak[m_uiCurrentSpan + 1];
        }

        // Solve the equation length(t) - target length = 0
        SolveNewtonRaphson(
            m_rgBreak[m_uiCurrentSpan],
            m_rgBreak[m_uiCurrentSpan + 1],
            m_rLatest,
            FUZZ,
            FUZZ * m_rgLength[m_cBreaks-1],
            OUT rLatest);

        m_rLatest = static_cast<REAL>(rLatest);
    }

    return m_rLatest;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::AcceptBreak
//
//  Synopsis:
//      Accept domain break if it is within the domain and if it doesn't
//      duplicate an existing one
//
//------------------------------------------------------------------------------
void
CAnimationSegment::AcceptBreak(
    IN double t)   // The candidate for a break
{
    if (t > g_rFuzzBreaks  &&  t < 1 - g_rFuzzBreaks  &&  m_cBreaks < 5)
    {
        // find the position of t in the list
        UINT i = 0;
        do i++; while (i < m_cBreaks  &&  t > m_rgBreak[i]);

        // Check if t doesn't duplicate an existing entry
        if ( ((0 == i)      ||  t > m_rgBreak[i-1] + g_rFuzzBreaks) &&
            ((m_cBreaks == i)  ||  t < m_rgBreak[i] - g_rFuzzBreaks))
        {
            // Insert t in the list
            UINT k = m_cBreaks - i;
            if (k > 0)
            {
                memmove(m_rgBreak + i + 1, m_rgBreak + i, k * sizeof(REAL));
            }
            m_rgBreak[i] = static_cast<REAL>(t);
            m_cBreaks++;
        }
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::SetBreaks
//
//  Synopsis:
//      Set the break points in the curve domain
//
//  Notes:
//      The first and last breaks are the curve's start and end. If the curve
//      speed has minima and maxima then breaks there seem to facilitate a
//      better approximation of the length integral.  If not enough of those
//      exist then we insert arbitrary ones.
//
//------------------------------------------------------------------------------
void
CAnimationSegment::SetBreaks()
{
    double t;
    const REAL delta = REAL(FUZZ) * 10;

    // The function we're working with is the derivative of the speed squared.
    // Its order of magnitude is as curve extents squared.
    REAL epsilon = GetExtent();
    epsilon = epsilon * epsilon * delta;

    // Domain start and end are the first & last breaks
    m_rgBreak[0] = 0;
    m_rgBreak[1] = 1;
    m_cBreaks = 2;

    // Define the speed derivative and find its zeros
    CSquaredSpeedDerivative CSquaredSpeedDerivative(*this);

    // A time-tested heuristic: Newton-Raphson with 3 different seeds.  It would be
    // nice to choose seeds based on some theory.

    // Solve with initial guess 0
    if (CSquaredSpeedDerivative.SolveNewtonRaphson(0, 1, 0, delta, epsilon, t))
    {
        AcceptBreak(t);
    }

    // Solve with initial guess .5
    if (CSquaredSpeedDerivative.SolveNewtonRaphson(0, 1, 0.5, delta, epsilon, t))
    {
        AcceptBreak(t);
    }

    // Solve with initial guess 1
    if (CSquaredSpeedDerivative.SolveNewtonRaphson(0, 1, 1, delta, epsilon, t))
    {
        AcceptBreak(t);
    }

    // Insert additional interior breaks if there are too few speed extrema
    if (2 == m_cBreaks)  // no interior breaks
    {
        // insert 3 additional breaks
        m_rgBreak[4] = m_rgBreak[1];
        m_rgBreak[2] = (m_rgBreak[0] + m_rgBreak[4]) / 2;
        m_rgBreak[1] = (m_rgBreak[0] + m_rgBreak[2]) / 2;
        m_rgBreak[3] = (m_rgBreak[2] + m_rgBreak[4]) / 2;
        m_cBreaks = 5;
    }
    else if (3 == m_cBreaks) // one interior break
    {
        // insert 2 additional breaks
        m_rgBreak[4] = m_rgBreak[2];
        m_rgBreak[2] = m_rgBreak[1];
        m_rgBreak[1] = (m_rgBreak[0] + m_rgBreak[2]) / 2;
        m_rgBreak[3] = (m_rgBreak[2] + m_rgBreak[4]) / 2;
        m_cBreaks = 5;
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationSegment::GetPointAtLength
//
//  Synopsis:
//      Get the point on the curve at a given length on the path
//
//  Returns:
//      If pvecTangent is not null then a tangent vector will be returned as
//      well
//
//------------------------------------------------------------------------------
void
CAnimationSegment::GetPointAtLength(
    IN REAL        rLength,
        // Fraction of length (between 0 and 1 (or NaN))
    __out_ecount(1) MilPoint2F  &pt,
        // Point on the segment there
    __out_ecount_opt(1) MilPoint2F  *pvecTangent)
        // Unit tangent there (NULL OK)
{
    // Make the length relative to this segment
    rLength -= m_rBaseLength;
    Assert(_isnan(rLength) || rLength >= 0);
    if (rLength > m_rgLength[m_cBreaks - 1])
    {
        rLength = m_rgLength[m_cBreaks - 1];
    }

    if (MilCoreSeg::TypeLine == m_bType)
    {
        GetPointAndTangentOnLine(rLength, pt, pvecTangent);
    }
    else
    {
        GetPointAndTangentOnCurve(rLength, pt, pvecTangent);
    }
}
//--------------------------------------------------------------------------------------------------

                        // Implementation of CAnimationPath

// Instruments a path for animating along it

//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationPath::SetUp
//
//  Synopsis:
//      Set up for animation
//
//------------------------------------------------------------------------------
HRESULT
CAnimationPath::SetUp(
    __in_ecount(1) const IShapeData &shape)
{
    UINT i;
    HRESULT hr = S_OK;
    UINT cPoints = 0;
    UINT cSegments = 0;

    m_rTotalLength = 0;

    // Get an estimate on the number of segments and points we need to allocate
    for (i = 0;  i < shape.GetFigureCount();  i++)
    {
        UINT u, v;
        IFC(shape.GetFigure(i).GetCountsEstimate(u, v));
        IFC(AddUINT(cSegments, u, OUT cSegments));
        IFC(AddUINT(cPoints, v, cPoints));
    }

    // Allocate animation segments and points
    IFCOOM(m_pSegments = new CAnimationSegment[cSegments]);

    {
        MtSetDefault(Mt(MilPoint2F));
        IFCOOM(m_pPoints = new MilPoint2F[cPoints]);
    }

    // Traverse the path and set up the segments
    m_uCurrentPoint = m_cSegments = 0;
    for (i = 0;  i < shape.GetFigureCount();  i++)
    {
        // capped by the value if i and m_uCurrentPoint is initialized to 0
        m_pPoints[m_uCurrentPoint++] = shape.GetFigure(i).GetStartPoint();
        IFC(TraverseForward(shape.GetFigure(i)));
    }

    // Weeding out degenerate segments may have left us with no segments
    if (m_cSegments < 1)
    {
        IFC(E_INVALIDARG);
    }

    m_uCurrentSegment = m_uCurrentPoint = 0;

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationPath::DoLine
//
//  Synopsis:
//      Set up a line animation segment
//
//  Notes:
//      Callback from TraverseForward
//
//------------------------------------------------------------------------------
HRESULT
CAnimationPath::DoLine(
    __in_ecount(1) const MilPoint2F &ptEnd)     // The line's end point
{
    const CMilPoint2F * pptStart = static_cast<CMilPoint2F*>(m_pPoints + m_uCurrentPoint - 1);

    Assert(m_pSegments);
    Assert(m_pPoints);

    m_pPoints[m_uCurrentPoint] = ptEnd;
    if (SUCCEEDED(m_pSegments[m_cSegments].InitAsLine(pptStart, m_rTotalLength)))
    {
        m_cSegments++;
        m_uCurrentPoint++;
    }

    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationPath::DoBezier
//
//  Synopsis:
//      Set up a curve animation segment
//
//  Notes:
//      Callback from TraverseForward
//
//------------------------------------------------------------------------------
HRESULT
CAnimationPath::DoBezier(
    __in_ecount(3) const MilPoint2F *ppt)     // The Second Bezier point
{
    const CMilPoint2F * pptStart = static_cast<CMilPoint2F*>(m_pPoints + m_uCurrentPoint - 1);

    Assert(m_pSegments);
    Assert(m_pPoints);

    memcpy(m_pPoints + m_uCurrentPoint, ppt, 3 * sizeof(MilPoint2F));
    if (SUCCEEDED(m_pSegments[m_cSegments].InitAsCurve(pptStart, m_rTotalLength)))
    {
        m_cSegments++;
        m_uCurrentPoint += 3;
    }

    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationPath::BinarySearch
//
//  Synopsis:
//      Find an index i so that GetLength(i) <= rLength <= GetLength(i+1)
//
//  Notes:
//      The search result is recorded as m_uCurrentSegment
//
//------------------------------------------------------------------------------
void
CAnimationPath::BinarySearch(
    IN REAL         rLength,        // The length to locate
    IN int          bottom,         // The bottom of the search interval
    IN int          top)            // The top of the search interval
{
    // If the following are not true then we should have not been called
    Assert(0 <= bottom);
    Assert(bottom <= top);
    Assert(static_cast<UINT>(top) <= m_cSegments);

    // Ignore NaNs
    Assert(!(GetLength(bottom) > rLength));
    Assert(!(GetLength(top) < rLength));

    while (bottom < top - 1)
    {
        int mid = (bottom + top) / 2;
        if (GetLength(mid) < rLength)
        {
            bottom = mid;
        }
        else
        {
            top = mid;
        }
    }

    m_uCurrentSegment = bottom;

    Assert(m_uCurrentSegment < m_cSegments);

    // Ignore NaNs
    Assert(!(rLength > GetLength(m_uCurrentSegment + 1)));
    Assert(!(GetLength(m_uCurrentSegment) > rLength));
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CAnimationPath::GetPointAtLengthFraction
//
//  Synopsis:
//      Get the point on the curve at a given fraction of length
//
//  Notes:
//      If pvecTangent is not null then a tangent vector will be returned as
//      well
//
//------------------------------------------------------------------------------
void
CAnimationPath::GetPointAtLengthFraction(
    IN REAL        rFraction,
        // Fraction of length (between 0 and 1)
    __out_ecount(1) MilPoint2F  &pt,
        // Point on the segment there
    __out_ecount_opt(1) MilPoint2F  *pvecTangent)
        // Unit tangent there (NULL OK)
{
    Assert(m_pSegments);        // Should have been allocated at init time
    Assert(m_cSegments > 0);    // Should have quit otherwise

    // Convert the fraction to length and find the segment that contains it
    if (rFraction <= 0)
    {
        rFraction = 0;
        m_uCurrentSegment = 0;
    }
    else if (rFraction >= 1)
    {
        rFraction = m_rTotalLength;
        m_uCurrentSegment = m_cSegments - 1;
    }
    else
    {
        // Convert to actual length
        rFraction *= m_rTotalLength;

        // Best guess: current segment, but are we still there?
        if (rFraction > GetLength(m_uCurrentSegment + 1))
        {
            // Try next segment up
            m_uCurrentSegment++;
            Assert(m_uCurrentSegment < m_cSegments);
            if (rFraction > GetLength(m_uCurrentSegment + 1))
            {
                // Perhaps we are starting over at the end
                {
                    if (rFraction > GetLength(m_cSegments - 1))
                    {
                        m_uCurrentSegment = m_cSegments - 1;
                    }
                    else
                    {
                        // Nope, perform a binary search
                        BinarySearch(rFraction, m_uCurrentSegment + 1, m_cSegments);
                    }
                }
            }
        }
        else if (rFraction < GetLength(m_uCurrentSegment))
        {
            // Try next segment down
            m_uCurrentSegment--;
            if (rFraction < GetLength(m_uCurrentSegment))
            {
                // Perhaps we are starting over at the beginning
                {
                    if (rFraction < GetLength(1))
                    {
                        m_uCurrentSegment = 0;
                    }
                    else
                    {
                        // Nope, perform a binary search
                        BinarySearch(rFraction, 0, m_uCurrentSegment);
                    }
                }
            }
        }
        // else m_uCurrentSegment is in the correct segment
    }

    // Get the point at the relative length on that segment
    m_pSegments[m_uCurrentSegment].GetPointAtLength(rFraction, pt, pvecTangent);
}

