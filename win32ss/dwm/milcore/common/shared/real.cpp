// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//
//   Floating point arithmetic support.
//

#include "precomp.hpp"

#define FREE_BUILD_FP_BARRIER 0

CFloatFPU::CFloatFPU()
{
    #if defined(_X86_)
    
        WORD wSaveState;

        // Save control word:

        _asm fnstcw  wSaveState

        m_wFPUControl = wSaveState;
        
        const WORD c_mask = static_cast<WORD>(~c_wFPCtrlMask);

        // setting the state is expensive, only do it if we're not already
        // in the required state.
        
        if((wSaveState & c_wFPCtrlMask) != c_wFPCtrlSingle)
        {
            // clear the current exception state.
            // fnclex is a non-wait version of fclex - which means it clears
            // the exceptions without taking any pending unmasked exceptions.
            // We issue a fclex so that any unmasked exceptions are 
            // triggered immediately. This indicates a bug in the caller
            // of the API.

            _asm fclex

            // Round to nearest, single precision, clear all exception masks.
            
            _asm mov     ax, wSaveState
            _asm and     ax, c_mask
            _asm or      ax, c_wFPCtrlSingle
            _asm mov     wSaveState, ax
            _asm fldcw   wSaveState
        }

    #endif
    
    #if DBG
//        InterlockedIncrement(&m_cNesting);
    #endif    
}

CFloatFPU::~CFloatFPU()
{
    AssertMode();
    
    #if DBG
//        InterlockedDecrement(&m_cNesting);
    #endif
    
    #if defined(_X86_)

        WORD wSaveState = m_wFPUControl;

        // setting the state is expensive, only do it if we're not already
        // in the required state.
        
        if((wSaveState & c_wFPCtrlMask) != c_wFPCtrlSingle)
        {
            // Clear the exception state.
            // Note: We issue the fwait and then an fnclex - which is equivalent
            // to the fclex instruction (9B DB E2) which causes us to 
            // immediately take any unmasked pending exceptions.
            // Because we clear the exception state on the way in, hitting 
            // an exception on this line means we generated an exception
            // in our code (or a call out to other code between the 
            // CFloatFPU constructor and destructor nesting) which was 
            // pending and not handled.

            _asm fclex

            // Restore control word (rounding mode and exception masks):
    
            _asm fldcw wSaveState    
        }
    #endif
}


// AssertMode.
//
// AssertMode does nothing in Free builds, unless the FREE_BUILD_FP_BARRIER
// define is set to 1. Debug builds always have FP barriers.
// If exceptions are unmasked and you're getting delayed FP exceptions, 
// turn on the FP barriers and add FPUStateSaver::AssertMode() calls 
// bracketing all your calls. This will allow you to isolate the FP 
// exception generator.

#if DBG

void CFloatFPU::AssertMode()
{
//    ASSERTMSG(m_cNesting >= 1, ("FPU mode not set via CFloatFPU class"));
    
    #if defined(_X86_)

/*
    UINT32 tempState;
    
    // Issue a FP barrier. Take all pending exceptions now.
    
    _asm fwait
    
    // get the control word

    _asm fnstcw  WORD PTR tempState
    
    // Assert that the control word is still set to our preferred
    // rounding mode and exception mask.
    // If we take this Assert, there was an unauthorized change of 
    // the FPU rounding mode or exception mask settings between
    // the FPUStateSaver constructor and destructor.
    
    ASSERTMSG( 
        (tempState & c_wFPCtrlMask) == c_wFPCtrlSingle,
        ("Incorrect FPU Control Word")
    );
*/    
    #endif
}

//+----------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::AssertPrecisionAndRoundingMode
//
//  Synopsis:
//      Check floating point unit state. Assert if rounding mode differs from
//      expected "round to nearest" or if precision is not 24 bit.
//
//-----------------------------------------------------------------------------
void
CFloatFPU::AssertPrecisionAndRoundingMode()
{
#if defined(_X86_)
    // Future Consideration:   X64 and X86 floating point consistency
    //  The assembly is also supported on X64 platforms and we should consider
    //  consistency.
    // IA64 processors are capable to control precision on per-command basis, and do not
    // require precautions.

    WORD uFPUState;
    
    _asm fnstcw uFPUState;
    
    uFPUState &= (c_wFPCtrlPrecisionMask | c_wFPCtrlRoundingMask);
    AssertConstMsg(uFPUState == (c_wFPCtrlPCSingle | c_wFPCtrlRCNearestEven),
                   "Wrong FPU Mode");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::AssertRoundingMode
//
//  Synopsis:
//      Check floating point unit state. Assert if rounding mode differs from
//      expected "round to nearest".
//
//-----------------------------------------------------------------------------
void
CFloatFPU::AssertRoundingMode()
{
#if defined(_X86_)
    // Future Consideration:   X64 and X86 floating point consistency
    //  The assembly is also supported on X64 platforms and we should consider
    //  consistency.

    WORD uFPUState;
    
    _asm fnstcw uFPUState;
    
    uFPUState &= c_wFPCtrlRoundingMask;
    AssertConstMsg(uFPUState == c_wFPCtrlRCNearestEven,
                   "Wrong FPU Rounding Mode");
#endif
}

LONG CFloatFPU::m_cNesting = 0;

#endif


CDoubleFPU::CDoubleFPU()
{
    #if defined(_X86_)
    
        WORD wSaveState;

        // clear the current exception state.
        // fnclex is a non-wait version of fclex - which means it clears
        // the exceptions without taking any pending unmasked exceptions.
        // We issue a fclex so that any unmasked exceptions are 
        // triggered immediately. This indicates a bug in the caller
        // of the API.
    
        _asm fclex
        
        // Save control word:

        _asm fnstcw  wSaveState

        m_wFPUControl = wSaveState;

        const WORD c_mask = static_cast<WORD>(~c_wFPCtrlMask);

        // setting the state is expensive, only do it if we're not already
        // in the required state.
        
        if((wSaveState & c_wFPCtrlMask) != c_wFPCtrlDouble)
        {
            // Round to nearest, double precision, clear all exception masks.
            
            _asm mov     ax, wSaveState
            _asm and     ax, c_mask
            _asm or      ax, c_wFPCtrlDouble
            _asm mov     wSaveState, ax
            _asm fldcw   wSaveState
        }

    #endif
    
    #if DBG
        InterlockedIncrement(&m_cNesting);
    #endif    
}

CDoubleFPU::~CDoubleFPU()
{
    AssertMode();
    
    #if DBG
        InterlockedDecrement(&m_cNesting);
    #endif
    
    #if defined(_X86_)

        WORD wSaveState = m_wFPUControl;

        // Clear the exception state.
        // Note: We issue the fwait and then an fnclex - which is equivalent
        // to the fclex instruction (9B DB E2) which causes us to 
        // immediately take any unmasked pending exceptions.
        // Because we clear the exception state on the way in, hitting 
        // an exception on this line means we generated an exception
        // in our code (or a call out to other code between the 
        // CDoubleFPU constructor and destructor nesting) which was 
        // pending and not handled.
        
        _asm fclex

        // setting the state is expensive, only do it if we're not already
        // in the required state.
        
        if((wSaveState & c_wFPCtrlMask) != c_wFPCtrlDouble)
        {
            // Restore control word (rounding mode and exception masks):
    
            _asm fldcw wSaveState    
        }

    #endif
}

// AssertMode.
//
// AssertMode does nothing in Free builds, unless the FREE_BUILD_FP_BARRIER
// define is set to 1. Debug builds always have FP barriers.
// If exceptions are unmasked and you're getting delayed FP exceptions, 
// turn on the FP barriers and add FPUStateSaver::AssertMode() calls 
// bracketing all your calls. This will allow you to isolate the FP 
// exception generator.

#if DBG

void CDoubleFPU::AssertMode()
{
    AssertMsg(m_cNesting >= 1, "FPU mode not set via CDoubleFPU class");
    
    #if defined(_X86_)

    UINT32 tempState;
    
    // Issue a FP barrier. Take all pending exceptions now.
    
    _asm fwait
    
    // get the control word

    _asm fnstcw  WORD PTR tempState
    
    // Assert that the control word is still set to our preferred
    // rounding mode and exception mask.
    // If we take this Assert, there was an unauthorized change of 
    // the FPU rounding mode or exception mask settings between
    // the FPUStateSaver constructor and destructor.
    
    AssertMsg((tempState & c_wFPCtrlMask) == c_wFPCtrlDouble,
        "Incorrect FPU Control Word");
    
    #endif
}

LONG CDoubleFPU::m_cNesting = 0;

#else

void CDoubleFPU::AssertMode() 
{
    #if defined(_X86_)
    #if FREE_BUILD_FP_BARRIER
    _asm fwait
    #endif
    #endif
}

#endif



/**************************************************************************\
*
* Function Description:
*
*   Internal definition of MSVCRT's pow()
*
* Arguments:
*
*   x - base
*   y - exponent
*
* Return Value:
*
*   x^y
*
* Notes:
*
*   I purposefully didn't make our code use Office's implementation when
*   we do the Office build. I want to avoid needing a separate build for
*   Office if I can.
*
* Created:
*
*   10/19/1999 agodfrey
*       Stole it from Office's code (mso\gel\gelfx86.cpp)
*
\**************************************************************************/

double
GpRealInstrinsics::Pow(
    double x, 
    double y
)
{
    // make sure we're in double precision mode.
    // This Assert is disabled because there are a bunch of callers abusing
    // this function - passing in doubles and not noticing they're getting
    // single-precision math.
    
    // CDoubleFPU::AssertMode();

#if defined(_X86_)
    
    static const double fphalf = 0.5;
    static const double fpone = 1.0;

    if ( x == 0.0 )
    {
        if ( y > 0.0 )
        {
            return 0.0;
        }

        if (y == 0.0)
        {
            TraceTag((tagMILWarning, "Call Pow(x, y) with x=0.0 and y=0.0"));
            return 1.0; // sic
        }

        if ( y < 0.0 )
        {
            TraceTag((tagMILWarning, "Call Pow(x, y) with x=0.0 and y < 0.0"));

            // return INF to comply with MSDN. Since we don't have INF defined
            // in our header files, we use DBL_MAX which should be
            // sufficient.
            // !!!Todo[minliu], figure out how to return INF

            return DBL_MAX;
        }
    }// x == 0.0

    if (y == 0.0)
    {
        return 1.0;
    }

    __asm FLD QWORD PTR [y];   // becomes ST(1)
    __asm FLD QWORD PTR [x];   // becomes ST
    __asm FYL2X;               // ST := ST(1) * log2(ST)
    __asm FST QWORD PTR [x];

    /* Do this in C++ just to avoid all the messing around with the condition
        flags, keep x in ST(0) while doing this. */
    if (fabs(x) < fpone)
    {
        /* The power is in the range which F2XM1 can handle. */
        __asm F2XM1;                  // ST := 2^ST - 1
        __asm FADD QWORD PTR [fpone]; // ST := 2^mantissa
    }
    else
    {
        /* The power needs to be handled as a separate fractional and whole
            number part, as F2XM1 only handles fractions.  Note that we don't
            care about the rounding mode here - we just need to split x
            into two parts one of which is <1.0. */
        __asm FLD ST;                 // Duplicate ST
        __asm FRNDINT;                // Integral value in ST
        //NB: doc bug in the x86 manual, the following does ST(1):=ST(1)-ST
        __asm FSUB ST(1), ST;         // Fractional value in ST(1)
        __asm FXCH;                   // Factional value in ST
        __asm F2XM1;                  // ST := 2^ST - 1
        __asm FADD QWORD PTR [fpone]; // ST := 2^frac
        __asm FSCALE;                 // ST := 2^frac * 2^integral
        __asm FSTP ST(1);             // FSCALE does not pop anything
    }

    __asm FSTP QWORD PTR [x];
    return x;

#else

// No choice at the moment - we have to use the CRT. We'll watch what
// Office does when they start caring about IA64.

#undef pow
    return pow(x,y);

#endif
}

/**************************************************************************\
*
* Function Description:
*
*   Out-of-line version of exp()
*
* Arguments:
*
*   x - input value
*
* Return Value:
*
*   e^x
*
* Notes:
*
*   Because we compile 'optimize for size', the compiler refuses to inline
*   calls to exp() - because its implementation is quite long. So, we
*   make an out-of-line version by setting the optimizations correctly
*   here, and generating an inline version. Yes, I agree completely.
*
* Created:
*
*   10/20/1999 agodfrey
*       Wrote it.
*
\**************************************************************************/

#pragma optimize("gt", on)

double
GpRealInstrinsics::Exp(
    double x
)
{
#undef exp
    return exp(x);
}
#pragma optimize("", on)

// This definition assumes y > 0.
// GpModF(x, Inf) = x, as long as x != Inf.
REAL GpModF(REAL x, REAL y)
{
    Assert(y > 0);

    REAL rMod = 0;

    if (x >= 0)
    {
        if (x < y)
        {
            rMod = x;
        }
        else
        {
            rMod = static_cast<REAL> (x - ((INT) (x/y))*y);
        }
    }
    else
    {
        // x is negative or NaN
        x *= -1;

        if (x < y)
        {
            rMod = x;
        }
        else
        {
            rMod = static_cast<REAL> (x - ((INT) (x/y))*y);
        }

        rMod = y - rMod;
    }

    return min(max(rMod,0.0f), y);
}


//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::LargeRound
//
//  Synopsis:
//      Convert given floating point number to closest integer.
//      Half integers are shifted up.
//
//      This code should not be inlined.
//      It serves as a helper for inlined CFloatFPU::Round
//      and is supposed to be called seldom.
//
//-------------------------------------------------------------------------
int CFloatFPU::LargeRound(float x)
{
#if defined(_X86_)
    if (CCPUInfo::HasSSE())
    {
        FI fi;
        static const float negHalf = -0.5f;
        __asm
        {
            movss    xmm0,x;        // load given value to xmm0
            cvtss2si eax,xmm0;      // convert it to integer (rounding mode doesn't matter)
            cvtsi2ss xmm1,eax;      // convert back to float
            subss    xmm1,xmm0;     // xmm1 = (rounded - given)
            movss    xmm0,negHalf;  // load -0.5f
            cmpless  xmm1,xmm0;     // get all-ones if (rounded - given) < -0.5f
            movss    fi.f,xmm1;     // get comparison result as integer
            sub      eax,fi.i;      // correct the result of rounding
        }
    }
    else
    {
        return RoundWithHalvesUp(x);
    }
    
#elif defined(_AMD64_)
    __m128 given = _mm_set_ss(x);                       // load given value
    __int32 result = _mm_cvtss_si32(given);             // convert it to integer (rounding mode doesn't matter)
    __m128 rounded = _mm_cvtsi32_ss(rounded, result);   // convert back to float
    __m128 diff = _mm_sub_ss(rounded, given);           // diff = (rounded - given)
    __m128 negHalf = _mm_set_ss(-0.5f);                 // load -0.5f
    __m128 mask = _mm_cmple_ss(diff, negHalf);          // get all-ones if (rounded - given) < -0.5f
    __int32 correction;                                 // get comparison result as integer
    _mm_store_ss(reinterpret_cast<float*>(&correction), mask);
    return result - correction;                         // correct the result of rounding
#else
    return static_cast<int>(floorf(x + 0.5f));
#endif
}

#if defined(_X86_)

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::LargeFloor (X86 only)
//
//  Synopsis:
//      Convert given floating point value to closest integer
//      that is less than or equal to given.
//
//      This code should not be inlined.
//      It serves as a helper for inlined CFloatFPU::FLoor
//      and is supposed to be called seldom.
//
//-------------------------------------------------------------------------
int CFloatFPU::LargeFloor(float x)
{
    if (CCPUInfo::HasSSE())
    {
        FI fi;
        __asm
        {
            movss    xmm0,x;        // load given value to xmm0
            cvtss2si eax,xmm0;      // convert it to integer (rounding mode doesn't matter)
            cvtsi2ss xmm1,eax;      // convert back to float
            cmpnless xmm1,xmm0;     // get all-ones 32-bit value if converted is greater than given
            movss    fi.f,xmm1;     // get comparison result as integer
            add      eax,fi.i;      // correct result
        }
    }
    else
    {
        int i = RoundWithHalvesToEven(x);

        if (static_cast<float>(i) > x)
        {
            i--;
        }

        return i;
    }
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::LargeCeiling (X86 only)
//
//  Synopsis:
//      Convert given floating point value to closest integer
//      that is greater than or equal to given.
//
//      This code should not be inlined.
//      It serves as a helper for inlined CFloatFPU::Ceiling
//      and is supposed to be called seldom.
//
//-------------------------------------------------------------------------
int CFloatFPU::LargeCeiling(float x)
{
    if (CCPUInfo::HasSSE())
    {
        FI fi;
        __asm
        {
            movss    xmm0,x;        // load given value to xmm0
            cvtss2si eax,xmm0;      // convert it to integer (rounding mode doesn't matter)
            cvtsi2ss xmm1,eax;      // convert back to float
            cmpltss  xmm1,xmm0;     // get all-ones 32-bit value if converted is less than given
            movss    fi.f,xmm1;     // get comparison result as integer
            sub      eax,fi.i;      // correct result
        }
    }
    else
    {
        int i = RoundWithHalvesToEven(x);

        if (static_cast<float>(i) < x)
        {
            i++;
        }

        return i;
    }
}

#endif


