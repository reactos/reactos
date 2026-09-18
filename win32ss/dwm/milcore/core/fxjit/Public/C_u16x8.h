// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 8 16-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u16x8
//
//  Synopsis:
//      Represents in-memory value of C_u16x8.
//
//------------------------------------------------------------------------------
struct u16x8
{
    UINT16 words[8];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u16x8
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//      Single 128-bit value is treated as an array of 8 16-bit words.
//
//
//------------------------------------------------------------------------------
class C_u16x8 : public C_XmmValue
{
public:
    C_u16x8() {}

    C_u16x8(C_u16x8 const & origin);
    C_u16x8& operator=(C_u16x8 const& origin);

    C_u16x8(u16x8 const & origin);
    C_u16x8& operator=(u16x8 const& origin);

    C_u16x8 operator+ (C_u16x8 const& other) const { return BinaryOperation(otXmmWordsAdd, other); }
    C_u16x8 operator- (C_u16x8 const& other) const { return BinaryOperation(otXmmWordsSub, other); }
    C_u16x8 operator* (C_u16x8 const& other) const { return BinaryOperation(otXmmWordsMul, other); }
    C_u16x8 operator==(C_u16x8 const& other) const { return BinaryOperation(otXmmWordsEqual, other); }
    C_u16x8 operator& (C_u16x8 const& other) const { return BinaryOperation(otXmmIntAnd, other); }
    C_u16x8 operator| (C_u16x8 const& other) const { return BinaryOperation(otXmmIntOr , other); }
    C_u16x8 operator^ (C_u16x8 const& other) const { return BinaryOperation(otXmmIntXor, other); }

    C_u16x8 AddSaturate(C_u16x8 const& other) const { return BinaryOperation(otXmmWordsAddSat, other); }
    C_u16x8 SubSaturate(C_u16x8 const& other) const { return BinaryOperation(otXmmWordsSubSat, other); }

    C_u32x4 MulAdd        (C_u16x8 const &other) const { return BinaryOperation(otXmmWordsMulAdd        , other); }
    C_u32x4 InterleaveLow (C_u16x8 const &other) const { return BinaryOperation(otXmmWordsInterleaveLow , other); }
    C_u32x4 InterleaveHigh(C_u16x8 const &other) const { return BinaryOperation(otXmmWordsInterleaveHigh, other); }
    C_u8x16 PackSignedSaturate(C_u16x8 const &other) const;
    C_u8x16 PackUnsignedSaturate(C_u16x8 const &other) const;

    C_u16x8 operator+(u16x8 const& src) const { return BinaryOperation(otXmmWordsAdd, src); }
    C_u16x8 operator-(u16x8 const& src) const { return BinaryOperation(otXmmWordsSub, src); }
    C_u16x8 operator*(u16x8 const& src) const { return BinaryOperation(otXmmWordsMul, src); }
    C_u16x8 operator&(u16x8 const& src) const { return BinaryOperation(otXmmIntAnd, src); }
    C_u16x8 operator|(u16x8 const& src) const { return BinaryOperation(otXmmIntOr , src); }
    C_u16x8 operator^(u16x8 const& src) const { return BinaryOperation(otXmmIntXor, src); }

    C_u16x8 AddSaturate(u16x8 const& src) const { return BinaryOperation(otXmmWordsAddSat, src); }
    C_u16x8 SubSaturate(u16x8 const& src) const { return BinaryOperation(otXmmWordsSubSat, src); }

    C_u32x4 MulAdd        (u16x8 const &src) const { return BinaryOperation(otXmmWordsMulAdd        , src); }
    C_u32x4 InterleaveLow (u16x8 const &src) const { return BinaryOperation(otXmmWordsInterleaveLow , src); }
    C_u32x4 InterleaveHigh(u16x8 const &src) const { return BinaryOperation(otXmmWordsInterleaveHigh, src); }
    C_u8x16 PackSignedSaturate(u16x8 const &src) const;
    C_u8x16 PackUnsignedSaturate(u16x8 const &src) const;

    C_u16x8& operator+=(C_u16x8 const& other) { return BinaryAssignment(otXmmWordsAdd, other); }
    C_u16x8& operator-=(C_u16x8 const& other) { return BinaryAssignment(otXmmWordsSub, other); }
    C_u16x8& operator*=(C_u16x8 const& other) { return BinaryAssignment(otXmmWordsMul, other); }
    C_u16x8& operator&=(C_u16x8 const& other) { return BinaryAssignment(otXmmIntAnd, other); }
    C_u16x8& operator|=(C_u16x8 const& other) { return BinaryAssignment(otXmmIntOr , other); }
    C_u16x8& operator^=(C_u16x8 const& other) { return BinaryAssignment(otXmmIntXor, other); }

    C_u16x8& operator+=(u16x8 const& src) { return BinaryAssignment(otXmmWordsAdd, src); }
    C_u16x8& operator-=(u16x8 const& src) { return BinaryAssignment(otXmmWordsSub, src); }
    C_u16x8& operator*=(u16x8 const& src) { return BinaryAssignment(otXmmWordsMul, src); }
    C_u16x8& operator&=(u16x8 const& src) { return BinaryAssignment(otXmmIntAnd, src); }
    C_u16x8& operator|=(u16x8 const& src) { return BinaryAssignment(otXmmIntOr , src); }
    C_u16x8& operator^=(u16x8 const& src) { return BinaryAssignment(otXmmIntXor, src); }

    C_u16x8 operator+(R_u16x8 const& ref) const { return BinaryReference(otXmmWordsAdd, ref); }
    C_u16x8 operator-(R_u16x8 const& ref) const { return BinaryReference(otXmmWordsSub, ref); }
    C_u16x8 operator&(R_u16x8 const& ref) const { return BinaryReference(otXmmIntAnd, ref); }
    C_u16x8 operator|(R_u16x8 const& ref) const { return BinaryReference(otXmmIntOr , ref); }
    C_u16x8 operator^(R_u16x8 const& ref) const { return BinaryReference(otXmmIntXor, ref); }

    C_u16x8 operator<<(int shift) const;
    C_u16x8& operator<<=(int shift);

    C_u16x8 operator>>(int shift) const;
    C_u16x8& operator>>=(int shift);

    C_u16x8 ShuffleLow(UINT8 uOrder) const;
    C_u16x8 ShuffleHigh(UINT8 uOrder) const;
    C_u16x8 ReplicateLow(UINT8 uWordIndex) const { return ShuffleLow((uWordIndex & 3)*0x55); }
    C_u16x8 ReplicateHigh(UINT8 uWordIndex) const { return ShuffleHigh((uWordIndex & 3)*0x55); }

    C_u32x4 UnpackToDWords() const;

private:
    C_u16x8 BinaryOperation(OpType ot, C_u16x8 const& other) const;
    C_u16x8 BinaryOperation(OpType ot, u16x8 const& src) const;
    C_u16x8& BinaryAssignment(OpType ot, C_u16x8 const& other);
    C_u16x8& BinaryAssignment(OpType ot, u16x8 const& src);
    C_u16x8 BinaryReference(OpType ot, R_u16x8 const& ref) const;
};

inline C_u16x8 C_u32x4::PackSignedSaturate(C_u32x4 const &other) const { return BinaryOperation(otXmmDWordsPackSS, other); }
inline C_u16x8 C_u32x4::PackSignedSaturate(u32x4 const &src) const { return BinaryOperation(otXmmDWordsPackSS, src); }

