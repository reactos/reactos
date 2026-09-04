// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//
//   Contains macros for helping with floating point arithmetic.
//
//-----------------------------------------------------------------------------

#pragma once


// IEEE single precision floating point value format:
//
//     Sign  |   Biased Exponent    |     Normalized Significand
// ----------+----------------------+--------------------------------
//    1 bit  |     8 bits           |   24 bits (23 encoded + 1 implied)
//
// Biased exponent values of 0 and 0xFF have special meaning.
// All other values define regular, or normalized, floating point
// value that can be calculated following way:
//
//      int sign = (code >> 31) & 1;
//      int biasedExponent = (code >> 23) & 0xFF;
//      int significand = code & 0x007FFFFF;
//      int exponent = biasedExponent - 127;
//      "math real" normalizedSignificand = 1 + significand/(2^23);
//      "math real" value = normalizedSignificand * (2^exponent);
//      if (sign != 0) value = -value;
//
// The biased exponent == 0 stands for denormalized numbers.
// The encoded value of denormalized float is defined following way:
//      // int sign, biasedExponent and significand are defained same way as above
//      Assert(biasedExponent == 0);
//      int exponent = -126; // == (biasedExponent - 127) + 1
//      "math real" normalizedSignificand = significand/(2^23);
//      "math real" value = normalizedSignificand * (2^exponent);
//      if (sign != 0) value = -value;
//
// The value of 0xFF in biased exponent has following meaning:
//      significand == 0: infinity
//      significand != 0: not a number


#define IEEE_FLOAT_AS_UINT(sign, exp, significand) \
    ( ((sign)<<31) | ( (((exp)&0xFF)+127)<<23 ) | ((significand) & 0x7FFFF) )


// The maximum integer value that can be converted to a float without risk
// of precision lost is 2^24, or 0x1000000. Smaller numbers fit in 24-bit
// normalized significand. Greater numbers need 25 or more bits so when
// casting to float some least bits will be lost.
//
#define MAX_INT_TO_FLOAT  (1 << 24) //  16777216
#define MIN_INT_TO_FLOAT -(1 << 24) // -16777216

// The minimum float value that's guaranteed to have zero fractional part
// is 2^23, or 0x800000. Numbers that are equal or greater than
// MIN_FLOAT_WITHOUT_FRACTION need 24 or more bits to represent, so
// normalized significand has no room for the fraction.
//
#define MIN_FLOAT_WITHOUT_FRACTION  static_cast<float>(1 << 23) //  8388608
#define MIN_FLOAT_WITHOUT_FRACTION_AS_UINT IEEE_FLOAT_AS_UINT(0,23,0)

/*****************************************************************************
    Intrinsic functions - it is essential that these be used because there is
    no library implementation.  Note that the intrinsic forms often have
    restricted behavior, e.g. the argument to a trignometric function must be
    less than 2^63 radians.
******************************************************************* JohnBo **/

#pragma intrinsic(sin, cos, tan)
#pragma intrinsic(atan, atan2)
#pragma intrinsic(sqrt)
#pragma intrinsic(log, log10, exp)
#pragma intrinsic(fabs)
#pragma intrinsic(fmod)

namespace GpRealInstrinsics 
{
/*
    [JohnBo]
    These in-line functions are required to force direct use of the
    instructions - without this the CI versions are used unless g optimization
    is switched on, with g optimization on the in-line function calls will be
    removed completely.
*/
#pragma optimize("g", on)
    inline double InlineSin(double x) { return sin(x); }
    inline double InlineCos(double x) { return cos(x); }
    inline double InlineTan(double x) { return tan(x); }
    inline double InlineATan(double x) { return atan(x); }
    inline double InlineATan2(double y, double x) { return atan2(y, x); }
    inline double InlineSqrt(double x) { return sqrt(x); }
    inline double InlineLog(double x) { return log(x); }
    inline double InlineLog10(double x) { return log10(x); }
    inline double InlineExp(double x) { return exp(x); }
/* Restore default optimization. */
#pragma optimize("", on)

    // Out-of-line math functions
    // pow: We implemented it ourselves
    // exp: Because the inline version is so long, the compiler won't
    //      inline it unless the original caller has 'generate fast code'
    //      set. Instead, we use an out-of-line version.
    double Pow(double, double);
    double Exp(double);
};

/* Force use of the in-line functions. */

#define sin(x) GpRealInstrinsics::InlineSin(x)
#define cos(x) GpRealInstrinsics::InlineCos(x)
#define tan(x) GpRealInstrinsics::InlineTan(x)
#define atan(x) GpRealInstrinsics::InlineATan(x)
#define atan2(y,x) GpRealInstrinsics::InlineATan2(y,x)
#define sqrt(x) GpRealInstrinsics::InlineSqrt(x)
#define log(x) GpRealInstrinsics::InlineLog(x)
#define log10(x) GpRealInstrinsics::InlineLog10(x)
#define exp(x) GpRealInstrinsics::Exp(x)

#define pow(x,y) GpRealInstrinsics::Pow(x,y)

/* Integer interfaces */
#pragma intrinsic(abs, labs)

// Our pixel positioning uses 28.4 fixed point arithmetic and therefore 
// anything below the threshold of 1/32 should be irrelevant.
// Our choice of PIXEL_EPSILON is 1/64 which should give us correct pixel
// comparisons even in the event of accumulated floating point error.

#define PIXEL_EPSILON   0.015625f     // 1/64

#ifndef REAL_EPSILON
#define REAL_EPSILON    FLT_EPSILON
#endif

// This is for computing the complexity of matrices. When you compose matrices
// or scale them up by large factors, it's really easy to hit the REAL_EPSILON
// limits without actually affecting the transform in any noticable way.
// e.g. a matrix with a rotation of 1e-5 degrees is, for all practical purposes,
// not a rotation.
#define MATRIX_EPSILON    (REAL_EPSILON*5000.0f)

// FIXED_24_EPSILON is a conservative estimate for the smallest delta that
// may be added to a fixed point 24-bit HW buffer mapped to 0..1 which
// will not equal the original number.
//
// An epsilon of 4 / 2^24 gives us 3 slop values. (= 2^-22)

#define FIXED_24_EPSILON    0.0000002384185791015625f

#define REAL_SQRT_2 1.4142135623730950488016887242097f

#define REALFMOD        fmodf
#define REALSQRT        sqrtf
#ifndef REALABS
#define REALABS         fabsf
#endif
#define REALSIN         sinf
#define REALCOS         cosf
#define REALATAN2       atan2f

// #define REAL_EPSILON    DBL_EPSILON
// #define REALFMOD        fmod
// #define REALSQRT        sqrt
// #define REALABS         fabs
// #define REALSIN         sin
// #define REALCOS         cos
// #define REALATAN2       atan2

// convert from unknown FLOAT type => REAL
#define TOREAL(x)       (static_cast<REAL>(x))

// defined separately for possibly future optimization LONG to REAL
#define LTOF(x)         (static_cast<REAL>(x))

// Return the positive integer remainder of a/b. b should not be zero.
// note that a % b will return a negative number 
// for a<0 xor b<0 which is not suitable for texture mapping
// or brush tiling.
// This macro computes the remainder of a/b
// correctly adjusted for tiling negative coordinates.

#define RemainderI(a, b)\
    ((a) >= 0 ? (a) % (b) : \
    (b) - 1 - ((-(a) - 1) % (b)))


// This definition assumes y > 0.
// GpModF(x, Inf) = x, as long as x != Inf.
REAL GpModF(REAL x, REAL y);

// These define the bits in the FPU control word that we care about.
// The high byte is the precision control (PC) and rounding control (RC)
// while the low byte defines the exception masks.

const WORD c_wFPCtrlMask = 0x0F3F;
const WORD c_wFPCtrlPrecisionMask = 0x0300;
const WORD c_wFPCtrlRoundingMask = 0x0C00;

// Rounding control - set to round-to-nearest (even)

const WORD c_wFPCtrlRCNearestEven = 0x0000;
const WORD c_wFPCtrlRCDown = 0x0400;
const WORD c_wFPCtrlRCUp = 0x0800;
const WORD c_wFPCtrlRCZero = 0x0C00;

// Precision control - set to single/double precision.

const WORD c_wFPCtrlPCSingle = 0x0000;
const WORD c_wFPCtrlPCDouble = 0x0200;

// Mask all FP exceptions.
// Exception bit pattern  543210
// Invalid operation      000001b
// Denormal               000010b
// Zero Divide            000100b
// Overflow               001000b
// Underflow              010000b
// Precision              100000b
//const WORD c_wFPCtrlExceptions = 0x0020;
const WORD c_wFPCtrlExceptions = 0x003F;

const WORD c_wFPCtrlSingle = (c_wFPCtrlExceptions | c_wFPCtrlRCNearestEven | c_wFPCtrlPCSingle);
const WORD c_wFPCtrlDouble = (c_wFPCtrlExceptions | c_wFPCtrlRCNearestEven | c_wFPCtrlPCDouble);

// FPSW (FPU Status Word) masks:
const WORD c_wFPStatusBusy          = 0x8000;   // FPU busy flag
const WORD c_wFPStatusC3            = 0x4000;   // condition code flag 3
const WORD c_wFPStatusStackTop      = 0x3800;   // register stack top
const WORD c_wFPStatusC2            = 0x0400;   // condition code flag 2
const WORD c_wFPStatusC1            = 0x0200;   // condition code flag 1
const WORD c_wFPStatusC0            = 0x0100;   // condition code flag 0
const WORD c_wFPStatusErrorSummary  = 0x0080;   // error summary flag
const WORD c_wFPStatusStackFailure  = 0x0040;   // register stack failure exception flag
const WORD c_wFPStatusPrecision     = 0x0020;   // precision exception flag
const WORD c_wFPStatusUnderflow     = 0x0010;   // underflow exception flag
const WORD c_wFPStatusOverflow      = 0x0008;   // overflow exception flag
const WORD c_wFPStatusZeroDivide    = 0x0004;   // zero divide exception flag
const WORD c_wFPStatusDenormal      = 0x0002;   // denormalized operand exception flag
const WORD c_wFPStatusInvalid       = 0x0001;   // invalid operation exception flag

//+----------------------------------------------------------------------------
//
//  Struct:
//      FPUENV
//
//  Synopsis:
//      Defines data format used by FLDENV and FSTENV instructions.
//
//-----------------------------------------------------------------------------
struct FPUENV
{
    WORD ControlWord;
    WORD Reserved1;
    WORD StatusWord;
    WORD Reserved2;
    WORD TagWord;
    WORD Reserved3;

    // Remaining fields are defined differently for CPU
    // in protected vs. real mode. We don't use them but
    // we need to reserve space.
    DWORD InstructionPointer1;
    DWORD InstructionPointer2;
    DWORD OperandPointer1;
    DWORD OperandPointer2;
};

//+----------------------------------------------------------------------------
//
//  Class:
//      CFloatFPU
//
//  Synopsis:
//      Sets the CPU Floating-Point Unit control word state to our preferred
//      internal state for single precision math.
//
//      Implements various functions to convert floating point numbers to
//      integers and other computation means related to floating point numbers
//      internal structure.
//
//-----------------------------------------------------------------------------
class CFloatFPU
{

private:

    // We need to save the FPU control word so we can restore it.
    
    WORD m_wFPUControl;

    #if DBG
    
    // For AssertMode, we use m_cNesting to keep track
    // of the nesting level of CFloatFPU. This is not thread safe.
    // At worst, that could cause spurious asserts; more likely it could
    // fail to catch some errors. To prevent the spurious asserts,
    // used interlocked instructions to modify it. To fix it properly,
    // we'd need to use per-thread storage.
    
    static LONG m_cNesting;
    
    #endif    

public:

    CFloatFPU();
    ~CFloatFPU();

    #if DBG
    static void AssertMode();
    static void AssertPrecisionAndRoundingMode();
    static void AssertRoundingMode();
    #else
    static void AssertMode() {}
    static void AssertPrecisionAndRoundingMode() {}
    static void AssertRoundingMode() {}
    #endif
    
    static int RoundWithHalvesUp(float x);
    static int RoundWithHalvesDown(float x);

    static int Floor(float x);
    static int Ceiling(float x);

    static int FloorFPU(float x);
    static int CeilingFPU(float x);

    static int Round(float x);

    static int Trunc(float x);

    // Saturation versions of the above conversion routines. Don't test for
    // equality to INT_MAX because, when converted to floating-point for the
    // comparison, the value is (INT_MAX + 1):

#define SATURATE(x, op) (x >= INT_MIN) ? ((x < INT_MAX) ? op(x) : INT_MAX) : INT_MIN

    static INT FloorSat(float x)   { return SATURATE(x, FloorFPU); }
    static INT TruncSat(float x)   { return SATURATE(x, Trunc); }
    static INT CeilingSat(float x) { return SATURATE(x, CeilingFPU); }
    static INT RoundSat(float x)   { return SATURATE(x, RoundWithHalvesUp); }

#undef SATURATE

    static INT FASTCALL SmallFloor(float x);
    static INT FASTCALL SmallCeiling(float x);
    static INT FASTCALL SmallRound(float x);

    static INT FASTCALL SmallFloorEx(float x);
    static INT FASTCALL SmallCeilingEx(float x);
    static INT FASTCALL SmallRoundEx(float x);

    static float FASTCALL OffsetToRounded(float x);

    static float FASTCALL CeilingF(float x);
    static float FASTCALL FloorF(float x);

    static float FASTCALL FloorFFast(float x);

    WORD RoundMode() const
    {
        return m_wFPUControl & c_wFPCtrlRoundingMask;
    }

    static float NextSmaller(float x);
    static float NextBigger(float x);

    static bool IsNaNF(const float &f);

private:

#if defined(_X86_)
    static int RoundWithHalvesToEven(float x);
    static int LargeFloor(float x);
    static int LargeCeiling(float x);
#endif

    static int LargeRound(float x);


    union FI
    {
        float f;
        __int32 i;
    };

public:
    // Maximum allowed argument for SmallRound
    static const UINT sc_uSmallMax = 0xFFFFF;

    // Binary representation of static_cast<float>(sc_uSmallMax)
    static const UINT32 sc_uBinaryFloatSmallMax = 0x497ffff0;
};

#if defined(_X86_)
//+----------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::RoundWithHalvesToEven  (x86 only)
//
//  Synopsis:
//      Convert given floating point number to closest integer.
//      Half integers are shifted to closest even number.
//
//  Notes:
//      This method is intended to be fast and employ the default FPU rounding
//      mode, which is round to nearest even.  This is not normally useful in
//      our rendering problem space because some calculations that you'd expect
//      to give the same result don't.  As an example consider translation by
//      an odd interger value - say 5.  Now start with 2.5.  We'd expect that
//      we can round to an integer before and after the translation by 5, but
//      we can't.  FPU Round(2.5) + 5 = 7, but FPU Round(2.5 + 5) = 8.
//
//      So callers will correct the result according to their preferred
//      rounding mode.
//
//      An alternative way to get specific rounding is to switch FPU rounding
//      mode to either "round up" or "round down".  Unfortunately it is too
//      slow for doing it twice on every call to this routine.  We also can't
//      do it for the whole thread, because it would distort all the floating
//      point calculations in external modules.
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE int
CFloatFPU::RoundWithHalvesToEven(float x)
{
    //
    // Assuming FPU rounding mode is "round to nearest".
    // This means half-integers are being rounded to nearest even number:
    //   0.5 -> 0
    //   1.5 -> 2
    //

    AssertRoundingMode();

    int i;

    _asm {
        fld     x
        fistp   i
    }

    return i;
}
#endif

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::RoundWithHalvesUp
//
//  Synopsis:
//      Convert given floating point number to closest integer.
//      Half integers are shifted up.
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE int
CFloatFPU::RoundWithHalvesUp(float x)
{
#if defined(_X86_)

    int i = RoundWithHalvesToEven(x);

    //
    // The routine RoundWithHalvesToEven() resolves ambiguity by shifting
    // half-integers to nearest even number:
    //      RoundWithHalvesToEven(0.5) -> 0
    //      RoundWithHalvesToEven(1.5) -> 2
    // Following code changes it so that half-integers are rounded up.
    //
    float offset = static_cast<float>(i) - x;
    if (offset <= -0.5f)
    {
        i++;
    }

    return i;
#else
    return static_cast<int>(floor(static_cast<double>(x)+0.5));
#endif
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::RoundWithHalvesDown
//
//  Synopsis:
//      Convert given floating point number to closest integer.
//      Half integers are shifted down.
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE int
CFloatFPU::RoundWithHalvesDown(float x)
{
#if defined(_X86_)

    int i = RoundWithHalvesToEven(x);

    //
    // The routine RoundWithHalvesToEven() resolves ambiguity by shifting
    // half-integers to nearest even number:
    //      RoundWithHalvesToEven(0.5) -> 0
    //      RoundWithHalvesToEven(1.5) -> 2
    // Following code changes it so that half-integers are rounded down.
    //
    float offset = static_cast<float>(i) - x;
    if (offset >= 0.5f)
    {
        i--;
    }

    return i;
#else
    return static_cast<int>(ceil(static_cast<double>(x)-0.5));
#endif
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::FloorFPU
//
//  Synopsis:
//      Convert given floating point value to closest integer
//      that is less than or equal to given.
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE int
CFloatFPU::FloorFPU(float x)
{
#if defined(_X86_)
    int i = RoundWithHalvesToEven(x);

    if (static_cast<float>(i) > x)
    {
        i--;
    }

    return i;
#else
    return static_cast<int>(floorf(x));
#endif
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::Floor
//
//  Synopsis:
//      Convert given floating point value to closest integer
//      that is less than or equal to given.
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE int
CFloatFPU::Floor(float x)
{
#if defined(_X86_)
    // cut off sign
    UINT32 xAbs = *reinterpret_cast<const UINT*>(&x) & 0x7FFFFFFF;

    return xAbs <= sc_uBinaryFloatSmallMax
        ? SmallFloor(x)
        : LargeFloor(x);

#elif defined(_AMD64_)
    __m128 given = _mm_set_ss(x);                       // load given value to xmm register
    __int32 result = _mm_cvtss_si32(given);             // convert it to integer (rounding mode doesn't matter)
    __m128 rounded = _mm_cvtsi32_ss(rounded, result);   // convert back to float
    __m128 mask = _mm_cmpgt_ss(rounded, given);         // get all-ones 32-bit value if converted is greater than given
    __int32 correction;
    _mm_store_ss(reinterpret_cast<float*>(&correction), mask);  // get comparison result as integer
    return result + correction;                         // correct result and return

#else
    return static_cast<int>(floorf(x));
#endif
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::CeilingFPU
//
//  Synopsis:
//      Convert given floating point value to closest integer
//      that is greater than or equal to given.
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE int
CFloatFPU::CeilingFPU(float x)
{
#if defined(_X86_)
    int i = RoundWithHalvesToEven(x);

    if (static_cast<float>(i) < x)
    {
        i++;
    }

    return i;

#else
    return static_cast<int>(ceilf(x));
#endif
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::Ceiling
//
//  Synopsis:
//      Convert given floating point value to closest integer
//      that is greater than or equal to given.
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE int
CFloatFPU::Ceiling(float x)
{
#if defined(_X86_)
    // cut off sign
    UINT32 xAbs = *reinterpret_cast<const UINT*>(&x) & 0x7FFFFFFF;

    return xAbs <= sc_uBinaryFloatSmallMax
        ? SmallCeiling(x)
        : LargeCeiling(x);

#elif defined(_AMD64_)
    __m128 given = _mm_set_ss(x);                       // load given value to xmm register
    __int32 result = _mm_cvtss_si32(given);             // convert it to integer (rounding mode doesn't matter)
    __m128 rounded = _mm_cvtsi32_ss(rounded, result);   // convert back to float
    __m128 mask = _mm_cmplt_ss(rounded, given);         // get all-ones 32-bit value if converted is less than given
    __int32 correction;
    _mm_store_ss(reinterpret_cast<float*>(&correction), mask);   // get comparison result as integer
    return result - correction;                         // correct result

#else
    return static_cast<int>(ceilf(x));
#endif
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::Trunc
//
//  Synopsis:
//      Truncate given floating point number to closest integer
//      that's less or equal by absolute value.
//      In other words, shift it toward zero.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE int CFloatFPU::Trunc(float x)
{
#if defined(_X86_)
    if (CCPUInfo::HasSSE())
    {
        __asm cvttss2si eax, x;
    }
    else
    {
        return static_cast<int>(x);
    }
#else //!defined(_X86_)
    // On I64 and AMD64 static_cast<int>(float) is compiled as single
    // instruction "cvttss2si eax,xmm0" that's really fast.
    // On other platforms we delegate troubles to compiler anyway.
    return static_cast<int>(x);
#endif
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::Round
//
//  Synopsis:
//      Convert given floating point number to closest integer.
//      Half integers are shifted up.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE int CFloatFPU::Round(float x)
{
    // cut off sign
    UINT32 xAbs = *reinterpret_cast<const UINT*>(&x) & 0x7FFFFFFF;

    return xAbs <= sc_uBinaryFloatSmallMax
        ? SmallRound(x)
        : LargeRound(x);
}

MIL_FORCEINLINE float FASTCALL CFloatFPU::CeilingF(float x)
{
    // Floats greater than or equal to MIN_FLOAT_WITHOUT_FRACTION are all integers
    if (!(fabsf(x) < MIN_FLOAT_WITHOUT_FRACTION))
    {
        // NaNs and infinities go this way also
        return x;
    }
    
    return static_cast<float>(CeilingFPU(x));
}

MIL_FORCEINLINE float FASTCALL CFloatFPU::FloorF(float x)
{
    // Floats greater than or equal to MIN_FLOAT_WITHOUT_FRACTION are all integers
    if (!(fabsf(x) < MIN_FLOAT_WITHOUT_FRACTION))
    {
        // NaNs and infinities go this way also
        return x;
    }
    
    return static_cast<float>(FloorFPU(x));
}

MIL_FORCEINLINE float FASTCALL CFloatFPU::FloorFFast(float x)
{
    // Floats greater than or equal to MIN_FLOAT_WITHOUT_FRACTION are all integers
#if defined(_X86_)
    // cut off sign
    UINT32 xAbs = *reinterpret_cast<const UINT*>(&x) & 0x7FFFFFFF;
    if (xAbs >= MIN_FLOAT_WITHOUT_FRACTION_AS_UINT)
    {
        // NaNs and infinities go this way also
        return x;
    }

    return xAbs <= sc_uBinaryFloatSmallMax
        ? static_cast<float>(SmallFloor(x))
        : static_cast<float>(LargeFloor(x));

#else
    if (!(fabsf(x) < MIN_FLOAT_WITHOUT_FRACTION))
    {
        // NaNs and infinities go this way also
        return x;
    }

  #if defined(_AMD64_)
    return static_cast<float>(Floor(x));
  #else
    return floorf(x);
  #endif
#endif
}

// Functions prefixed with "Gp" do not depend on FPU rounding mode.

MIL_FORCEINLINE INT FASTCALL  GpFloor(float x)      { return(CFloatFPU::FloorFPU(x));          }
MIL_FORCEINLINE INT FASTCALL  GpTrunc(float x)      { return(CFloatFPU::Trunc(x));             }
MIL_FORCEINLINE INT FASTCALL  GpCeiling(float x)    { return(CFloatFPU::CeilingFPU(x));        }
MIL_FORCEINLINE INT FASTCALL  GpRound(float x)      { return(CFloatFPU::RoundWithHalvesUp(x)); }

MIL_FORCEINLINE INT FASTCALL  GpFloorSat(float x)   { return(CFloatFPU::FloorSat(x));          }
MIL_FORCEINLINE INT FASTCALL  GpTruncSat(float x)   { return(CFloatFPU::TruncSat(x));          }
MIL_FORCEINLINE INT FASTCALL  GpCeilingSat(float x) { return(CFloatFPU::CeilingSat(x));        }
MIL_FORCEINLINE INT FASTCALL  GpRoundSat(float x)   { return(CFloatFPU::RoundSat(x));          }

MIL_FORCEINLINE bool FASTCALL GpIsNaNF(const float &x) { return(CFloatFPU::IsNaNF(x));         }

//+------------------------------------------------------------------------
//
//  Function:   CFloatFPU::SmallRound
//
//  Synopsis:   Convert given floating point value to nearest integer.
//              Half-integers are rounded up.
//
//              Important: this routine is fast but restricted:
//              given x should be within (-(0x100000-.5) < x < (0x100000-.5))
//
//  Details:    Implementation has abnormal looking that use to confuse
//              many people. However, it indeed works, being tested
//              thoroughly on x86 and ia64 platforms for literally
//              each possible argument values in the given range.
//              
//  More details:
//      Implementation is based on the knowledge of floating point
//      value representation. This 32-bits value consists of three parts:
//      v & 0x80000000 = sign
//      v & 0x7F800000 = exponent
//      v & 0x007FFFFF - mantissa
//
//      Let N to be a floating point number within -0x400000 <= N <= 0x3FFFFF.
//      The sum (S = 0xC00000 + N) thus will satisfy Ox800000 <= S <= 0xFFFFFF.
//      All the numbers within this range (sometimes referred to as "binade")
//      have same position of most significant bit, i.e. 0x800000.
//      Therefore they are normalized equal way, thus
//      providing the weights on mantissa's bits to be the same
//      as integer numbers have. In other words, to get
//      integer value of floating point S, when Ox800000 <= S <= 0xFFFFFF,
//      we can just throw away the exponent and sign, and add assumed
//      most significant bit (that is always 1 and therefore is not stored
//      in floating point value):
//      (int)S = (<float S as int> & 0x7FFFFF | 0x800000);
//      To get given N in as integer, we need to subtract back
//      the value 0xC00000 that was added in order to obtain
//      proper normalization:
//      N = (<float S as int> & 0x7FFFFF | 0x800000) - 0xC00000.
//      or
//      N = (<float S as int> & 0x7FFFFF           ) - 0x400000.
//
//      Hopefully, the text above explains how
//      following routine works:
//        int SmallRound1(float x)
//        {
//            union
//            {
//                __int32 i;
//                float f;
//            } u;
//
//            u.f = x + float(0x00C00000);
//            return ((u.i - (int)0x00400000) << 9) >> 9;
//        }
//      Unfortunatelly it is imperfect, due to the way how FPU
//      use to round intermediate calculation results.
//      By default, rounding mode is set to "nearest".
//      This means that when it calculates N+float(0x00C00000),
//      the 80-bit precise result will not fit in 32-bit float,
//      so some least significant bits will be thrown away.
//      Rounding to nearest means that S consisting of intS + fraction,
//      where 0 <= fraction < 1, will be converted to intS
//      when fraction < 0.5 and to intS+1 if fraction > 0.5.
//      What would happen with fraction exactly equal to 0.5?
//      Smart thing: S will go to intS if intS is even and
//      to intS+1 if intS is odd. In other words, half-integers
//      are rounded to nearest even number.
//      This FPU feature apparently is useful to minimize
//      average rounding error when somebody is, say,
//      digitally simulating electrons' behavior in plasma.
//      However for graphics this is not desired.
//
//      We want to move half-integers up, therefore
//      define SmallRound(x) as {return SmallRound1(x*2+.5) >> 1;}.
//      This may require more comments.
//      Let given x = i+f, where i is integer and f is fraction, 0 <= f < 1.
//      Let's wee what is y = x*2+.5:
//          y = i*2 + (f*2 + .5) = i*2 + g, where g = f*2 + .5;
//      If "f" is in the range 0 <= f < .5 (so correct rounding result should be "i"),
//      then range for "g" is .5 <= g < 1.5. The very first value, .5 will force
//      SmallRound1 result to be "i*2", due to round-to-even rule; the remaining
//      will lead to "i*2+1". Consequent shift will throw away extra "1" and give
//      us desired "i".
//      When "f" in in the range .5 <= f < 1, then 1.5 <= g < 2.5.
//      All these values will round to 2, so SmallRound1 will return (2*i+2),
//      and the final shift will give desired 1+1.
//
//      To get final routine looking we need to transform the combines
//      expression for u.f:
//            (x*2) + .5 + float(0x00C00000) ==
//             (x + (.25 + double(0x00600000)) )*2
//      Note that the ratio "2" means nothing for following operations,
//      since it affects only exponent bits that are ignored anyway.
//      So we can save some processor cycles avoiding this multiplication.
//
//      And, the very final beautification:
//      to avoid subtracting 0x00400000 let's ignore this bit.
//      This mean that we effectively decrease available range by 1 bit,
//      but we're chasing for performance and found it acceptable.
//      So 
//         return ((u.i - (int)0x00400000) << 9) >> 9;
//      is converted to
//         return ((u.i                  ) << 10) >> 10;
//      Eventually, will found that final shift by 10 bits may be combined
//      with shift by 1 in the definition {return SmallRound1(x*2+.5) >> 1;},
//      we'll just shift by 11 bits. That's it.
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT FASTCALL CFloatFPU::SmallRound(float x)
{
    AssertPrecisionAndRoundingMode();
    Assert(-(0x100000-.5) < x && x < (0x100000-.5));

    FI fi;
    fi.f = (float)(x + (0x00600000+.25));
    int result = fi.i << 10 >> 11;

    Assert(x < result + .5 && x >= result - .5);
    return result;
}

//+------------------------------------------------------------------------
//
//  Function:   CFloatFPU::SmallFloor
//
//  Synopsis:   Convert given floating point value to closest integer
//              that is less than or equal to given.
//              Given x should be within (-(0x100000-.5) < x < (0x100000-.5))
//              See comments for CFloatFPU::SmallRound for details.
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT FASTCALL CFloatFPU::SmallFloor(float x)
{
    AssertPrecisionAndRoundingMode();
    Assert(-(0x100000-.5) < x && x < (0x100000-.5));

    FI fi;
    fi.f = (float)(x + (0x00600000-.25));
    int result = fi.i << 10 >> 11;

    Assert(x >= result && x < result + 1);
    return result;
}

//+------------------------------------------------------------------------
//
//  Function:   CFloatFPU::SmallCeiling
//
//  Synopsis:   Convert given floating point value to closest integer
//              that is greater than or equal to given.
//              Given x should be within (-(0x100000-.5) < x < (0x100000-.5))
//              See comments for CFloatFPU::SmallRound for details.
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT FASTCALL CFloatFPU::SmallCeiling(float x)
{
    AssertPrecisionAndRoundingMode();
    Assert(-(0x100000-.5) < x && x < (0x100000-.5));

    FI fi;
    fi.f = (float)(x + (.25 - 0x00600000));
    return -(fi.i << 10 >> 11);
}

//+------------------------------------------------------------------------
//
//  Function:   CFloatFPU::SmallRoundEx
//
//  Synopsis:   same as CFloatFPU::SmallRound but handles bigger numbers:
//              given x may be within (-(0x200000-.5) < x < (0x200000-.5))
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT FASTCALL CFloatFPU::SmallRoundEx(float x)
{
    AssertPrecisionAndRoundingMode();
    Assert(-(0x200000-.5) < x && x < (0x200000-.5));

    FI fi;
    fi.f = (float)(x + (0x00600000+.25));
    int result = (fi.i - 0x00400000) << 9 >> 10;

    Assert(x < result + .5 && x >= result - .5);
    return result;
}

//+------------------------------------------------------------------------
//
//  Function:   CFloatFPU::SmallFloorEx
//
//  Synopsis:   same as CFloatFPU::SmallFloor but handles bigger numbers:
//              given x may be within (-(0x200000-.5) < x < (0x200000-.5))
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT FASTCALL CFloatFPU::SmallFloorEx(float x)
{
    AssertPrecisionAndRoundingMode();
    Assert(-(0x200000-.5) < x && x < (0x200000-.5));

    FI fi;
    fi.f = (float)(x + (0x00600000-.25));
    int result = (fi.i - 0x00400000) << 9 >> 10;

    Assert(x >= result && x < result + 1);
    return result;
}

//+------------------------------------------------------------------------
//
//  Function:   CFloatFPU::SmallCeilingEx
//
//  Synopsis:   same as CFloatFPU::SmallCeiling but handles bigger numbers:
//              given x may be within (-(0x200000-.5) < x < (0x200000-.5))
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT FASTCALL CFloatFPU::SmallCeilingEx(float x)
{
    AssertPrecisionAndRoundingMode();
    Assert(-(0x200000-.5) < x && x < (0x200000-.5));

    FI fi;
    fi.f = (float)(x + (.25 - 0x00600000));
    int result = -((fi.i - 0x00400000) << 9 >> 10);

    Assert(x <= result && x > result - 1);
    return result;
}


//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::OffsetToRounded
//
//  Synopsis:
//      Calculates the difference between integer number, closest to given,
//      and the given number itself.
//
//      Ambiguities that appear for half-integer that has two closest
//      integers are resolved so that the bigger one is chosen, so
//      the result is in the range
//      -0.5 < result <= 0.5
//      
//-------------------------------------------------------------------------
MIL_FORCEINLINE float FASTCALL CFloatFPU::OffsetToRounded(float x)
{
    // Floats greater than or equal to MIN_FLOAT_WITHOUT_FRACTION are all integers
    if (!(fabsf(x) < MIN_FLOAT_WITHOUT_FRACTION))
    {
        // NaNs and infinities go this way also
        return 0;
    }

#if defined(_X86_)

    float rounded;

    _asm {
        fld x
        frndint // this guy converts half-integer to nearest even number.
        fstp rounded
    }
#else
    float rounded = floorf(x+0.5f);
#endif

    float offset = rounded - x;
    if (offset <= -0.5f)
    {
        offset += 1;
    }

    Assert(fabs(offset) <= 0.5f);
    return offset;
}


//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::NextSmaller
//
//  Synopsis:
//      Compute max float less than given.
//
//  Note:
//      This routine works only for positive given numbers.
//      Negatives, zeros, infinity and NaN are asserted but not handled;
//
//-------------------------------------------------------------------------

MIL_FORCEINLINE float
CFloatFPU::NextSmaller(float x)
{
    FI fi;
    fi.f = x;

    // This routine works only for positive given numbers.
    // Following assertion detects infinity and NaNs.
    Assert(fi.i > 0 && fi.i < 0x7F800000);

    fi.i--;
    return fi.f;
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::NextBigger
//
//  Synopsis:
//      Compute max float less than given.
//
//  Note:
//      This routine works only for positive given numbers.
//      Negatives, infinity and NaN are asserted but not handled;
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE float
CFloatFPU::NextBigger(float x)
{
    FI fi;
    fi.f = x;

    // Following assertion detects infinity and NaNs.
    Assert(fi.i >= 0 && fi.i < 0x7F7FFFFF);

    fi.i++;
    return fi.f;
}

//+------------------------------------------------------------------------
//
//  Method:
//      CFloatFPU::IsNaNF
//
//  Synopsis:
//      Determine whether a given float is NaN, as quickly as possible.
//
//  Note:
//      On x86, perf tests indicate that this algorithm is about twice as
//      fast as self-comparison and about 10 times as fast as _isnan
//      (converting from float to double is slow).
//
//
//      Having "float" instead of "const float &" causes a 3 times slow
//      down. Using the FI union similarly causes a 3 times slow down. A
//      sample assembly difference is
//
//      fast version:
//          mov     ecx,dword ptr [ecx+eax*4]
//
//      slow version:
//          fld     dword ptr [ecx+eax*4]
//          fstp    dword ptr [ebp-0Ch]
//          mov     ecx,dword ptr [ebp-0Ch]
//
//
//      Future Consideration:  Evaluate this mechanism on AMD64
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE
bool
CFloatFPU::IsNaNF(const float &f)
{
#if defined(_X86_)
    return ((*((unsigned long *)&f)) & 0x7fffffff) > 0x7f800000;
#else
    return f != f;
#endif
}


/**************************************************************************
*
* Class Description:
*
*    CDoubleFPU
*    Sets the CPU Floating-Point Unit control word state to our preferred
*    internal state for double precision math.
*
* Created:
*
*   10/27/2001 asecchia
*      Created it.
*
**************************************************************************/

class CDoubleFPU
{

private:

    // We need to save the FPU control word so we can restore it.
    
    WORD m_wFPUControl;

    #if DBG
    
    // For AssertMode, we use m_cNesting to keep track
    // of the nesting level of CDoubleFPU. This is not thread safe.
    // At worst, that could cause spurious asserts; more likely it could
    // fail to catch some errors. To prevent the spurious asserts,
    // used interlocked instructions to modify it. To fix it properly,
    // we'd need to use per-thread storage.
    
    static LONG m_cNesting;
    
    #endif    

public:

    CDoubleFPU();
    ~CDoubleFPU();

    static VOID AssertMode();
    
    static INT FASTCALL Floor(double x);
    static INT FASTCALL Trunc(double x);
    static INT FASTCALL Ceiling(double x);
    static INT FASTCALL Round(double x);
    
private:

    static INT FASTCALL InternalRound(double x);    
};

MIL_FORCEINLINE INT FASTCALL CDoubleFPU::InternalRound(double x)
{
#if defined(_X86_)

    INT i;

    _asm {
        fld     x
        fistp   i
    }

    return i;
#else
    return static_cast<INT>(floor(x+0.5));
#endif
}

MIL_FORCEINLINE INT FASTCALL CDoubleFPU::Floor(double x)
{
    AssertMode();
    
    INT temp = InternalRound(x);
    
    if(x-temp < -DBL_EPSILON*fabs(x))  // x < temp within appropriate epsilon.
    {
        temp--;
    }
    
//    Assert(temp <= x);
    
    return temp;
}

MIL_FORCEINLINE INT FASTCALL CDoubleFPU::Trunc(double x)
{
    AssertMode();
    return (x>=0) ? Floor(x) : -Floor(-x);
}

MIL_FORCEINLINE INT FASTCALL CDoubleFPU::Ceiling(double x)
{
    AssertMode();
    return((INT) -Floor(-x));
}

MIL_FORCEINLINE INT FASTCALL CDoubleFPU::Round(double x)
{
    AssertMode();
    return InternalRound(x);
}

MIL_FORCEINLINE INT FASTCALL GpFloor(double x)      {return(CDoubleFPU::Floor(x));     }
MIL_FORCEINLINE INT FASTCALL GpTrunc(double x)      {return(CDoubleFPU::Trunc(x));     }
MIL_FORCEINLINE INT FASTCALL GpCeiling(double x)    {return(CDoubleFPU::Ceiling(x));   }
MIL_FORCEINLINE INT FASTCALL GpRound(double x)      {return(CDoubleFPU::Round(x));     }



// FPUStateSandbox
//
// This object is designed to sandbox FPU unsafe code.
// For example, many badly written printer drivers on win9x codebases
// manipulate the FPU state without restoring it on exit. In order to 
// prevent code like that from hosing us, we wrap calls to potentially
// unsafe code (like driver escapes) in an FPUStateSandbox.
// 
// This will guarantee that after calling the unsafe code, the FPU state
// (rounding mode and exceptions) are reset to our preferred state.
// Because we assume that we're restoring to our preferred state, we 
// Assert on our preferred state being set on entry. This means that
// the sandbox must be declared inside some top level FPUStateSaver block.
// This condition is not strictly necessary and if there is a requirement
// for an FPUStateSandbox not contained inside an FPUStateSaver, this
// Assert can be removed. The sandbox saves the current state and restores
// it on exit, so it can operate outside of our preferred state if required.
//
// So far we've found a number of printer drivers on win9x codebase that
// require sandboxing - e.g. HP4500c pcl.
//
// Couple of caveats: This code is designed to wrap simple calls out of 
// GDI+ to the printer driver, such as Escape. It's not intended to be
// nested or for use with GDI+ code. However, nesting will work. In 
// particular you should not call FPUStateSaver functions inside of 
// an FPUStateSandbox unless you've acquired another nested FPUStateSaver.
// The only anticipated need for this is for sandboxing a callback function
// that calls into our API again. In this case it's ok, because all the 
// GDI+ calls will be wrapped by a nested FPUStateSaver acquired at the 
// API.
//
// NOTE: The ASSERTs in GpRound will not catch the scenario when they're
// called inside of a sandbox and not properly surrounded by a FPUStateSaver.
// GpRound may work incorrectly inside of a sandbox because the unsafe code
// could change the rounding mode. It could also generate exceptions.

class FPUStateSandbox
{
private:

    UINT32 SavedState;
public:

    FPUStateSandbox()
    {
        // it is assumed that this call is issued inside of an 
        // FPUStateSaver block, so that the CTRL word is set to
        // our preferred state.
        
        #if defined(_X86_)
        
            #if DBG
                //
                // This code should not be called with non-empty register stack.
                //

                //
                // Read FPU environment
                //
                FPUENV fpuEnv;
                _asm fnstenv fpuEnv;

                //
                // Check register stack emptiness. Note that we can't rely
                // upon zero in c_wFPStatusStackTop. FPU allows saving stack
                // bottom registers and freeing them using FFREE instruction so
                // that register stack can wrap around safely.
                //
                Assert(fpuEnv.TagWord == 0xFFFF);

                //
                // Check whether register stack overflow has ever happened.
                // 
                Assert((fpuEnv.StatusWord & c_wFPStatusStackFailure) == 0);

            #endif

        UINT32 savedState;
        
        // We must protect the sandboxed code from clearing the exception
        // masks and taking an exception generated by GDI+.
        // We do this by issuing fclex - which takes any unmasked exceptions
        // and clears all of the exceptions after that (masked and unmasked)
        // giving the sandboxed code a clean state.
        
        _asm fnclex
        
        // Save control word:
        
        _asm fnstcw  WORD PTR savedState
        this->SavedState = savedState;

        #endif
    }

    ~FPUStateSandbox()
    {
        #if defined(_X86_)
            //
            // This code is called on returning from sandboxed code.
            // It detects possible FPU state mismatches and re-initializes
            // FPU so that caller code is protected. Note however that
            // the assertions signal of vulnerability inside sandboxed code.
            // When FPUStateSandbox is used to wrap dll loading/initializing,
            // this might implicitly cause a snowball of various dll loading;
            // if one of them has a bug breaking FPU state then others
            // can suffer of wrong FPU calculation results.
            // 
        

            //
            // The assertions below have been disabled as far as they served already.
            // It turned out so that FPU state is broken while loading d3d9.dll and
            // calling Direct3DCreate9 procedure.
            // 
            #if NEVER // replace "NEVER" with "DBG" to re-enable assertions

                //
                // Read FPU environment
                //
                FPUENV fpuEnv;
                _asm fnstenv fpuEnv;

                //
                // Check register stack emptiness. Note that we can't rely
                // upon zero in c_wFPStatusStackTop. FPU allows saving stack
                // bottom registers and freeing them using FFREE instruction so
                // that register stack can wrap around safely.
                //
                Assert(fpuEnv.TagWord == 0xFFFF);

                //
                // Check whether register stack overflow has ever happened.
                // 
                Assert((fpuEnv.StatusWord & c_wFPStatusStackFailure) == 0);

            #endif

        UINT32 savedState = this->SavedState;
    
        // Clear the current exception state.
        // Note that fclex/fnclex instruction should not be used here
        // because they don't clean FPU register stack. The caveat
        // is that if a buggy sandboxed code returns with FPU register
        // stack that's partially filled, this might be not noticed
        // during long time while free space in stack is sufficient for
        // calculations. This mismatch will be only revealed on code
        // snippets that use FPU intensively which can result in
        // both wrong calculation results and wrong comparison conclusion.

        _asm fninit;
        
        // Restore control word (rounding mode and exception masks):

        _asm fldcw   WORD PTR savedState    
        
        #endif
    }
};

/**************************************************************************\
*
* Function Description:
*
*   Return TRUE if two points are close. Close is defined as near enough
*   that the rounding to 32bit float precision could have resulted in the
*   difference. We define an arbitrary number of allowed rounding errors (10).
*   We divide by b to normalize the difference. It doesn't matter which point
*   we divide by - if they're significantly different, we'll return true, and
*   if they're really close, then a==b (almost).
*
* Arguments:
*
*   a, b - input numbers to compare.
*
* Return Value:
*
*   TRUE if the numbers are close enough.
*
* Created:
*
*   12/11/2000 asecchia 
*
\**************************************************************************/

inline BOOL IsCloseReal(const REAL a, const REAL b)
{
    // if b == 0.0f we don't want to divide by zero. If this happens
    // it's sufficient to use 1.0 as the divisor because REAL_EPSILON
    // should be good enough to test if a number is close enough to zero.
    
    // NOTE: if b << a, this could cause an FP overflow. Currently we mask
    // these exceptions, but if we unmask them, we should probably check
    // the divide.
    
    // We assume we can generate an overflow exception without taking down
    // the system. We will still get the right results based on the FPU
    // default handling of the overflow.
    
    // Ensure that anyone clearing the overflow mask comes and revisits this
    // assumption. If you hit this Assert, it means that the #O exception mask
    // has been cleared. Go check c_wFPCtrlExceptions.
    
    Assert(c_wFPCtrlSingle & 0x8);
    
    CFloatFPU::AssertMode();
    
    return( REALABS( (a-b) / ((b==0.0f)?1.0f:b) ) < 10.0f*REAL_EPSILON );
}

inline BOOL IsClosePointF(
    __in_ecount(1) const MilPoint2F &pt1,
    __in_ecount(1) const MilPoint2F &pt2
    )
{
    return (
        IsCloseReal(pt1.X, pt2.X) && 
        IsCloseReal(pt1.Y, pt2.Y)
    );
}

inline BOOL IsExactlyEqualRectD(
    __in_ecount(1) const MilPointAndSizeD &rect1,
    __in_ecount(1) const MilPointAndSizeD &rect2
    )
{
    return (rect1.X == rect2.X) &&
            (rect1.Y == rect2.Y) &&
            (rect1.Width == rect2.Width) &&
            (rect1.Height == rect2.Height);
}

inline REAL DegToRad(REAL rDegree)
{
    return TOREAL(rDegree * M_PI / 180.0f);
}

inline REAL RadToDeg(REAL rRad)
{
    return TOREAL(rRad * TOREAL(180.0 / M_PI));
}


inline double DegToRad(double rDegree)
{
    return (rDegree * M_PI / 180.0);
}

inline double RadToDeg(double rRad)
{
    return (rRad * 180.0 / M_PI);
}

inline bool IsNaNOrIsEqualTo(REAL rMaybeNaN, REAL rTestNumber)
{
    return (rMaybeNaN == rTestNumber) || _isnan(rMaybeNaN);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      BitwiseEquals
//
//  Synopsis:  
//      Because NaN != NaN, this function can be used to check to see whether
//      two values are equal to each other (even if they are NaN).
//

inline bool BitwiseEquals(FLOAT value1, FLOAT value2)
{
    return *reinterpret_cast<UINT32*>(&value1) == *reinterpret_cast<UINT32*>(&value2);
}



