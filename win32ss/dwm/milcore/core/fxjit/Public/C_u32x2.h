// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 2 32-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u32x2
//
//  Synopsis:
//      Represents in-memory value of C_u32x2.
//
//------------------------------------------------------------------------------
struct u32x2
{
    UINT32 data[2];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u32x2
//
//  Synopsis:
//      Represents integer 64-bit variable in prototype program.
//      Single 64-bit value is treated as an array of 2 32-bit words.
//
//
//------------------------------------------------------------------------------
class C_u32x2 : public C_MmValue
{
public:
    C_u32x2() {}

    C_u32x2(C_u32x2 const & origin);
    C_u32x2& operator=(C_u32x2 const& origin);

    C_u32x2(C_u32 const & origin);
    C_u32x2& operator=(C_u32 const& origin);

    C_u32x2(u32x2 const & origin);
    C_u32x2& operator=(u32x2 const& origin);

    C_u32x2(UINT32 const & origin);
    C_u32x2& operator=(UINT32 const& origin);

    C_u32x2 operator+ (C_u32x2 const& other) const { return BinaryOperation(otMmDWordsAdd, other); }
    C_u32x2 operator- (C_u32x2 const& other) const { return BinaryOperation(otMmDWordsSub, other); }
    C_u32x2 operator==(C_u32x2 const& other) const { return BinaryOperation(otMmDWordsEqual, other); }
    C_u32x2 operator> (C_u32x2 const& other) const { return BinaryOperation(otMmDWordsGreater, other); }
    C_u32x2 operator& (C_u32x2 const& other) const { return BinaryOperation(otMmQWordAnd, other); }
    C_u32x2 operator| (C_u32x2 const& other) const { return BinaryOperation(otMmQWordOr , other); }
    C_u32x2 operator^ (C_u32x2 const& other) const { return BinaryOperation(otMmQWordXor, other); }

    C_u64x1 InterleaveLow (C_u32x2 const& other) const { return BinaryOperation(otMmDWordsInterleaveLow , other); }
    C_u64x1 InterleaveHigh(C_u32x2 const& other) const { return BinaryOperation(otMmDWordsInterleaveHigh, other); }
    C_u16x4 PackSignedSaturate(C_u32x2 const &other) const;

    C_u32x2 operator+(u32x2 const& src) const { return BinaryOperation(otMmDWordsAdd, src); }
    C_u32x2 operator-(u32x2 const& src) const { return BinaryOperation(otMmDWordsSub, src); }
    C_u32x2 operator&(u32x2 const& src) const { return BinaryOperation(otMmQWordAnd, src); }
    C_u32x2 operator|(u32x2 const& src) const { return BinaryOperation(otMmQWordOr , src); }
    C_u32x2 operator^(u32x2 const& src) const { return BinaryOperation(otMmQWordXor, src); }
    C_u32x2 operator>(u32x2 const& src) const { return BinaryOperation(otMmDWordsGreater, src); }

    C_u64x1 InterleaveLow (u32x2 const& src) const { return BinaryOperation(otMmDWordsInterleaveLow , src); }
    C_u64x1 InterleaveHigh(u32x2 const& src) const { return BinaryOperation(otMmDWordsInterleaveHigh, src); }
    C_u16x4 PackSignedSaturate(u32x2 const &src) const;

    C_u32x2& operator+=(C_u32x2 const& other) { return BinaryAssignment(otMmDWordsAdd, other); }
    C_u32x2& operator-=(C_u32x2 const& other) { return BinaryAssignment(otMmDWordsSub, other); }
    C_u32x2& operator&=(C_u32x2 const& other) { return BinaryAssignment(otMmQWordAnd, other); }
    C_u32x2& operator|=(C_u32x2 const& other) { return BinaryAssignment(otMmQWordOr , other); }
    C_u32x2& operator^=(C_u32x2 const& other) { return BinaryAssignment(otMmQWordXor, other); }

    C_u32x2& operator+=(u32x2 const& src) { return BinaryAssignment(otMmDWordsAdd, src); }
    C_u32x2& operator-=(u32x2 const& src) { return BinaryAssignment(otMmDWordsSub, src); }
    C_u32x2& operator&=(u32x2 const& src) { return BinaryAssignment(otMmQWordAnd, src); }
    C_u32x2& operator|=(u32x2 const& src) { return BinaryAssignment(otMmQWordOr , src); }
    C_u32x2& operator^=(u32x2 const& src) { return BinaryAssignment(otMmQWordXor, src); }

    C_u32x2 operator+(R_u32x2 const& ref) const { return BinaryReference(otMmDWordsAdd, ref); }
    C_u32x2 operator-(R_u32x2 const& ref) const { return BinaryReference(otMmDWordsSub, ref); }
    C_u32x2 operator&(R_u32x2 const& ref) const { return BinaryReference(otMmQWordAnd, ref); }
    C_u32x2 operator|(R_u32x2 const& ref) const { return BinaryReference(otMmQWordOr , ref); }
    C_u32x2 operator^(R_u32x2 const& ref) const { return BinaryReference(otMmQWordXor, ref); }
    C_u32x2 operator>(R_u32x2 const& ref) const { return BinaryReference(otMmDWordsGreater, ref); }

    C_u32x2 operator<<(C_u32x2 const& other) const;
    C_u32x2 operator<<(int shift) const;
    C_u32x2& operator<<=(int shift);

    C_u32x2 operator>>(int shift) const;
    C_u32x2& operator>>=(int shift);

    C_MmValue operator~() const;

    C_u32x2 operator<(C_u32x2 const& other) const {return other > *this;}
    C_u32x2 operator<=(C_u32x2 const& other) const {return ~(*this > other);}
    C_u32x2 operator>=(C_u32x2 const& other) const {return ~(other > *this);}

private:
    C_u32x2 BinaryOperation(OpType ot, C_u32x2 const& other) const;
    C_u32x2 BinaryOperation(OpType ot, u32x2 const& src) const;
    C_u32x2& BinaryAssignment(OpType ot, C_u32x2 const& other);
    C_u32x2& BinaryAssignment(OpType ot, u32x2 const& src);
    C_u32x2 BinaryReference(OpType ot, R_u32x2 const& ref) const;
};



