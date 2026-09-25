// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 4 16-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u16x4
//
//  Synopsis:
//      Represents in-memory value of C_u16x4.
//
//------------------------------------------------------------------------------
struct u16x4
{
    UINT16 data[4];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u16x4
//
//  Synopsis:
//      Represents integer 64-bit variable in prototype program.
//      Single 64-bit value is treated as an array of 4 16-bit words.
//
//
//------------------------------------------------------------------------------
class C_u16x4 : public C_MmValue
{
public:
    C_u16x4() {}

    C_u16x4(C_u16x4 const & origin);
    C_u16x4& operator=(C_u16x4 const& origin);

    C_u16x4(u16x4 const & origin);
    C_u16x4& operator=(u16x4 const& origin);

    C_u16x4 operator+ (C_u16x4 const& other) const { return BinaryOperation(otMmWordsAdd, other); }
    C_u16x4 operator- (C_u16x4 const& other) const { return BinaryOperation(otMmWordsSub, other); }
    C_u16x4 operator==(C_u16x4 const& other) const { return BinaryOperation(otMmWordsEqual, other); }
    C_u16x4 operator& (C_u16x4 const& other) const { return BinaryOperation(otMmQWordAnd, other); }
    C_u16x4 operator| (C_u16x4 const& other) const { return BinaryOperation(otMmQWordOr , other); }
    C_u16x4 operator^ (C_u16x4 const& other) const { return BinaryOperation(otMmQWordXor, other); }

    C_u16x4 AddSaturate(C_u16x4 const& other) const { return BinaryOperation(otMmWordsAddSat, other); }
    C_u16x4 SubSaturate(C_u16x4 const& other) const { return BinaryOperation(otMmWordsSubSat, other); }

    C_u32x2 MulAdd        (C_u16x4 const &other) const { return BinaryOperation(otMmWordsMulAdd        , other); }
    C_u32x2 InterleaveLow (C_u16x4 const &other) const { return BinaryOperation(otMmWordsInterleaveLow , other); }
    C_u32x2 InterleaveHigh(C_u16x4 const &other) const { return BinaryOperation(otMmWordsInterleaveHigh, other); }
    C_u8x8 PackSignedSaturate(C_u16x4 const &other) const;
    C_u8x8 PackUnsignedSaturate(C_u16x4 const &other) const;

    C_u16x4 operator+(u16x4 const& src) const { return BinaryOperation(otMmWordsAdd, src); }
    C_u16x4 operator-(u16x4 const& src) const { return BinaryOperation(otMmWordsSub, src); }
    C_u16x4 operator&(u16x4 const& src) const { return BinaryOperation(otMmQWordAnd, src); }
    C_u16x4 operator|(u16x4 const& src) const { return BinaryOperation(otMmQWordOr , src); }
    C_u16x4 operator^(u16x4 const& src) const { return BinaryOperation(otMmQWordXor, src); }

    C_u16x4 AddSaturate(u16x4 const& src) const { return BinaryOperation(otMmWordsAddSat, src); }
    C_u16x4 SubSaturate(u16x4 const& src) const { return BinaryOperation(otMmWordsSubSat, src); }

    C_u32x2 MulAdd        (u16x4 const &src) const { return BinaryOperation(otMmWordsMulAdd        , src); }
    C_u32x2 InterleaveLow (u16x4 const &src) const { return BinaryOperation(otMmWordsInterleaveLow , src); }
    C_u32x2 InterleaveHigh(u16x4 const &src) const { return BinaryOperation(otMmWordsInterleaveHigh, src); }
    C_u8x8  PackSignedSaturate(u16x4 const &src) const;
    C_u8x8  PackUnsignedSaturate(u16x4 const &src) const;

    C_u16x4& operator+=(C_u16x4 const& other) { return BinaryAssignment(otMmWordsAdd, other); }
    C_u16x4& operator-=(C_u16x4 const& other) { return BinaryAssignment(otMmWordsSub, other); }
    C_u16x4& operator&=(C_u16x4 const& other) { return BinaryAssignment(otMmQWordAnd, other); }
    C_u16x4& operator|=(C_u16x4 const& other) { return BinaryAssignment(otMmQWordOr , other); }
    C_u16x4& operator^=(C_u16x4 const& other) { return BinaryAssignment(otMmQWordXor, other); }

    C_u16x4& operator+=(u16x4 const& src) { return BinaryAssignment(otMmWordsAdd, src); }
    C_u16x4& operator-=(u16x4 const& src) { return BinaryAssignment(otMmWordsSub, src); }
    C_u16x4& operator&=(u16x4 const& src) { return BinaryAssignment(otMmQWordAnd, src); }
    C_u16x4& operator|=(u16x4 const& src) { return BinaryAssignment(otMmQWordOr , src); }
    C_u16x4& operator^=(u16x4 const& src) { return BinaryAssignment(otMmQWordXor, src); }

    C_u16x4 operator+(R_u16x4 const& ref) const { return BinaryReference(otMmWordsAdd, ref); }
    C_u16x4 operator-(R_u16x4 const& ref) const { return BinaryReference(otMmWordsSub, ref); }
    C_u16x4 operator&(R_u16x4 const& ref) const { return BinaryReference(otMmQWordAnd, ref); }
    C_u16x4 operator|(R_u16x4 const& ref) const { return BinaryReference(otMmQWordOr , ref); }
    C_u16x4 operator^(R_u16x4 const& ref) const { return BinaryReference(otMmQWordXor, ref); }

    C_u16x4 operator*(C_u16x4 const& other) const;
    C_u16x4& operator*=(C_u16x4 const& other);

    C_u16x4 operator<<(int shift) const;
    C_u16x4& operator<<=(int shift);

    C_u16x4 operator>>(int shift) const;
    C_u16x4& operator>>=(int shift);

private:
    C_u16x4 BinaryOperation(OpType ot, C_u16x4 const& other) const;
    C_u16x4 BinaryOperation(OpType ot, u16x4 const& src) const;
    C_u16x4& BinaryAssignment(OpType ot, C_u16x4 const& other);
    C_u16x4& BinaryAssignment(OpType ot, u16x4 const& src);
    C_u16x4 BinaryReference(OpType ot, R_u16x4 const& ref) const;
};

inline C_u16x4 C_u32x2::PackSignedSaturate(C_u32x2 const &other) const { return BinaryOperation(otMmDWordsPackSS, other); }
inline C_u16x4 C_u32x2::PackSignedSaturate(u32x2 const &src) const { return BinaryOperation(otMmDWordsPackSS, src); }

