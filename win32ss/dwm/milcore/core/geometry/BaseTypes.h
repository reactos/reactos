// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//      Note:       Do not modify the enumeration member values.
//
//  $Description:
//      Enumerations and number types used by the exact arithmetic module.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

namespace RobustIntersections
{
    // SIGNINDICATOR enumeration.
    // Do not modify the member values.
    enum SIGNINDICATOR
    {
        SI_STRICTLY_NEGATIVE = -1,
        SI_ZERO = 0,
        SI_STRICTLY_POSITIVE = 1
    };

    // COMPARISON enumeration.
    // Do not modify the member values.
    enum COMPARISON
    {
        C_STRICTLYLESSTHAN = -1,
        C_EQUAL = 0,
        C_STRICTLYGREATERTHAN = 1,
        C_UNDEFINED = 255
    };

    inline COMPARISON OppositeComparison(COMPARISON c)
    {
        Assert(c != C_UNDEFINED);

        return (static_cast<COMPARISON>(-c));
    }

    // An integer in the [-2^30, +2^30] interval exactly represented by an IEEE 754 double.
    typedef double Integer30;
    const Integer30 LARGESTINTEGER30 = 0x40000000;

    inline bool IsValidInteger30(double v)
    {
        Assert(_isnan(v) == 0);
        return v == floor(v) && v <= LARGESTINTEGER30 && v >= - LARGESTINTEGER30;
    }

    // An integer in the [-2^31, +2^31] interval exactly represented by an IEEE 754 double.
    typedef double Integer31;
    const Integer31 LARGESTINTEGER31 = 0x80000000;

    inline bool IsValidInteger31(double v)
    {
        Assert(_isnan(v) == 0);
        return v == floor(v) && v <= LARGESTINTEGER31 && v >= - LARGESTINTEGER31;
    }

    // An integer in the [-2^33, +2^33] interval exactly represented by an IEEE 754 double.
    typedef double Integer33;
    const Integer33 LARGESTINTEGER33 = 
        static_cast<Integer31>(LARGESTINTEGER31) * static_cast<Integer31>(4);
    
    inline bool IsValidInteger33(double v)
    {
        Assert(_isnan(v) == 0);
        Assert(LARGESTINTEGER33 == floor(LARGESTINTEGER33));
        Assert(LARGESTINTEGER33 - 1.0 == floor(LARGESTINTEGER33 - 1.0));
        Assert(- LARGESTINTEGER33 == floor(- LARGESTINTEGER33));
        return v == floor(v) && v <= LARGESTINTEGER33 && v >= - LARGESTINTEGER33;
    }

    // An integer in the [-2^53, +2^53] interval exactly represented by an IEEE 754 double.
    typedef double Integer53;
    const Integer53 LARGESTINTEGER53 = 
        static_cast<Integer33>(LARGESTINTEGER33) * static_cast<Integer33>(0x100000);
    
    inline bool IsValidInteger53(double v)
    {
        Assert(_isnan(v) == 0);
        Assert(LARGESTINTEGER53 == floor(LARGESTINTEGER53));
        Assert(LARGESTINTEGER53 - 1.0 == floor(LARGESTINTEGER53 - 1.0));
        Assert(- LARGESTINTEGER53 == floor(- LARGESTINTEGER53));
        return v == floor(v) && v <= LARGESTINTEGER53 && v >= - LARGESTINTEGER53;
    }

    // An integer equal to 2^26 exactly represented by an IEEE 754 double.
    const double LARGESTINTEGER26 = 0x4000000;
};

