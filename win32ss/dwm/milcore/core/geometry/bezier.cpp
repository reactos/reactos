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
//      Class for flattening a bezier.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

// BEZIER_FLATTEN_GDI_COMPATIBLE:
//
// Don't turn on this switch without testing carefully. It's more for
// documentation's sake - to show the values that GDI used - for an error
// tolerance of 2/3.

// It turns out that 2/3 produces very noticable artifacts on antialiased lines,
// so we want to use 1/4 instead.

#ifdef BEZIER_FLATTEN_GDI_COMPATIBLE

// Flatten to an error of 2/3.  During initial phase, use 18.14 format.

#define TEST_MAGNITUDE_INITIAL    (6 * 0x00002aa0L)

// Error of 2/3.  During normal phase, use 15.17 format.

#define TEST_MAGNITUDE_NORMAL     (TEST_MAGNITUDE_INITIAL << 3)

#else

// Flatten to an error of 1/4.  During initial phase, use 18.14 format.

#define TEST_MAGNITUDE_INITIAL    (6 * 0x00001000L)

// Error of 1/4.  During normal phase, use 15.17 format.

#define TEST_MAGNITUDE_NORMAL     (TEST_MAGNITUDE_INITIAL << 3)

#endif

// I have modified the constants for HFD32 as part of fixing accuracy errors
// (Bug 816015).  Something similar could be done for the 64 bit hfd, but it ain't
// broke so I'd rather not fix it.

// The shift to the steady state 15.17 format
const LONG HFD32_SHIFT = HFD32_INITIAL_SHIFT + HFD32_ADDITIONAL_SHIFT;

// Added to output numbers before rounding back to original representation
const LONG HFD32_ROUND = 1L << (HFD32_SHIFT - 1);

// The error is tested on max(|e2|, |e3|), which represent 6 times the actual error.
// The flattening tolerance is hard coded to 1/4 in the original geometry space,
// which translates to 4 in 28.4 format.  So 6 times that is:

const LONGLONG HFD32_TOLERANCE = 24L;

// During the initial phase, while working in 18.14 format
const LONGLONG HFD32_INITIAL_TEST_MAGNITUDE = HFD32_TOLERANCE << HFD32_INITIAL_SHIFT; 

// During the steady state, while working in 15.17 format
const LONGLONG HFD32_TEST_MAGNITUDE = HFD32_INITIAL_TEST_MAGNITUDE << HFD32_ADDITIONAL_SHIFT; 

// We will stop halving the segment with basis e1, e2, e3, e4 when max(|e2|, |e3|)
// is less than HFD32_TOLERANCE.  The operation e2 = (e2 + e3) >> 3 in vHalveStepSize() may
// eat up 3 bits of accuracy. HfdBasis32 starts off with a pad of HFD32_SHIFT zeros, so
// we can stay exact up to HFD32_SHIFT/3 subdivisions.  Since every subdivision is guaranteed
// to shift max(|e2|, |e3|) at least by 2, we will subdivide no more than n times if the 
// initial max(|e2|, |e3|) is less than than HFD32_TOLERANCE << 2n.  But if the initial 
// max(|e2|, |e3|) is greater than HFD32_TOLERANCE >> (HFD32_SHIFT / 3) then we may not be
// able to flatten with the 32 bit hfd, so we need to resort to the 64 bit hfd. 

const INT HFD32_MAX_ERROR = HFD32_TOLERANCE << ((2 * HFD32_INITIAL_SHIFT) / 3);

// The maximum size of coefficients that can be handled by HfdBasis32.
const LONGLONG HFD32_MAX_SIZE = 0xffffc000;

// Michka 9/12/03: I found this number in the the body of the code witout any explanation.
// My analysis suggests that we could get away with larger numbers, but if I'm wrong we 
// could be in big trouble, so let us stay conservative.
//
// In bInit() we subtract Min(Bezier coeffients) from the original coefficients, so after
// that 0 <= coefficients <= Bound, and the test will be Bound < HFD32_MAX_SIZE. When 
// switching to the HFD basis in bInit():
//   * e0 is the first Bezier coeffient, so abs(e0) <= Bound.
//   * e1 is a difference of non-negative coefficients so abs(e1) <= Bound. 
//   * e2 and e3 can be written as 12*(p - (q + r)/2) where p,q and r are coefficients.
//     0 <=(q + r)/2 <= Bound, so abs(p - (q + r)/2) <= 2*Bound, hence 
//     abs(e2), abs(e3) <= 12*Bound.
//
// During vLazyHalveStepSize we add e2 + e3, resulting in absolute value <= 24*Bound.
// Initially HfdBasis32 shifts the numbers by HFD32_INITIAL_SHIFT, so we need to handle 
// 24*bounds*(2^HFD32_SHIFT), and that needs to be less than 2^31. So the bounds need to
// be less than 2^(31-HFD32_INITIAL_SHIFT)/24).
//
// For speed, the algorithm uses & rather than < for comparison.  To facilitate that we 
// replace 24 by 32=2^5, and then the binary representation of the number is of the form
// 0...010...0 with HFD32_SHIFT+5 trailing zeros.  By subtracting that from 2^32 = 0xffffffff+1 
// we get a number that is 1..110...0 with the same number of trailing zeros, and that can be 
// used with an & for comparison.  So the number should be:
//
//      0xffffffffL - (1L << (31 - HFD32_INITIAL_SHIFT - 5)) + 1 = (1L << 16) + 1 = 0xffff0000
//
// For the current values of HFD32_INITIAL_SHIFT=10 and HFD32_ADDITIONAL_SHIFT=3, the steady
// state doesn't pose additional requirements, as shown below. 
//
// For some reason the current code uses 0xfffc0000 = (1L << 14) + 1.
//
// Here is why the steady state doesn't pose additional requirements:
//
// In vSteadyState we multiply e0 and e1 by 8, so the requirement is Bounds*2^13 < 2^31,
// or Bounds < 2^18, less stringent than the above.
//
// In vLazyHalveStepSize we cut the error down by subdivision, making abs(e2) and abs(e3) 
// less than HFD32_TEST_MAGNITUDE = 24*2^13,  well below 2^31.
//
// During all the steady-state operations - vTakeStep, vHalveStepSize and vDoubleStepSize, 
// e0 is on the curve and e1 is a difference of 2 points on the curve, so
// abs(e0), abs(e1) < Bounds * 2^13, which requires Bound < 2^(31-13) = 2^18.  e2 and e3
// are errors, kept below 6*HFD32_TEST_MAGNITUDE = 216*2^13.  Details:
//
// In vTakeStep e2 = 2e2 - e3 keeps abs(e2) < 3*HFD32_TEST_MAGNITUDE =  72*2^13,  
// well below 2^31
//
// In vHalveStepSize we add e2 + e3 when their absolute is < 3*HFD32_TEST_MAGNITUDE (because
// this comes after a step), so that keeps the result below 6*HFD32_TEST_MAGNITUDE = 216*2^13.
// 
// In vDoubleStepSize we know that abs(e2), abs(e3) < HFD32_TEST_MAGNITUDE/4, otherwise we
// would not have doubled the step.


typedef struct _BEZIERCONTROLS {
    POINT ptfx[4];
} BEZIERCONTROLS;

inline VOID vBoundBox(
    __in_ecount(4) const POINT* aptfx,
    __out_ecount(1) RECT* prcfx)
{
    INT i;

    LONG left = aptfx[0].x;
    LONG right = aptfx[0].x;
    LONG top = aptfx[0].y;
    LONG bottom = aptfx[0].y;

    for (i = 1; i < 4; i++)
    {
        left = min(left, aptfx[i].x);
        top = min(top, aptfx[i].y);
        right = max(right, aptfx[i].x);
        bottom = max(bottom, aptfx[i].y);
    }

    // We make the bounds one pixel loose for the nominal width 
    // stroke case, which increases the bounds by half a pixel 
    // in every dimension:

    prcfx->left = left - 16;
    prcfx->top = top - 16;
    prcfx->right = right + 16;
    prcfx->bottom = bottom + 16;
}


BOOL bIntersect(
    __in_ecount(1) const RECT *a,
    __in_ecount(1) const RECT *b)
{
    return((a->left < b->right) &&
           (a->top < b->bottom) &&
           (a->right > b->left) &&
           (a->bottom > b->top));
}

BOOL Bezier32::bInit(
    __in_ecount(4) const POINT* aptfxBez,
        // Pointer to 4 control points
    __in_ecount_opt(1) const RECT* prcfxClip)
        // Bound box of visible region (optional)
{
    POINT aptfx[4];
    LONG cShift = 0;    // Keeps track of 'lazy' shifts

    cSteps = 1;         // Number of steps to do before reach end of curve

    vBoundBox(aptfxBez, &rcfxBound);

    *((BEZIERCONTROLS*) aptfx) = *((BEZIERCONTROLS*) aptfxBez);

    {
        register INT fxOr;
        register INT fxOffset;

        fxOffset = rcfxBound.left;
        fxOr  = (aptfx[0].x -= fxOffset);
        fxOr |= (aptfx[1].x -= fxOffset);
        fxOr |= (aptfx[2].x -= fxOffset);
        fxOr |= (aptfx[3].x -= fxOffset);

        fxOffset = rcfxBound.top;
        fxOr |= (aptfx[0].y -= fxOffset);
        fxOr |= (aptfx[1].y -= fxOffset);
        fxOr |= (aptfx[2].y -= fxOffset);
        fxOr |= (aptfx[3].y -= fxOffset);

    // This 32 bit cracker can only handle points in a 10 bit space:

        if ((fxOr & HFD32_MAX_SIZE) != 0)
            return(FALSE);
    }

    if (!x.bInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x))
    {
        return FALSE;
    }
    if (!y.bInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y))
    {
        return FALSE;
    }

    if (prcfxClip == (RECT*) NULL || bIntersect(&rcfxBound, prcfxClip))
    {
        for ( ; ; )
        {
            register LONG lTestMagnitude = HFD32_INITIAL_TEST_MAGNITUDE << cShift;

            if (x.lError() <= lTestMagnitude && y.lError() <= lTestMagnitude)
                break;

            cShift += 2;
            x.vLazyHalveStepSize(cShift);
            y.vLazyHalveStepSize(cShift);
            cSteps <<= 1;
        }
    }

    x.vSteadyState(cShift);
    y.vSteadyState(cShift);

// Note that this handles the case where the initial error for
// the Bezier is already less than HFD32_TEST_MAGNITUDE:

    x.vTakeStep();
    y.vTakeStep();
    cSteps--;

    return(TRUE);
}

INT Bezier32::cFlatten(
    __out_ecount_part(cptfx, return) POINT* pptfx,
    INT cptfx,
    __out_ecount(1) BOOL *pbMore)
{
    Assert(cptfx > 0);

    INT cptfxOriginal = cptfx;

    do {
    // Return current point:
    
        pptfx->x = x.fxValue() + rcfxBound.left;
        pptfx->y = y.fxValue() + rcfxBound.top;
        pptfx++;
    
    // If cSteps == 0, that was the end point in the curve!
    
        if (cSteps == 0)
        {
            *pbMore = FALSE;

            // '+1' because we haven't decremented 'cptfx' yet:

            return(cptfxOriginal - cptfx + 1);
        }
    
    // Okay, we have to step:
    
        if (max(x.lError(), y.lError()) > HFD32_TEST_MAGNITUDE)
        {
            x.vHalveStepSize();
            y.vHalveStepSize();
            cSteps <<= 1;
        }
    
        // We are here after vTakeStep.  Before that the error max(|e2|,|e3|) was less
        // than HFD32_TEST_MAGNITUDE.  vTakeStep changed e2 to 2e2-e3. Since 
        // |2e2-e3| < max(|e2|,|e3|) << 2 and vHalveStepSize is guaranteed to reduce 
        // max(|e2|,|e3|) by >> 2, no more than one subdivision should be required to 
        // bring the new max(|e2|,|e3|) back to within HFD32_TEST_MAGNITUDE, so:
        Assert(max(x.lError(), y.lError()) <= HFD32_TEST_MAGNITUDE);
    
        while (!(cSteps & 1) &&
               x.lParentErrorDividedBy4() <= (HFD32_TEST_MAGNITUDE >> 2) &&
               y.lParentErrorDividedBy4() <= (HFD32_TEST_MAGNITUDE >> 2))
        {
            x.vDoubleStepSize();
            y.vDoubleStepSize();
            cSteps >>= 1;
        }
    
        cSteps--;
        x.vTakeStep();
        y.vTakeStep();

    } while (--cptfx != 0);

    *pbMore = TRUE;
    return(cptfxOriginal);
}


///////////////////////////////////////////////////////////////////////////
// Bezier64
//
// All math is done using 64 bit fixed numbers in a 36.28 format.
//
// All drawing is done in a 31 bit space, then a 31 bit window offset
// is applied.  In the initial transform where we change to the HFD
// basis, e2 and e3 require the most bits precision: e2 = 6(p2 - 2p3 + p4).
// This requires an additional 4 bits precision -- hence we require 36 bits
// for the integer part, and the remaining 28 bits is given to the fraction.
//
// In rendering a Bezier, every 'subdivide' requires an extra 3 bits of
// fractional precision.  In order to be reversible, we can allow no
// error to creep in.  Since a INT coordinate is 32 bits, and we
// require an additional 4 bits as mentioned above, that leaves us
// 28 bits fractional precision -- meaning we can do a maximum of
// 9 subdivides.  Now, the maximum absolute error of a Bezier curve in 27
// bit integer space is 2^29 - 1.  But 9 subdivides reduces the error by a
// guaranteed factor of 2^18, meaning we can subdivide down only to an error
// of 2^11 before we overflow, when in fact we want to reduce error to less
// than 1.
//
// So what we do is HFD until we hit an error less than 2^11, reverse our
// basis transform to get the four control points of this smaller curve
// (rounding in the process to 32 bits), then invoke another copy of HFD
// on the reduced Bezier curve.  We again have enough precision, but since
// its starting error is less than 2^11, we can reduce error to 2^-7 before
// overflowing!  We'll start a low HFD after every step of the high HFD.
////////////////////////////////////////////////////////////////////////////

inline VOID HfdBasis64::vParentError(__out_ecount(1) LONGLONG* peq) const
{
    *peq = max(llabs(e3 << 2), llabs((e2 << 3) - (e3 << 2)));
}

inline VOID HfdBasis64::vError(__out_ecount(1) LONGLONG* peq) const
{
    *peq = max(llabs(e2), llabs(e3));
}

inline INT HfdBasis64::fxValue() const
{
// Convert from 36.28 and round:

    LONGLONG eq = e0;
    eq += (1L << (BEZIER64_FRACTION - 1));
    eq >>= BEZIER64_FRACTION;
    return((INT) (LONG) eq);
}

VOID HfdBasis64::vInit(INT p1, INT p2, INT p3, INT p4)
{
    LONGLONG eqTmp;
    LONGLONG eqP2 = (LONGLONG) p2;
    LONGLONG eqP3 = (LONGLONG) p3;

// e0 = p1
// e1 = p4 - p1
// e2 = 6(p2 - 2p3 + p4)
// e3 = 6(p1 - 2p2 + p3)

// Change basis:

    e0 = p1;                                        // e0 = p1
    e1 = p4;
    e2 = eqP2; e2 -= eqP3; e2 -= eqP3; e2 += e1;    // e2 = p2 - 2*p3 + p4
    e3 = e0;   e3 -= eqP2; e3 -= eqP2; e3 += eqP3;  // e3 = p1 - 2*p2 + p3
    e1 -= e0;                                       // e1 = p4 - p1

// Convert to 36.28 format and multiply e2 and e3 by six:

    e0 <<= BEZIER64_FRACTION;
    e1 <<= BEZIER64_FRACTION;
    eqTmp = e2; e2 += eqTmp; e2 += eqTmp; e2 <<= (BEZIER64_FRACTION + 1);
    eqTmp = e3; e3 += eqTmp; e3 += eqTmp; e3 <<= (BEZIER64_FRACTION + 1);
}

VOID HfdBasis64::vUntransform(
    __out_ecount(7) LONG* afx) const
{
// Declare some temps to hold our operations, since we can't modify e0..e3.

    LONGLONG eqP0;
    LONGLONG eqP1;
    LONGLONG eqP2;
    LONGLONG eqP3;

// p0 = e0
// p1 = e0 + (6e1 - e2 - 2e3)/18
// p2 = e0 + (12e1 - 2e2 - e3)/18
// p3 = e0 + e1

    eqP0 = e0;

// NOTE PERF: Convert this to a multiply by 6: [andrewgo]

    eqP2 = e1;
    eqP2 += e1;
    eqP2 += e1;
    eqP1 = eqP2;
    eqP1 += eqP2;           // 6e1
    eqP1 -= e2;             // 6e1 - e2
    eqP2 = eqP1;
    eqP2 += eqP1;           // 12e1 - 2e2
    eqP2 -= e3;             // 12e1 - 2e2 - e3
    eqP1 -= e3;
    eqP1 -= e3;             // 6e1 - e2 - 2e3

// NOTE: May just want to approximate these divides! [andrewgo]
// Or can do a 64 bit divide by 32 bit to get 32 bits right here.

    eqP1 /= 18;
    eqP2 /= 18;
    eqP1 += e0;
    eqP2 += e0;

    eqP3 = e0;
    eqP3 += e1;

// Convert from 36.28 format with rounding:

    eqP0 += (1L << (BEZIER64_FRACTION - 1)); eqP0 >>= BEZIER64_FRACTION; afx[0] = (LONG) eqP0;
    eqP1 += (1L << (BEZIER64_FRACTION - 1)); eqP1 >>= BEZIER64_FRACTION; afx[2] = (LONG) eqP1;
    eqP2 += (1L << (BEZIER64_FRACTION - 1)); eqP2 >>= BEZIER64_FRACTION; afx[4] = (LONG) eqP2;
    eqP3 += (1L << (BEZIER64_FRACTION - 1)); eqP3 >>= BEZIER64_FRACTION; afx[6] = (LONG) eqP3;
}

VOID HfdBasis64::vHalveStepSize()
{
// e2 = (e2 + e3) >> 3
// e1 = (e1 - e2) >> 1
// e3 >>= 2

    e2 += e3; e2 >>= 3;
    e1 -= e2; e1 >>= 1;
    e3 >>= 2;
}

VOID HfdBasis64::vDoubleStepSize()
{
// e1 = 2e1 + e2
// e3 = 4e3;
// e2 = 8e2 - e3

    e1 <<= 1; e1 += e2;
    e3 <<= 2;
    e2 <<= 3; e2 -= e3;
}

VOID HfdBasis64::vTakeStep()
{
    e0 += e1;
    LONGLONG eqTmp = e2;
    e1 += e2;
    e2 += eqTmp; e2 -= e3;
    e3 = eqTmp;
}

VOID Bezier64::vInit(
    __in_ecount(4) const POINT*    aptfx,
        // Pointer to 4 control points
    __in_ecount_opt(1) const RECT*     prcfxVis,
        // Pointer to bound box of visible area (may be NULL)
    LONGLONG        eqError)
        // Fractional maximum error (32.32 format)
{
    LONGLONG eqTmp;

    cStepsHigh = 1;
    cStepsLow  = 0;

    xHigh.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
    yHigh.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);

// Initialize error:

    eqErrorLow = eqError;

    if (prcfxVis == (RECT*) NULL)
        prcfxClip = (RECT*) NULL;
    else
    {
        rcfxClip = *prcfxVis;
        prcfxClip = &rcfxClip;
    }

    while (((xHigh.vError(&eqTmp), eqTmp) > geqErrorHigh) ||
           ((yHigh.vError(&eqTmp), eqTmp) > geqErrorHigh))
    {
        cStepsHigh <<= 1;
        xHigh.vHalveStepSize();
        yHigh.vHalveStepSize();
    }
}

INT Bezier64::cFlatten(
    __out_ecount_part(cptfx, return) POINT* pptfx,
    INT cptfx,
    __out_ecount(1) BOOL *pbMore)
{
    POINT    aptfx[4];
    RECT     rcfxBound;
    LONGLONG eqTmp;
    INT      cptfxOriginal = cptfx;

    Assert(cptfx > 0);

    do {
        if (cStepsLow == 0)
        {
        // Optimization that if the bound box of the control points doesn't
        // intersect with the bound box of the visible area, render entire
        // curve as a single line:
    
            xHigh.vUntransform(&aptfx[0].x);
            yHigh.vUntransform(&aptfx[0].y);
    
            xLow.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
            yLow.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);
            cStepsLow = 1;
    
            if (prcfxClip != (RECT*) NULL)
                vBoundBox(aptfx, &rcfxBound);
    
            if (prcfxClip == (RECT*) NULL || bIntersect(&rcfxBound, prcfxClip))
            {
                while (((xLow.vError(&eqTmp), eqTmp) > eqErrorLow) ||
                       ((yLow.vError(&eqTmp), eqTmp) > eqErrorLow))
                {
                    cStepsLow <<= 1;
                    xLow.vHalveStepSize();
                    yLow.vHalveStepSize();
                }
            }
    
        // This 'if' handles the case where the initial error for the Bezier
        // is already less than the target error:
    
            if (--cStepsHigh != 0)
            {
                xHigh.vTakeStep();
                yHigh.vTakeStep();
    
                if (((xHigh.vError(&eqTmp), eqTmp) > geqErrorHigh) ||
                    ((yHigh.vError(&eqTmp), eqTmp) > geqErrorHigh))
                {
                    cStepsHigh <<= 1;
                    xHigh.vHalveStepSize();
                    yHigh.vHalveStepSize();
                }
    
                while (!(cStepsHigh & 1) &&
                       ((xHigh.vParentError(&eqTmp), eqTmp) <= geqErrorHigh) &&
                       ((yHigh.vParentError(&eqTmp), eqTmp) <= geqErrorHigh))
                {
                    xHigh.vDoubleStepSize();
                    yHigh.vDoubleStepSize();
                    cStepsHigh >>= 1;
                }
            }
        }
    
        xLow.vTakeStep();
        yLow.vTakeStep();
    
        pptfx->x = xLow.fxValue();
        pptfx->y = yLow.fxValue();
        pptfx++;
    
        cStepsLow--;
        if (cStepsLow == 0 && cStepsHigh == 0)
        {
            *pbMore = FALSE;

            // '+1' because we haven't decremented 'cptfx' yet:

            return(cptfxOriginal - cptfx + 1);
        }
    
        if (((xLow.vError(&eqTmp), eqTmp) > eqErrorLow) ||
            ((yLow.vError(&eqTmp), eqTmp) > eqErrorLow))
        {
            cStepsLow <<= 1;
            xLow.vHalveStepSize();
            yLow.vHalveStepSize();
        }
    
        while (!(cStepsLow & 1) &&
               ((xLow.vParentError(&eqTmp), eqTmp) <= eqErrorLow) &&
               ((yLow.vParentError(&eqTmp), eqTmp) <= eqErrorLow))
        {
            xLow.vDoubleStepSize();
            yLow.vDoubleStepSize();
            cStepsLow >>= 1;
        }

    } while (--cptfx != 0);

    *pbMore = TRUE;
    return(cptfxOriginal);
}



