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
//      Define CMILBezier, a class for flattening a bezier.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


//+-----------------------------------------------------------------------------
//
//  class HfdBasis32
//
//  Class for HFD vector objects.
//
//  Public Interface:
//      bInit(p1, p2, p3, p4)       - Re-parameterizes the given control points
//                                    to our initial HFD error basis.
//      vLazyHalveStepSize(cShift)  - Does a lazy shift.  Caller has to remember
//                                    it changes 'cShift' by 2.
//      vSteadyState(cShift)        - Re-parameterizes to our working normal
//                                    error basis.
//
//      vTakeStep()                 - Forward steps to next sub-curve
//      vHalveStepSize()            - Adjusts down (subdivides) the sub-curve
//      vDoubleStepSize()           - Adjusts up the sub-curve
//      lError()                    - Returns error if current sub-curve were
//                                    to be approximated using a straight line
//                                    (value is actually multiplied by 6)
//      fxValue()                   - Returns rounded coordinate of first point in
//                                    current sub-curve.  Must be in steady
//                                    state.
//

// The code is actually smaller when these methods are forced inline;
// this is one of the rare cases where 'forceinline' is warranted:
// use the commone MIL_FORCEINLINE so that debugging is easier.

// First conversion from original 28.4 to 18.14 format
#define HFD32_INITIAL_SHIFT 10

// Second conversion to 15.17 format
#define HFD32_ADDITIONAL_SHIFT 3

extern const LONG HFD32_SHIFT;
extern const INT HFD32_MAX_ERROR;
extern const LONG HFD32_ROUND;


class HfdBasis32
{
private:
    LONG  e0;
    LONG  e1;
    LONG  e2;
    LONG  e3;

public:
    MIL_FORCEINLINE LONG lParentErrorDividedBy4() 
    { 
        return(max(abs(e3), abs(e2 + e2 - e3))); 
    }

    MIL_FORCEINLINE LONG lError()                 
    { 
        return(max(abs(e2), abs(e3))); 
    }

    MIL_FORCEINLINE INT fxValue() const
    { 
        return((e0 + HFD32_ROUND) >> HFD32_SHIFT); 
    }

    MIL_FORCEINLINE BOOL bInit(INT p1, INT p2, INT p3, INT p4)
    {
    // Change basis and convert from 28.4 to 18.14 format:
    
        e0 = (p1                     ) << HFD32_INITIAL_SHIFT;
        e1 = (p4 - p1                ) << HFD32_INITIAL_SHIFT;
        
        e2 = 6 * (p2 - p3 - p3 + p4);
        e3 = 6 * (p1 - p2 - p2 + p3);

        if (lError() >= HFD32_MAX_ERROR)
        {
            // Large error, will require too many subdivision for this 32 bit hfd
            return FALSE;
        }
        
        e2 <<= HFD32_INITIAL_SHIFT;
        e3 <<= HFD32_INITIAL_SHIFT;

        return TRUE; 
    }
    
    MIL_FORCEINLINE VOID vLazyHalveStepSize(LONG cShift)
    {
        e2 = ExactShiftRight(e2 + e3,  1);
        e1 = ExactShiftRight(e1 - ExactShiftRight(e2, cShift), 1);
    }
    
    MIL_FORCEINLINE VOID vSteadyState(LONG cShift)
    {
    // We now convert from 18.14 fixed format to 15.17:
    
        e0 <<= HFD32_ADDITIONAL_SHIFT;
        e1 <<= HFD32_ADDITIONAL_SHIFT;
    
        register LONG lShift = cShift - HFD32_ADDITIONAL_SHIFT;
    
        if (lShift < 0)
        {
            lShift = -lShift;
            e2 <<= lShift;
            e3 <<= lShift;
        }
        else
        {
            e2 >>= lShift;
            e3 >>= lShift;
        }
    }
    
    MIL_FORCEINLINE VOID vHalveStepSize()
    {
        e2 = ExactShiftRight(e2 + e3, 3);
        e1 = ExactShiftRight(e1 - e2, 1);
        e3 = ExactShiftRight(e3, 2);
    }
    
    MIL_FORCEINLINE VOID vDoubleStepSize()
    {
        e1 += e1 + e2;
        e3 <<= 2;
        e2 = (e2 << 3) - e3;
    }
    
    MIL_FORCEINLINE VOID vTakeStep()
    {
        e0 += e1;
        register LONG lTemp = e2;
        e1 += lTemp;
        e2 += lTemp - e3;
        e3 = lTemp;
    }

    MIL_FORCEINLINE int ExactShiftRight(int num, int shift)
    {
        // Performs a shift to the right while asserting that we're not 
        // losing significant bits
     
        Assert(num == (num >> shift) << shift); 
        return num >> shift;
    }
};

//+-----------------------------------------------------------------------------
//
//  class Bezier32
//
//  Bezier cracker.
//
//  A hybrid cubic Bezier curve flattener based on KirkO's error factor.
//  Generates line segments fast without using the stack.  Used to flatten a
//  path.
//
//  For an understanding of the methods used, see:
//
//  Kirk Olynyk, "..."
//  Goossen and Olynyk, "System and Method of Hybrid Forward
//      Differencing to Render Bezier Splines"
//  Lien, Shantz and Vaughan Pratt, "Adaptive Forward Differencing for
//  Rendering Curves and Surfaces", Computer Graphics, July 1987
//  Chang and Shantz, "Rendering Trimmed NURBS with Adaptive Forward
//      Differencing", Computer Graphics, August 1988
//  Foley and Van Dam, "Fundamentals of Interactive Computer Graphics"
//
//  Public Interface:
//      bInit(pptfx)                - pptfx points to 4 control points of
//                                    Bezier.  Current point is set to the first
//                                    point after the start-point.
//      Bezier32(pptfx)             - Constructor with initialization.
//      vGetCurrent(pptfx)          - Returns current polyline point.
//      bCurrentIsEndPoint()        - TRUE if current point is end-point.
//      vNext()                     - Moves to next polyline point.
//

class Bezier32
{
private:
    LONG       cSteps;
    HfdBasis32 x;
    HfdBasis32 y;
    RECT       rcfxBound;

public:
    BOOL bInit(
        __in_ecount(4) const POINT* aptfx,
        __in_ecount_opt(1) const RECT*);
    INT cFlatten(
        __out_ecount_part(cptfx, return) POINT* pptfx,
        INT cptfx,
        __out_ecount(1) BOOL *pbMore);
};

class HfdBasis64
{
private:
    LONGLONG e0;
    LONGLONG e1;
    LONGLONG e2;
    LONGLONG e3;

public:
    VOID vInit(INT p1, INT p2, INT p3, INT p4);
    VOID vHalveStepSize();
    VOID vDoubleStepSize();
    VOID vTakeStep();
    VOID vUntransform(__out_ecount(7) LONG* afx) const;

    VOID vParentError(__out_ecount(1) LONGLONG* peq) const;
    VOID vError(__out_ecount(1) LONGLONG* peq) const;
    INT fxValue() const;
};

class Bezier64
{
private:
    HfdBasis64 xLow;
    HfdBasis64 yLow;
    HfdBasis64 xHigh;
    HfdBasis64 yHigh;

    LONGLONG    eqErrorLow;
    RECT*       prcfxClip;
    RECT        rcfxClip;

    LONG        cStepsHigh;
    LONG        cStepsLow;

public:

    INT cFlatten(
        __out_ecount_part(cptfx,return) POINT* pptfx,
        INT cptfx,
        __out_ecount(1) BOOL *pbMore);
    VOID vInit(
        __in_ecount(4) const POINT* aptfx,
        __in_ecount_opt(1) const RECT* prcfx,
        const LONGLONG eq);
};


#define BEZIER64_FRACTION 28

// The following is our 2^11 target error encoded as a 36.28 number
// (don't forget the additional 4 bits of fractional precision!) and
// the 6 times error multiplier:

const LONGLONG geqErrorHigh = (LONGLONG)(6 * (1L << 15) >> (32 - BEZIER64_FRACTION)) << 32;

#ifdef BEZIER_FLATTEN_GDI_COMPATIBLE

// The following is the default 2/3 error encoded as a 36.28 number,
// multiplied by 6, and leaving 4 bits for fraction:

const LONGLONG geqErrorLow = (LONGLONG)(4) << 32;

#else

// The following is the default 1/4 error encoded as a 36.28 number,
// multiplied by 6, and leaving 4 bits for fraction:

const LONGLONG geqErrorLow = (LONGLONG)(3) << 31;

#endif

//+-----------------------------------------------------------------------------
//
//  class CMILBezier
//
//  Bezier cracker.  Flattens any Bezier in our 28.4 device space down to a
//  smallest 'error' of 2^-7 = 0.0078.  Will use fast 32 bit cracker for small
//  curves and slower 64 bit cracker for big curves.
//
//  Public Interface:
//      vInit(aptfx, prcfxClip, peqError)
//          - pptfx points to 4 control points of Bezier.  The first point
//            retrieved by bNext() is the the first point in the approximation
//            after the start-point.
//
//          - prcfxClip is an optional pointer to the bound box of the visible
//            region.  This is used to optimize clipping of Bezier curves that
//            won't be seen.  Note that this value should account for the pen's
//            width!
//
//          - optional maximum error in 32.32 format, corresponding to Kirko's
//            error factor.
//
//      bNext(pptfx)
//          - pptfx points to where next point in approximation will be
//            returned.  Returns FALSE if the point is the end-point of the
//            curve.
//

class CMILBezier
{
private:

    union
    {
        Bezier64 bez64;
        Bezier32 bez32;
    } bez;

    BOOL bBez32;

public:

    // All coordinates must be in 28.4 format:

    CMILBezier(
        __in_ecount(4) const POINT* aptfx,
        __in_ecount_opt(1) const RECT* prcfxClip)
    {
        bBez32 = bez.bez32.bInit(aptfx, prcfxClip);
        if (!bBez32)
            bez.bez64.vInit(aptfx, prcfxClip, geqErrorLow);
    }

    // Returns the number of points filled in. This will never be zero.
    //
    // The last point returned may not be exactly the last control
    //            point. The workaround is for calling code to add an extra
    //            point if this is the case.
    
    INT Flatten(
        __out_ecount_part(cptfx, return) POINT* pptfx,
        INT cptfx,
        __out_ecount(1) BOOL *pbMore)
    {
        if (bBez32)
        {
            return(bez.bez32.cFlatten(pptfx, cptfx, pbMore));
        }
        else
        {
            return(bez.bez64.cFlatten(pptfx, cptfx, pbMore));
        }
    }
};                                  






