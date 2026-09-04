// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 4 signed integer 32-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      s32x4
//
//  Synopsis:
//      Represents in-memory value of C_s32x4.
//
//------------------------------------------------------------------------------
struct s32x4
{
    UINT32 data[4];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_s32x4
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//      Single 128-bit value is treated as an array of 4 signed 32-bit words.
//
//
//------------------------------------------------------------------------------
class C_s32x4 : public C_u32x4
{
public:
    C_s32x4() {}

    C_s32x4(C_s32x4 const & origin);
    C_s32x4& operator=(C_s32x4 const& origin);

    C_s32x4(C_u32 const & origin);
    C_s32x4& operator=(C_u32 const& origin);

    C_s32x4(s32x4 const & origin);
    C_s32x4& operator=(s32x4 const& origin);

    C_s32x4 operator>(C_s32x4 const& other) const { return BinaryOperation(otXmmDWordsGreater, other); }
    C_s32x4 operator>(s32x4 const& src) const { return BinaryOperation(otXmmDWordsGreater, src); }
    C_s32x4 operator>(R_s32x4 const& ref) const { return BinaryReference(otXmmDWordsGreater, ref); }

    C_s32x4 operator<(C_s32x4 const& other) const {return other > *this;}
    C_s32x4 operator<=(C_s32x4 const& other) const {return ~(*this > other);}
    C_s32x4 operator>=(C_s32x4 const& other) const {return ~(other > *this);}

    C_s32x4 Max(C_s32x4 const& other) const;
    C_s32x4 Min(C_s32x4 const& other) const;

    C_s32x4 operator>>(int shift) const;
    C_s32x4& operator>>=(int shift);
    C_s32x4 operator<<(int shift) const { return (C_s32x4) C_u32x4::operator <<(shift); }
    C_s32x4& operator<<=(int shift) { return (C_s32x4&) C_u32x4::operator <<(shift); }

    C_u32 ExtractSignBits() const;

private:
    C_s32x4 BinaryOperation(OpType ot, C_s32x4 const& other) const
    {
        return (C_s32x4)C_u32x4::BinaryOperation(ot, other);
    }
    C_s32x4 BinaryOperation(OpType ot, s32x4 const& src) const
    {
        return (C_s32x4)C_u32x4::BinaryOperation(ot, (u32x4 const&)src);
    }
    C_s32x4& BinaryAssignment(OpType ot, C_s32x4 const& other);
    C_s32x4& BinaryAssignment(OpType ot, s32x4 const& src);
    C_s32x4 BinaryReference(OpType ot, R_s32x4 const& ref) const;
};



