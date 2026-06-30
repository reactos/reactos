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
//      CZ64, CZ128, and CZ192 implementation.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

using namespace RobustIntersections;

// Constants

// Number of bytes and number of bits in a digit.
static const int DIGIT_BYTESIZE = sizeof(UINT);
static const int DIGIT_BITSIZE = 8 * sizeof(UINT);

// This must be #defined for SAL to recognize it.
#define MAX_DIGITS 30

// Global constant for debugging purposes.
#if DBG
bool g_fExactArithmeticDump = false;
#endif

// Implementation of helper functions.

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaNumDigits
//
//  Synopsis:
//      Computes the number of significant digits in an unsigned number.
//
//  Returns:
//      The number of significant digits.
//
//------------------------------------------------------------------------------
inline static 
UINT
EaNumDigits(
    __in_ecount(nl) const UINT* nn,          // The unsigned number base address.
    UINT nl                                  // The unsigned number length.
    ) 
{
    Assert(nl > 0);
    nn += nl;
    while (nl != 0 && *--nn == 0)
    {
        nl--;
    }
    
    return nl > 0 ? nl : 1;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaCompareDigits
//
//  Synopsis:
//      Compares two digits
//
//  Returns:
//      a COMPARISON
//
//------------------------------------------------------------------------------
inline static 
COMPARISON 
EaCompareDigits(
    UINT d1,    // The first digit
    UINT d2     // The second digit
    )
{
    return d1 > d2 ? C_STRICTLYGREATERTHAN : (d1 == d2 ? C_EQUAL : C_STRICTLYLESSTHAN);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaCompare
//
//  Synopsis:
//      Compares two unsigned numbers of arbitrary length.
//
//  Returns:
//      a COMPARISON
//
//------------------------------------------------------------------------------
inline static 
COMPARISON 
EaCompare(
    __in_ecount(ml) const UINT* mm,              // First number base address 
    IN int ml,                                   // First number length
    __in_ecount(nl) const UINT* nn,              // Second number base address
    IN int nl                                    // Second number length
    )
{
    Assert(mm && nn && ml > 0 && nl > 0);

    COMPARISON result = C_EQUAL;

    ml = EaNumDigits(mm, ml);
    nl = EaNumDigits(nn, nl);

    if (ml != nl)
    {
        result =  ml > nl ? C_STRICTLYGREATERTHAN : C_STRICTLYLESSTHAN;
    }
    else
    {
        while ((result == C_EQUAL) && (ml > 0)) 
        {
            ml--;
            result = EaCompareDigits(*(mm + ml), *(nn + ml));
        }
    }
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaCopyDigitsAndZeroOverflowDigit
//
//  Synopsis:
//      Copies digits and zeroes the last digit.
//
//  Note:
//      Destination must have at least nl + 1 digits.
//
//------------------------------------------------------------------------------
inline static 
void 
EaCopyDigitsAndZeroOverflowDigit(
    __inout_ecount(nl+1) UINT* dest,  // Destination number base address 
    __in_ecount(nl) const UINT* source,          // Source number base address
    __in_range(1, MAX_DIGITS) int nl             // Number of digits to copy, must be > 0
    )
{
    // Copy the nl first digits of source into destination, set the last digit of destination to 0.
    Assert(dest && source && nl > 0);
    CopyMemory(dest, source, nl * DIGIT_BYTESIZE);
    dest[nl] = 0;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaCopyDigits
//
//  Synopsis:
//      Copies digits.
//
//  Note:
//      Destination must have at least nl digits.
//
//------------------------------------------------------------------------------
inline static 
void 
EaCopyDigits(
    __inout_ecount(nl) UINT* dest,       // Destination number base 
    __in_ecount(nl) const UINT* source,              // Source number base
    __in_range(1, MAX_DIGITS) int nl                 // Number of digits to copy, must be > 0
    )
{
    // Copy the nl first digits of source into destination.
    Assert(dest && source && nl > 0);
    CopyMemory(dest, source, nl * DIGIT_BYTESIZE);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaAddCarry
//
//  Synopsis:
//      Adds a carry to an unsigned number of arbitrary length.
//
//  Returns:
//      The carry out, either 0 or 1
//
//  Note:
//      If the unsigned number equals MaxUINT^nl - 1, the function does access
//      *(nn + nl) which must exist.
//
//------------------------------------------------------------------------------
inline static 
UINT 
EaAddCarry(
    __inout_ecount(nl) UINT* nn,      // The unsigned number base address.  
    __in_range(1, MAX_DIGITS) int nl, // The number of significant digits.
    UINT uCarryIn                     // Carry, either 0 or 1.
)
{
    Assert(nn && nl > 0 && (uCarryIn < 2));

    UINT result;
    if (uCarryIn == 0)
    {
        result = 0;
    }
    else
    { 
        // Walk the digits starting from the least significant one,
        // add the carry, which is equal to 1,
        // continue iff the new digit is 0 and there are remaining 
        // digits.
        while (--nl >= 0 && !(++(*nn++)))
        {
            ;
        }

        // If nl is negative, all digits were equal to their maximum value.
        result = nl >= 0 ? 0 : 1;
    }
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      SubtractBorrow
//
//  Synopsis:
//      Subtracts a borrow from an unsigned number of arbitrary length.
//
//  Returns:
//      The borrow out, either 0 or 1
//
//  Note:
//      The function might access *(nn + nl) which must exist.
//
//------------------------------------------------------------------------------
inline static 
UINT 
EaSubtractBorrow(
    __inout_ecount(nl) UINT* nn,    // The unsigned number base address.  
    int nl,                         // The number of significant digits
    UINT uBorrowIn                  // Borrow, either 0 or 1.
)
{
    Assert(nn && nl > 0 && (uBorrowIn < 2));

    UINT result;
    if (uBorrowIn == 1)
    {
        result = 1;
    }
    else
    {
        // See EaAddCarry above. 
        while (--nl >= 0 && !((*nn++)--)) 
        {
            ;
        }
        result = nl >= 0 ? 1 : 0;
    }
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaAdd
//
//  Synopsis:
//      Adds two unsigned numbers. Modifies the first number.
//
//  Returns:
//      The carry out, either 0 or 1
//
//  Note:
//      The first number length must be greater or equal to the second number
//      length.
//
//------------------------------------------------------------------------------
inline static 
UINT 
EaAdd(
    __inout_ecount(ml) UINT* mm,                 // The first number. 
    __in_range(1, MAX_DIGITS) int ml,            // The first number number of significant digits.
    __in_ecount(nl) const UINT* nn,              // The second number.
    __in_range(1, ml) int nl,                    // The second number number of significant digits.
    UINT uCarryIn                                // Carry, either 0 or 1.
    )
{
    Assert(mm && ml > 0 && nn && nl > 0 && (uCarryIn < 2) && ml >= nl);

    unsigned __int64 c = uCarryIn;

    ml -= nl;
    while (--nl >= 0)
    {
        c += static_cast<unsigned __int64>(*mm) + *(nn++);
        *(mm++) = static_cast<UINT>(c);
        c >>= DIGIT_BITSIZE;
    }

    if (ml == 0)
    {
        return static_cast<UINT>(c);
    }
    else
    {
        return EaAddCarry(mm, ml, static_cast<UINT>(c));
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaSubtract
//
//  Synopsis:
//      Subtracts two unsigned numbers. Modifies the first number.
//
//  Returns:
//      The borrow out, either 0 or 1
//
//  Note:
//      The first number length must be greater or equal to the second number
//      length.
//
//------------------------------------------------------------------------------
inline static 
UINT 
EaSubtract(
    __inout_ecount(ml) UINT* mm,                 // The first number. 
    __in_range(1, MAX_DIGITS) int ml,               // The first number number of significant digits.
    __in_ecount(nl) const UINT* nn,              // The second number.
    __in_range(1, ml) int nl,                    // The second number number of significant digits.
    UINT uBorrowIn                               // Borrow, either 0 or 1.
    )
{
    Assert(mm && ml > 0 && nn && nl > 0 && uBorrowIn < 2 && ml >= nl);

    unsigned __int64 c = uBorrowIn;
    UINT invn;

    ml -= nl;
    while (nl > 0) 
    {
        invn = *(nn++) ^ (-1);
        c += static_cast<unsigned __int64>(*mm) + invn;
        *(mm++) = static_cast<UINT>(c);
        c >>= DIGIT_BITSIZE;
        nl--;
    }

    if (ml == 0)
    {
        return static_cast<UINT>(c);
    }
    else
    {
        return EaSubtractBorrow(mm, ml, static_cast<UINT>(c));
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaMultiplyDigit
//
//  Synopsis:
//      Adds the product of the multiplier and of the multiplicand to the sum
//
//  Returns:
//      The carry out, either 0 or 1
//
//  Notes:
//      Let p be the sum, m the multiplier, and d the multiplicand, let r = p +
//      m * d. This function returns the carry out of the operation on the right
//      hand side and as a side effect sets the digits of p to the pl first
//      significant digits of r. Therefore, the new value of p postfixed by the
//      carry out equals r.
//
//------------------------------------------------------------------------------
inline static 
UINT 
EaMultiplyDigit(
    __inout_ecount(pl) UINT* pp,          // The sum. 
    __in_range(ml+1, MAX_DIGITS) int pl,  // The sum's number of digits.
    __in_ecount(ml) const UINT* mm,       // The multiplier, strictly shorter than the sum. 
    __in_range(1, MAX_DIGITS) int ml,     // The multiplier's number of significant digits.
    UINT d                                // The multiplicand.
    )
{ 
    Assert(pp && mm && pl > ml && ml > 0);

    unsigned __int64 c = 0;
    UINT result;

    if (d == 0)
    {
        result = 0;
    }
    else if (d == 1)
    {
        result = EaAdd(pp, pl, mm, ml, 0);
    }
    else
    {
        pl -= ml;
        while (ml != 0) 
        {
            ml--;
            c += *pp + (static_cast<unsigned __int64>(d) * (*(mm++)));
            *(pp++) = static_cast<UINT>(c);
            c >>= DIGIT_BITSIZE;
        } 
        while (pl != 0) 
        {
            pl--;
            c += *pp;
            *(pp++) = static_cast<UINT>(c);
            c >>= DIGIT_BITSIZE;
        }
        result = static_cast<UINT>(c);
    }
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EaMultiply
//
//  Synopsis:
//      Adds the product of the multiplier and the multiplicand to the sum
//
//  Returns:
//      The carry out, either 0 or 1
//
//  Notes:
//      Let p be the sum, m the multiplier, and n the multiplicand, let r = p +
//      m * n. This function returns the carry out of the operation on the right
//      hand side and as a side effect sets the digits of p to the pl first
//      significant digits of r. Therefore, the new value of p postfixed by the
//      carry out equals r.
//
//------------------------------------------------------------------------------
inline static 
UINT 
EaMultiply(
    __inout_ecount(pl) UINT* pp,        // The sum. 
    __range(ml+nl, MAX_DIGITS) int pl,  // The sum's number of digits.
    __in_ecount(ml) const UINT* mm,     // The multiplier. 
    __in_range(1, MAX_DIGITS) int ml,   // The multiplier's number of significant digits.
    __in_ecount(nl) const UINT* nn,     // The multiplicand 
    __in_range(1, MAX_DIGITS) int nl    // The multiplicand's number of significant digits.
    )
{
    Assert(pp && pl > 0 && mm && ml > 0 && nn && nl > 0);
    Assert(pl >= ml + nl);

    UINT c;

    // Multiply one digit at a time.
    for (c = 0; nl-- > 0; pp++, nn++, pl--)
    {
        c += EaMultiplyDigit(pp, pl, mm, ml, *nn);
    }
    return c;
}


// Class CZ64 public methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ64::CZ64
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CZ64::CZ64(
    Integer31 value             // The value.
    ) : CZBase(3)
{
    Assert(IsValidInteger31(value));
    Assert(GetSize() == 3);

    ZeroMemory(m_uDigits, GetSize() * DIGIT_BYTESIZE); 
    if (value > 0) 
    { 
        SetSign(SI_STRICTLY_POSITIVE);
        m_uDigits[0] = static_cast<UINT>(value);
    }
    else if (value < 0) 
    {
        SetSign(SI_STRICTLY_NEGATIVE);
        m_uDigits[0] = static_cast<UINT>(- value);
    }
    else 
    {
        SetSign(SI_ZERO);
    }
    SetDigitsPointer(m_uDigits);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ64::Compare
//
//  Synopsis:
//      Compares this number with another one.
//
//  Returns:
//      The result of the comparison.
//
//------------------------------------------------------------------------------
COMPARISON 
CZ64::Compare(
    __in_ecount(1) const CZ64& other            // The number to compare.
    ) const
{
    SIGNINDICATOR eOtherSI = other.GetSign();
    SIGNINDICATOR eThisSI = GetSign();
    COMPARISON c = C_EQUAL;

    if (eThisSI > eOtherSI)
    {
        c = C_STRICTLYGREATERTHAN;
    }
    else if (eThisSI < eOtherSI)
    {
        c = C_STRICTLYLESSTHAN;
    }
    else if (eThisSI > 0)
    {
        c = EaCompare(GetDigits(), GetSize(), other.GetDigits(), other.GetSize());
    }
    else if (eThisSI < 0)
    {
        c = EaCompare(other.GetDigits(), other.GetSize(), GetDigits(), GetSize());
    }
    return c;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ64::Multiply
//
//  Synopsis:
//      Multiplies this number by another number.
//
//  Returns:
//      This number.
//
//  Notes:
//      There are limitations on the size of the operands.
//
//------------------------------------------------------------------------------
CZ64& 
CZ64::Multiply(
    __in_ecount(1) const CZ64& other            // The number to multiply by.
    )
{
    // Set *this to *this * other and return *this.

    Assert(GetDigitCount() == 1 && other.GetDigitCount() == 1);

    Assert(GetSize() == 3);
    UINT uNewDigits[2];
    ZeroMemory(uNewDigits, sizeof uNewDigits);

    (void)EaMultiply(uNewDigits, 2, GetDigits(), 1, other.GetDigits(), 1);
    ReplaceDigitsButKeepSign(uNewDigits, 2);
    SetSign(static_cast<SIGNINDICATOR>(GetSign() * other.GetSign()));    
    return *this;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ64::Dump
//
//  Synopsis:
//      Debug dump
//
//------------------------------------------------------------------------------
void
CZ64::Dump() const
{
    if (g_fExactArithmeticDump)
    {
        Assert(GetSize() == 3);
        MILDebugOutput(L"CZ64 sign=%d, digits %x %x %x\n", 
                       GetSign(), m_uDigits[0], m_uDigits[1], m_uDigits[2]);
    }
}
#endif

// Class CZ128 public methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ128::CZ128
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CZ128::CZ128(
    Integer53 value      // The value.
    ) : CZBase(5)
{
    Assert(IsValidInteger53(value));
    Assert(GetSize() == 5);

    ZeroMemory(m_uDigits, GetSize() * DIGIT_BYTESIZE); 
    SetSignAndFirstTwoDigits(value);
    SetDigitsPointer(m_uDigits);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ128::Compare
//
//  Synopsis:
//      Compares this number with another one.
//
//  Returns:
//      The result of the comparison.
//
//------------------------------------------------------------------------------
COMPARISON 
CZ128::Compare(
    __in_ecount(1) const CZ128& other           // The number to compare with.
    ) const
{
    SIGNINDICATOR eOtherSI = other.GetSign();
    SIGNINDICATOR eThisSI = GetSign();
    COMPARISON c = C_EQUAL;

    if (eThisSI > eOtherSI)
    {
        c = C_STRICTLYGREATERTHAN;
    }
    else if (eThisSI < eOtherSI)
    {
        c = C_STRICTLYLESSTHAN;
    }
    else if (eThisSI > 0)
    {
        Assert(eOtherSI > 0);
        c = EaCompare(GetDigits(), GetSize(), other.GetDigits(), other.GetSize());
    }
    else if (eThisSI < 0)
    {
        Assert(eOtherSI < 0);
        c = EaCompare(other.GetDigits(), other.GetSize(), GetDigits(), GetSize());
    }
    return c;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ128::Multiply
//
//  Synopsis:
//      Multiplies this number by another number.
//
//  Returns:
//      This number.
//
//  Notes:
//      There are limitations on the size of the operands.
//
//------------------------------------------------------------------------------
CZ128& 
CZ128::Multiply(
    __in_ecount(1) const CZ128& other           // The number to multiply with.
    )
{
    // Set *this to *this * other and return *this.
    // y is *this, z is other.

    UINT yl = GetDigitCount();
    UINT zl = other.GetDigitCount();
    Assert(yl > 0 && yl < GetSize() - 2 && zl > 0 && zl < GetSize() - 2);

    Assert(GetSize() == 5);
    UINT uNewDigits[5 - 1];
    ZeroMemory(uNewDigits, sizeof uNewDigits);

    (void)EaMultiply(uNewDigits, yl + zl, GetDigits(), yl, other.GetDigits(), zl);
    ReplaceDigitsButKeepSign(uNewDigits, yl + zl);
    SetSign(static_cast<SIGNINDICATOR>(GetSign() * other.GetSign()));
    
    return *this;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ128::Dump
//
//  Synopsis:
//      Debug dump.
//
//------------------------------------------------------------------------------
void
CZ128::Dump() const
{
    if (g_fExactArithmeticDump)
    {
        Assert(GetSize() == 5);
        MILDebugOutput(L"CZ128 sign=%d, digits %x %x %x %x %x\n", 
                GetSign(), m_uDigits[0], m_uDigits[1], m_uDigits[2], m_uDigits[3], m_uDigits[4]);
    }
}
#endif

// Class CZ192 public methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ192::CZ192
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
CZ192::CZ192(
    Integer33 value             // The initial value for this number.
    ) : CZBase(7)
{
    Assert(IsValidInteger33(value));
    Assert(GetSize() == 7);

    ZeroMemory(m_uDigits, GetSize() * DIGIT_BYTESIZE); 
    SetSignAndFirstTwoDigits(value);
    SetDigitsPointer(m_uDigits);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ192::Compare
//
//  Synopsis:
//      Compares this number with another one.
//
//  Returns:
//      The result of the comparison.
//
//------------------------------------------------------------------------------
COMPARISON 
CZ192::Compare(__in_ecount(1) const CZ192& other) const
{
    SIGNINDICATOR eOtherSI = other.GetSign();
    SIGNINDICATOR eThisSI = GetSign();
    COMPARISON c = C_EQUAL;

    if (eThisSI > eOtherSI)
    {
        c = C_STRICTLYGREATERTHAN;
    }
    else if (eThisSI < eOtherSI)
    {
        c = C_STRICTLYLESSTHAN;
    }
    else if (eThisSI > 0)
    {
        c = EaCompare(GetDigits(), GetSize(), other.GetDigits(), other.GetSize());
    }
    else if (eThisSI < 0)
    {
        c = EaCompare(other.GetDigits(), other.GetSize(), GetDigits(), GetSize());
    }
    return c;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ192::Add
//
//  Synopsis:
//      Adds another number to this number.
//
//  Returns:
//      This number.
//
//  Notes:
//      There are limitations on the size of the operands.
//
//------------------------------------------------------------------------------
CZ192& CZ192::Add(
    __in_ecount(1) const CZ192& other    // The number to add.
    )
{
    // Set *this to *this + other and return *this.
    // y is *this, z is other.

    UINT yl = GetDigitCount();
    UINT zl = other.GetDigitCount();
    Assert(yl < GetSize() && zl < GetSize());

    Assert(GetSize() == 7);
    UINT uNewDigits[7];
    ZeroMemory(uNewDigits, sizeof uNewDigits);

    if (GetSign() == other.GetSign()) 
    {
        // Add magnitudes, the sign does not change.
        switch (EaCompare(GetDigits(), yl, other.GetDigits(), zl)) 
        {
        case C_EQUAL: case C_STRICTLYGREATERTHAN:    // |y| >= |z|  
            EaCopyDigitsAndZeroOverflowDigit(uNewDigits, GetDigits(), yl);
            EaAdd(uNewDigits, yl + 1, other.GetDigits(), zl, 0);
            ReplaceDigitsButKeepSign(uNewDigits, yl + 1);
            break;

        case C_STRICTLYLESSTHAN:                     // |y| < |z|
            EaCopyDigitsAndZeroOverflowDigit(uNewDigits, other.GetDigits(), zl);
            EaAdd(uNewDigits, zl + 1, GetDigits(), yl, 0);
            ReplaceDigitsButKeepSign(uNewDigits, zl + 1);
            break;

        default:
            Assert(false);
            break;
        }
    }
    else 
    {
        // Subtract magnitudes.
        switch (EaCompare(GetDigits(), yl, other.GetDigits(), zl)) 
        {
        case C_EQUAL:                           // y = -z  
            SetToZero();
            break;

        case C_STRICTLYGREATERTHAN:             // |y| > |z|
            // We have enough digits. Keep sign and size.
            (void)EaSubtract(m_uDigits, yl, other.GetDigits(), zl, 1);
            break;

        case C_STRICTLYLESSTHAN:                // |y| < |z|
            EaCopyDigits(uNewDigits, other.GetDigits(), zl);
            (void)EaSubtract(uNewDigits, zl, GetDigits(), yl, 1);
            ReplaceDigitsButKeepSign(uNewDigits, zl);
            SetSign(other.GetSign());
            break;

        default:
            Assert(false);
            break;
        }
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ192::Subtract
//
//  Synopsis:
//      Subtracts another number from this number.
//
//  Returns:
//      This number.
//
//  Notes:
//      There are limitations on the size of the operands.
//
//------------------------------------------------------------------------------
CZ192& 
CZ192::Subtract(
    __in_ecount(1) const CZ192& other   // The number to subtract.
    )
{
    if (this == &other)
    {
        SetToZero();
    }
    else 
    {
        const_cast<CZ192&>(other).Negate();
        Add(other);
        const_cast<CZ192&>(other).Negate();
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ192::Multiply
//
//  Synopsis:
//      Multiplies this number by another number.
//
//  Returns:
//      This number.
//
//  Notes:
//      There are limitations on the size of the operands.
//
//------------------------------------------------------------------------------
CZ192& 
CZ192::Multiply(
    __in_ecount(1) const CZ192& other      // The number to multiply by.
    )
{
    // Set *this to *this * other and return *this.
    // y is *this, z is other.

    UINT yl = GetDigitCount();
    UINT zl = other.GetDigitCount();
    Assert(yl > 0 && zl > 0 && (yl + zl < GetSize()));

    Assert(GetSize() == 7);
    UINT uNewDigits[7 - 1];
    ZeroMemory(uNewDigits, sizeof uNewDigits);

    (void)EaMultiply(uNewDigits, yl + zl, GetDigits(), yl, other.GetDigits(), zl);
    ReplaceDigitsButKeepSign(uNewDigits, yl + zl);
    SetSign(static_cast<SIGNINDICATOR>(GetSign() * other.GetSign()));   
    return *this;
}

#if DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ128::Dump
//
//  Synopsis:
//      Prints this number digits.
//
//------------------------------------------------------------------------------
void
CZ192::Dump() const
{
    if (g_fExactArithmeticDump)
    {
        Assert(GetSize() == 7);
        MILDebugOutput(L"CZ192 sign=%d, digits %x %x %x %x %x %x %x\n", 
            GetSign(), m_uDigits[0], m_uDigits[1], m_uDigits[2], 
            m_uDigits[3], m_uDigits[4], m_uDigits[5], m_uDigits[6]);
    }
}
#endif 

// Class CZBase implementation methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZBase::GetDigitCount
//
//  Synopsis:
//      Computes this number number of significant digits.
//
//  Returns:
//      The number of significant digits.
//
//------------------------------------------------------------------------------
UINT
CZBase::GetDigitCount() const
{
    return EaNumDigits(GetDigits(), GetSize());
}

// Class CZ128 implementation methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ128::SetSignAndFirstTwoDigits
//
//  Synopsis:
//      Sets this number value to the argument.
//
//------------------------------------------------------------------------------
void 
CZ128::SetSignAndFirstTwoDigits(
    Integer53 v                         // The value.
    )
{
    Assert(IsValidInteger53(v) && GetSize() > 1);

    ULARGE_INTEGER tab;

    if (v > 0.0)
    {
        tab.QuadPart = static_cast<ULONGLONG>(v);
        SetSign(SI_STRICTLY_POSITIVE);
    }
    else if (v < 0.0)
    {
        tab.QuadPart = static_cast<ULONGLONG>(- v);
        SetSign(SI_STRICTLY_NEGATIVE);
    }
    else
    {
        tab.QuadPart = 0;
        SetSign(SI_ZERO);
    }
    m_uDigits[0] = tab.LowPart;
    m_uDigits[1] = tab.HighPart;
}

// Class CZ192 implementation methods.

//+-----------------------------------------------------------------------------
//
//  Member:
//      CZ192::SetSignAndFirstTwoDigits
//
//  Synopsis:
//      Sets this number value to the argument.
//
//------------------------------------------------------------------------------
void 
CZ192::SetSignAndFirstTwoDigits(
    Integer33 v                     // The value.
    )
{
    Assert(IsValidInteger33(v) && GetSize() > 1);

    ULARGE_INTEGER tab;

    if (v > 0.0)
    {
        tab.QuadPart = static_cast<ULONGLONG>(v);
        SetSign(SI_STRICTLY_POSITIVE);
    }
    else if (v < 0.0)
    {
        tab.QuadPart = static_cast<ULONGLONG>(- v);
        SetSign(SI_STRICTLY_NEGATIVE);
    }
    else
    {
        tab.QuadPart = 0;
        SetSign(SI_ZERO);
    }
    m_uDigits[0] = tab.LowPart;
    m_uDigits[1] = tab.HighPart;
}

