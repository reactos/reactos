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
//      CIntegralInterval class declaration amd implementation.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

// Macros

// Mask for the sign bit.
#define SIGN_MASK 0x8000000000000000

// Numb mask. Equals the maximum signed 64-bit integer.
#define NUMB_MASK 0x7FFFFFFFFFFFFFFF

// Unused but potentially useful masks.

// Mask for the exponent.
// #define BEXP_MASK 0x7FF0000000000000

// Mask for the mantissa. 
// #define FRAC_MASK 0xFFFFFFFFFFFFF

// Bits for negative zero.
// 0x8000000000000000

namespace RobustIntersections
{
    // The next two functions return the smallest (largest) double strictly larger (smaller) 
    // than their argument.
    // This does not work for NaNs and QNaNs but does work for Normals and Denormals.
    // Note that NextDouble(0) and PreviousDouble(0) return a denormalized number.

    // One could think of using _nextafter instead of these functions but the current 
    // implementation is twice as slow.

    inline double NextDouble(double x)
    {
        unsigned __int64 u = *(reinterpret_cast<unsigned __int64*>(&x));

        if (u & SIGN_MASK)
        {
            // A negative number.
            if (u & NUMB_MASK)
            {
                u--;
            }
            else
            {
                u = 1;
            }
        }
        else
        {
            // A positive number.
            u++;
        }
        
        // The next line is much slower than the code above.
        // u = (u & SIGN_MASK) ? ((u & NUMB_MASK) ? --u : 1) : ++u;

        Assert(*(reinterpret_cast<double*>(&u)) == _nextafter(x, DBL_MAX));
        return *(reinterpret_cast<double*>(&u));
    }

    inline double PreviousDouble(double x)
    {
        unsigned __int64 u = *(reinterpret_cast<unsigned __int64*>(&x));

        if ((u & SIGN_MASK) == 0)
        {
            // A positive number.
            if (u & NUMB_MASK)
            {
                u--;
            }
            else
            {
                u = SIGN_MASK | 1;
            }
        }
        else
        {
            // A negative number.
            u++;
        }

        // The next line is much slower than the code above.
        // u = ((u & SIGN_MASK) == 0) ? ((u & NUMB_MASK) ? --u : (SIGN_MASK | 1)) :  ++u;

        Assert(*(reinterpret_cast<double*>(&u)) == _nextafter(x, - DBL_MAX)); 
        return *(reinterpret_cast<double*>(&u));
    }

    //+-------------------------------------------------------------------------
    //
    //  Class:
    //      CIntegralInterval
    //
    //  Synopsis:
    //      An integral interval is an interval whose bounds are integers
    //      represented by double precision floats. The interval is considered
    //      as closed.
    //
    //  Note:
    //      This class is an implementation class and should not be used outside
    //      of the RobustIntersections module.
    //
    //--------------------------------------------------------------------------
    class CIntegralInterval
    {
    public:

        // Default Constructor
        CIntegralInterval() : m_l(0), m_h(0) 
        {}

        // Constructor. Initializes this interval to the closed interval [v, v].
        CIntegralInterval(double v) : m_l(v), m_h(v) 
        {
            Assert(IsValid());
        }

        // Constructor. Initializes this interval to an interval containing the 
        // value of the determinant a*d - b*c.
        CIntegralInterval(double a, double b, double c, double d)
        {
            // We know that a, b, c, and d are exact.
            m_l = m_h = a * d;
            double l2 = b * c;
            double h2 = l2;
            if (abs(m_l) > LARGESTINTEGER53)
            {
                m_l = PreviousDouble(m_l);
                m_h = NextDouble(m_h);
            }
            if (abs(l2) > LARGESTINTEGER53)
            {
                l2 = PreviousDouble(l2);
                h2 = NextDouble(h2);
            }

            m_l -= h2;
            m_h -= l2;
            if (abs(m_l) > LARGESTINTEGER53)
            {
                m_l = PreviousDouble(m_l);
            }
            if (abs(m_h) > LARGESTINTEGER53)
            {
                m_h = NextDouble(m_h);
            }
            Assert(IsValid());
        }

        // The compiler generated copy constructor and assignemnt operator do the right thing.

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::GetSign
        //
        //  Synopsis:
        //      Gets the sign of this interval.
        //
        //  Returns:
        //      The sign of this interval, SI_STRICTLY_NEGATIVE iff both bounds
        //      are strictly negative, SI_ZERO iff the (closed) interval
        //      contains 0, and SI_STRICTLY_POSITIVE otherwise.
        //
        //----------------------------------------------------------------------
        SIGNINDICATOR GetSign() const
        {
            Assert(IsValid());
            return m_h < 0 ? SI_STRICTLY_NEGATIVE : (m_l > 0 ? SI_STRICTLY_POSITIVE : SI_ZERO); 
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Compare
        //
        //  Synopsis:
        //      Compares this interval with the argument and returns the result.
        //
        //  Returns:
        //      The comparison result.
        //
        //  Note:
        //      This interval is the right hand-side term in the comparison,
        //      that is, Compare returns (*this ? other).
        //
        //----------------------------------------------------------------------
        COMPARISON Compare(
            __in_ecount(1) const CIntegralInterval& other
                // The interval to compare with
            ) const
        {
            Assert(IsValid() && other.IsValid());
            COMPARISON result = C_UNDEFINED;
            if (Intersects(other))
            {
                if (Equals(other) && m_h == m_l)
                {
                    result = C_EQUAL;
                }
            }
            else
            {
                result = m_h < other.m_l ? C_STRICTLYLESSTHAN : C_STRICTLYGREATERTHAN;
            }
            return result;
        }
 
        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Add
        //
        //  Synopsis:
        //      Adds the argument to this interval.
        //
        //  Returns:
        //      This interval.
        //
        //----------------------------------------------------------------------
        CIntegralInterval& Add(
            __in_ecount(1) const CIntegralInterval& other
                // The interval to add.
            )
        {
            Assert(IsValid() && other.IsValid());

            m_l += other.m_l;
            m_h += other.m_h;
            if (abs(m_l) > LARGESTINTEGER53)
            {
                m_l = PreviousDouble(m_l);
            }
            if (abs(m_h) > LARGESTINTEGER53)
            {
                m_h = NextDouble(m_h);
            }
            Assert(IsValid());
            return *this;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Subtract
        //
        //  Synopsis:
        //      Subtracts the argument from this interval.
        //
        //  Returns:
        //      This interval.
        //
        //----------------------------------------------------------------------
        CIntegralInterval& Subtract(
            __in_ecount(1) const CIntegralInterval& other
                // The interval to subtract.
            )
        {
            Assert(IsValid() && other.IsValid());
            m_l -= other.m_h;
            m_h -= other.m_l;
            if (abs(m_l) > LARGESTINTEGER53)
            {
                m_l = PreviousDouble(m_l);
            }
            if (abs(m_h) > LARGESTINTEGER53)
            {
                m_h = NextDouble(m_h);
            }
            Assert(IsValid());
            return *this;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Multiply
        //
        //  Synopsis:
        //      Multiplies this interval by the argument.
        //
        //  Returns:
        //      This interval.
        //
        //----------------------------------------------------------------------
        CIntegralInterval& Multiply(
            __in_ecount(1) const CIntegralInterval& other
                // The multiplier.
            )
        {
            Assert(IsValid() && other.IsValid());

            // Easy case first.
            if (IsZero() || other.IsZero())
            {
                m_l = m_h = 0;
            }
            else
            {
                // Out of the remining nine cases only the last one needs four multiplies.
                if (m_l >= 0)
                {
                    Assert(m_h > 0);
                    if (other.m_l >= 0)
                    {
                        Assert(other.m_h > 0);
                        m_l *= other.m_l;
                        m_h *= other.m_h;

                    }
                    else if (other.m_h <= 0)
                    {
                        Assert(other.m_l < 0);
                        double temp = m_h * other.m_l;
                        m_h = m_l * other.m_h;
                        m_l = temp;
                    }
                    else
                    {
                        Assert(other.m_l < 0 && other.m_h > 0);
                        m_l = m_h * other.m_l;
                        m_h *= other.m_h;
                    }
                }
                else if (m_h <= 0)
                {
                    Assert(m_l < 0);
                    if (other.m_l >= 0)
                    {
                        Assert(other.m_h > 0);
                        m_l *= other.m_h;
                        m_h *= other.m_l;
                    }
                    else if (other.m_h <= 0)
                    {
                        Assert(other.m_l < 0);
                        double temp = m_h * other.m_h;
                        m_h = m_l * other.m_l;
                        m_l = temp;
                    }
                    else
                    {
                        Assert(other.m_l < 0 && other.m_h > 0);
                        m_h = m_l * other.m_l;
                        m_l *= other.m_h;
                    }
                }
                else
                {
                    // This interval contains 0 but is not [0, 0]
                    Assert(m_l < 0 && m_h > 0);
                    if (other.m_l >= 0)
                    {
                        Assert(other.m_h > 0);
                        m_l *= other.m_h;
                        m_h *= other.m_h;
                    }
                    else if (other.m_h <= 0)
                    {
                        Assert(other.m_l < 0);
                        double temp = m_h * other.m_l;
                        m_h = m_l * other.m_l;
                        m_l = temp;
                    }
                    else
                    {
                        // Both intervals contain zero and are not equal to [0, 0].
                        Assert(other.m_l < 0 && other.m_h > 0);

                        // IEEE 754 preserves order, that is, if a > b then a * c >= b * c for c > 0.
                        // The equality might be due to rounding.
                        double minmin = m_l * other.m_l;
                        double minmax = m_l * other.m_h;
                        double maxmin = m_h * other.m_l;
                        double maxmax = m_h * other.m_h;
                        m_l = min(minmax, maxmin);
                        m_h = max(minmin, maxmax);
                    }
                }

                if (abs(m_l) > LARGESTINTEGER53)
                {
                    m_l = PreviousDouble(m_l);
                }
                if (abs(m_h) > LARGESTINTEGER53)
                {
                    m_h = NextDouble(m_h);
                }
            }
            Assert(IsValid());
            return *this;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::IsValid
        //
        //  Synopsis:
        //      Validity check
        //
        //  Returns:
        //      true when valid and otherwise false.
        //
        //----------------------------------------------------------------------
        bool IsValid() const 
        {
            int cl = _fpclass(m_l);
            int ch = _fpclass(m_h);

            return  (cl == _FPCLASS_NN || cl == _FPCLASS_NZ ||
                     cl == _FPCLASS_PZ || cl == _FPCLASS_PN) &&
                    (ch == _FPCLASS_NN || ch == _FPCLASS_NZ || 
                     ch == _FPCLASS_PZ || ch == _FPCLASS_PN) &&
                    m_l == floor(m_l) && m_h == floor(m_h) && m_l <= m_h;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Equals
        //
        //  Synopsis:
        //      Equality test.
        //
        //  Returns:
        //      true when this interval is equal to the argument and otherwise
        //      false.
        //
        //----------------------------------------------------------------------
        bool Equals(
            __in_ecount(1) const CIntegralInterval& other
                // The interval to compare with.
            ) const
        {
            Assert(IsValid() && other.IsValid());
            return m_l == other.m_l && m_h == other.m_h;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Intersects
        //
        //  Synopsis:
        //      Intersection test.
        //
        //  Returns:
        //      true when this interval intersects the argument and otherwise
        //      false.
        //
        //----------------------------------------------------------------------
        bool Intersects(
            __in_ecount(1) const CIntegralInterval& other
                // The other interval.
            ) const
        {
            Assert(IsValid() && other.IsValid());
            return other.m_l <= m_h && other.m_h >= m_l;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Contains
        //
        //  Synopsis:
        //      Inclusion test.
        //
        //  Returns:
        //      true when this interval contains the argument and otherwise
        //      false.
        //
        //----------------------------------------------------------------------
        bool Contains(
            __in_ecount(1) const CIntegralInterval& other
                // The interval to compare with.
            ) const
        {
            Assert(IsValid() && other.IsValid());
            return m_l <= other.m_l && m_h >= other.m_h;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::Contains
        //
        //  Synopsis:
        //      Inclusion test.
        //
        //  Returns:
        //      true when this interval contains the argument and otherwise
        //      false.
        //
        //----------------------------------------------------------------------
        bool Contains(double v) const
        {
            Assert(IsValid());
            return m_l <= v && m_h >= v;
        }

        //+---------------------------------------------------------------------
        //
        //  Member:
        //      CIntegralInterval::IsZero
        //
        //  Synopsis:
        //      Test for equality with [0, 0].
        //
        //  Returns:
        //      true when this interval equals [0, 0] and otherwise false.
        //
        //----------------------------------------------------------------------
        bool IsZero() const
        {
            Assert(IsValid());
            return m_l == 0.0 && m_h == 0.0;
        }

    private:

        // Data members.

        // Low and high bounds. The interval is considered as closed.
        double m_l, m_h;
    };
}


