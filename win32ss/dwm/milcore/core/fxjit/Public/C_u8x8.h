// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on a vector of 8 8-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u8x8
//
//  Synopsis:
//      Represents in-memory value of C_u8x8.
//
//------------------------------------------------------------------------------
struct u8x8
{
    UINT8 data[8];
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u8x8
//
//  Synopsis:
//      Represents integer 64-bit variable in prototype program.
//      Single 64-bit value is treated as an array of 8 bytes.
//
//
//------------------------------------------------------------------------------
class C_u8x8 : public C_MmValue
{
public:
    C_u8x8() {}

    C_u8x8(C_u8x8 const & origin);
    C_u8x8& operator=(C_u8x8 const& origin);

    C_u8x8(u8x8 const & origin);
    C_u8x8& operator=(u8x8 const& origin);

    C_u8x8 operator+ (C_u8x8 const& other) const { return BinaryOperation(otMmBytesAdd, other); }
    C_u8x8 operator- (C_u8x8 const& other) const { return BinaryOperation(otMmBytesSub, other); }
    C_u8x8 operator==(C_u8x8 const& other) const { return BinaryOperation(otMmBytesEqual, other); }
    C_u8x8 operator& (C_u8x8 const& other) const { return BinaryOperation(otMmQWordAnd, other); }
    C_u8x8 operator| (C_u8x8 const& other) const { return BinaryOperation(otMmQWordOr , other); }
    C_u8x8 operator^ (C_u8x8 const& other) const { return BinaryOperation(otMmQWordXor, other); }

    C_u16x4 InterleaveLow (C_u8x8 const &other) const { return BinaryOperation(otMmBytesInterleaveLow , other); }
    C_u16x4 InterleaveHigh(C_u8x8 const &other) const { return BinaryOperation(otMmBytesInterleaveHigh, other); }

    C_u8x8 operator+(u8x8 const& src) const { return BinaryOperation(otMmBytesAdd, src); }
    C_u8x8 operator-(u8x8 const& src) const { return BinaryOperation(otMmBytesSub, src); }
    C_u8x8 operator&(u8x8 const& src) const { return BinaryOperation(otMmQWordAnd, src); }
    C_u8x8 operator|(u8x8 const& src) const { return BinaryOperation(otMmQWordOr , src); }
    C_u8x8 operator^(u8x8 const& src) const { return BinaryOperation(otMmQWordXor, src); }

    C_u16x4 InterleaveLow (u8x8 const &src) const { return BinaryOperation(otMmBytesInterleaveLow , src); }
    C_u16x4 InterleaveHigh(u8x8 const &src) const { return BinaryOperation(otMmBytesInterleaveHigh, src); }

    C_u8x8& operator+=(C_u8x8 const& other) { return BinaryAssignment(otMmBytesAdd, other); }
    C_u8x8& operator-=(C_u8x8 const& other) { return BinaryAssignment(otMmBytesSub, other); }
    C_u8x8& operator&=(C_u8x8 const& other) { return BinaryAssignment(otMmQWordAnd, other); }
    C_u8x8& operator|=(C_u8x8 const& other) { return BinaryAssignment(otMmQWordOr , other); }
    C_u8x8& operator^=(C_u8x8 const& other) { return BinaryAssignment(otMmQWordXor, other); }

    C_u8x8& operator+=(u8x8 const& src) { return BinaryAssignment(otMmBytesAdd, src); }
    C_u8x8& operator-=(u8x8 const& src) { return BinaryAssignment(otMmBytesSub, src); }
    C_u8x8& operator&=(u8x8 const& src) { return BinaryAssignment(otMmQWordAnd, src); }
    C_u8x8& operator|=(u8x8 const& src) { return BinaryAssignment(otMmQWordOr , src); }
    C_u8x8& operator^=(u8x8 const& src) { return BinaryAssignment(otMmQWordXor, src); }

    C_u8x8 operator+(R_u8x8 const& ref) const { return BinaryReference(otMmBytesAdd, ref); }
    C_u8x8 operator-(R_u8x8 const& ref) const { return BinaryReference(otMmBytesSub, ref); }
    C_u8x8 operator&(R_u8x8 const& ref) const { return BinaryReference(otMmQWordAnd, ref); }
    C_u8x8 operator|(R_u8x8 const& ref) const { return BinaryReference(otMmQWordOr , ref); }
    C_u8x8 operator^(R_u8x8 const& ref) const { return BinaryReference(otMmQWordXor, ref); }

private:
    C_u8x8 BinaryOperation(OpType ot, C_u8x8 const& other) const;
    C_u8x8 BinaryOperation(OpType ot, u8x8 const& src) const;
    C_u8x8& BinaryAssignment(OpType ot, C_u8x8 const& other);
    C_u8x8& BinaryAssignment(OpType ot, u8x8 const& src);
    C_u8x8 BinaryReference(OpType ot, R_u8x8 const& ref) const;
};


inline C_u8x8 C_u16x4::PackSignedSaturate(C_u16x4 const &other) const { return BinaryOperation(otMmWordsPackSS, other); }
inline C_u8x8 C_u16x4::PackUnsignedSaturate(C_u16x4 const &other) const { return BinaryOperation(otMmWordsPackUS, other); }
inline C_u8x8 C_u16x4::PackSignedSaturate(u16x4 const &src) const { return BinaryOperation(otMmWordsPackSS, src); }
inline C_u8x8 C_u16x4::PackUnsignedSaturate(u16x4 const &src) const { return BinaryOperation(otMmWordsPackUS, src); }

