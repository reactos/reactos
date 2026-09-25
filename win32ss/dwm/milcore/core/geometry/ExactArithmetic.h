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
//      Fixed size signed integer classes declaration.
//
//  $ENDTAG
//
//  Classes:
//      CZBase
//      CZ64
//      CZ128
//      CZ192
//
//------------------------------------------------------------------------------

namespace RobustIntersections
{
    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CZBase
    //
    //  Synopsis:
    //      Base class for the signed integers modeled by the classes CZ64,
    //      CZ128, and CZ192. The base class holds the sign, the size, and a
    //      pointer to the array of digits of a derived class instance.
    //
    //  Notes:
    //      This class is an implementation class and should not be used outside
    //      of the RobustIntersections module.
    //
    //--------------------------------------------------------------------------
    class CZBase
    {
    public:

        // Constructor.
        CZBase(const UINT nSize)
            : m_nSize(nSize)
        {
            Assert(nSize == 3 || nSize == 5 || nSize == 7);
            m_eSign = SI_ZERO;
            m_puDigits = NULL;
        }

        // Returns this number sign.
        SIGNINDICATOR GetSign() const
        {
            return m_eSign;
        }

        // Negates this number.
        void Negate()
        {
            SetSign(GetOppositeSign());
        }

    protected:

        // Function members.

        // Returns the number of digits for this number.
        const UINT GetSize() const
        {
            return m_nSize;
        }

        // Sets the sign of this number.
        void SetSign(SIGNINDICATOR sign)
        {
            m_eSign = sign;
        }

        // Returns the opposite sign. 
        SIGNINDICATOR GetOppositeSign() const
        {
            return static_cast<SIGNINDICATOR>(- m_eSign);
        }

        // Sets the the base address of the digits array.
        void SetDigitsPointer(UINT* puDigits)
        {
            Assert(puDigits != NULL);
            m_puDigits = puDigits;
        }

        // Returns the base address of the digits array.
        const UINT* GetDigits() const
        {
            Assert(m_puDigits != NULL);
            return m_puDigits;
        }

        // Returns the number of significant digits.
        UINT GetDigitCount() const;

        // Sets this number to zero.
        void SetToZero()
        {
            SetSign(SI_ZERO);
            ZeroMemory(m_puDigits, GetSize() * sizeof(UINT));
        }

        // Copies the first count digits of digits into this number digits,
        // does not modify the sign of this number.
        void ReplaceDigitsButKeepSign(__in_ecount(count) const UINT *puDigits, __in_range(1,7) UINT count)
        {
            Assert(puDigits && count > 0 && count <= GetSize());
            CopyMemory(m_puDigits, puDigits, count * sizeof(UINT));
        }

    private:

        // Function members.

        // No public default constructor, copy constructor, and assignment operator.
        CZBase();
        CZBase(__in_ecount(1) const CZBase& source);
        CZBase& operator=(__in_ecount(1) const CZBase& source);

        // Data members.

        // Constant equal to the fixed number of digits in this number.
        const UINT m_nSize;

        // Sign
        SIGNINDICATOR m_eSign;

        // Base address of the digits array.
        // This member is set by the derived classes in their constructor.
        UINT* m_puDigits;
    };

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CZ64
    //
    //  Synopsis:
    //      A class instance models a signed integer in the range [-2^64 - 1,
    //      2^64 - 1].
    //
    //  Notes:
    //      This class is an implementation class and should not be used outside
    //      of the RobustIntersections module. The arithmetic operations assume
    //      that the operands and the result fit in a CZ64 instance.
    //
    //--------------------------------------------------------------------------
    class CZ64 : public CZBase
    {
    public:

        // Constructor.
        // The argument must be a valid Integer31.
        CZ64(Integer31 value);

        // Compares this number with other and returns the result.
        // This number is the right hand-side term in the comparison, 
        // that is, Compare returns (*this ? other).
        COMPARISON Compare(__in_ecount(1) const CZ64& other) const;

        // Multiplies this number by other and returns this number.
        // Restrictions apply.
        CZ64& Multiply(__in_ecount(1) const CZ64& other);

#if DBG
        // Debug dump.
        void Dump() const;
#endif

    private:

        // Function members.

        // No public default constructor, copy constructor, and assignment operator.
        CZ64();
        CZ64(__in_ecount(1) const CZ64& source);
        CZ64& operator=(__in_ecount(1) const CZ64& source);

        // Data members.

        // Digits. Each digit is a 32 bit unsigned integer.
        // The digits are ordered from the least significant to the most significant.
        UINT m_uDigits[3];
    };

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CZ128
    //
    //  Synopsis:
    //      A class instance models a signed integer in the range [-2^128 - 1,
    //      2^128 - 1].
    //
    //  Notes:
    //      This class is an implementation class and should not be used outside
    //      of the RobustIntersections module. The arithmetic operations assume
    //      that the operands and the result fit in a CZ128 instance.
    //
    //--------------------------------------------------------------------------
    class CZ128 : public CZBase
    {
    public:

        // Constructor.

        // The argument must be a valid Integer53.
        CZ128(Integer53 value);

        // Compares this number with other and returns the result.
        // This number is the first term in the comparison, that is, we return (*this ? other)
        COMPARISON Compare(__in_ecount(1) const CZ128& other) const;

        // Multiplies this number by other and returns this number.
        CZ128& Multiply(__in_ecount(1) const CZ128& other);

#if DBG
        // Debug dump.
        void Dump() const;
#endif

    private:

        // Function members.

        // Sets this number value to the argument
        void SetSignAndFirstTwoDigits(Integer53 v);

        // No public default constructor, copy constructor, and assignment operator.
        CZ128();
        CZ128(const CZ128& source);
        CZ128& operator=(__in_ecount(1) const CZ128& source);

        // Data members.

        // Digits. Each digit is a 32 bit unsigned integer.
        // The digits are ordered from the least significant to the most significant.
        UINT m_uDigits[5];
    };

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CZ192
    //
    //  Synopsis:
    //      A class instance models a signed integer in the range [-2^192 - 1,
    //      2^192 - 1].
    //
    //  Notes:
    //      This class is an implementation class and should not be used outside
    //      of the RobustIntersections module. The arithmetic operations assume
    //      that the operands and the result fit in a CZ192 instance.
    //
    //--------------------------------------------------------------------------
    class CZ192 : public CZBase
    {
    public:

        // Constructor.
        // The argument must be a valid Integer33.
        CZ192(Integer33 value);

        // Compares this number with other and returns the result.
        // This number is the first term in the comparison, that is, we return (*this ? other)
        COMPARISON Compare(__in_ecount(1) const CZ192& other) const;

        // Adds other to this number and returns this number.
        CZ192& Add(__in_ecount(1) const CZ192& other);

        // Subtracts other from this number and returns this number.
        CZ192& Subtract(__in_ecount(1) const CZ192& other);

        // Multiplies this number by other and returns this number.
        CZ192& Multiply(__in_ecount(1) const CZ192& other);

#if DBG
        // Debug dump.
        void Dump() const;
#endif

    private:

        // Function members.

        // Sets this number value to the argument
        void SetSignAndFirstTwoDigits(Integer33 v);

        // No public default constructor, copy constructor, and assignment operator.
        CZ192();
        CZ192(__in_ecount(1) const CZ192& source);
        CZ192& operator=(__in_ecount(1) const CZ192& source);

        // Data members.

        // Digits. Each digit is a 32 bit unsigned integer.
        // The digits are ordered from the least significant to the most significant.
        UINT m_uDigits[7];
    };
};

