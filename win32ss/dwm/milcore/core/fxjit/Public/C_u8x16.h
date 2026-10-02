// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on a vector of 16 8-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u8x16
//
//  Synopsis:
//      Represents in-memory value of C_u8x16.
//
//------------------------------------------------------------------------------
struct u8x16
{
    UINT8 data[16];
};


//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u8x16
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//      Single 128-bit value is treated as an array of 16 bytes.
//
//
//------------------------------------------------------------------------------
class C_u8x16 : public C_XmmValue
{
public:
    C_u8x16() {}

    C_u8x16(C_u8x16 const & origin);
    C_u8x16& operator=(C_u8x16 const& origin);

    C_u8x16(u8x16 const & origin);
    C_u8x16& operator=(u8x16 const& origin);

    C_u8x16 operator+ (C_u8x16 const& other) const { return BinaryOperation(otXmmBytesAdd, other); }
    C_u8x16 operator- (C_u8x16 const& other) const { return BinaryOperation(otXmmBytesSub, other); }
    C_u8x16 operator==(C_u8x16 const& other) const { return BinaryOperation(otXmmBytesEqual, other); }
    C_u8x16 operator& (C_u8x16 const& other) const { return BinaryOperation(otXmmIntAnd, other); }
    C_u8x16 operator| (C_u8x16 const& other) const { return BinaryOperation(otXmmIntOr , other); }
    C_u8x16 operator^ (C_u8x16 const& other) const { return BinaryOperation(otXmmIntXor, other); }

    C_u16x8 InterleaveLow (C_u8x16 const &other) const { return BinaryOperation(otXmmBytesInterleaveLow , other); }
    C_u16x8 InterleaveHigh(C_u8x16 const &other) const { return BinaryOperation(otXmmBytesInterleaveHigh, other); }

    C_u8x16 operator+(u8x16 const& src) const { return BinaryOperation(otXmmBytesAdd, src); }
    C_u8x16 operator-(u8x16 const& src) const { return BinaryOperation(otXmmBytesSub, src); }
    C_u8x16 operator&(u8x16 const& src) const { return BinaryOperation(otXmmIntAnd, src); }
    C_u8x16 operator|(u8x16 const& src) const { return BinaryOperation(otXmmIntOr , src); }
    C_u8x16 operator^(u8x16 const& src) const { return BinaryOperation(otXmmIntXor, src); }

    C_u16x8 InterleaveLow (u8x16 const &src) const { return BinaryOperation(otXmmBytesInterleaveLow , src); }
    C_u16x8 InterleaveHigh(u8x16 const &src) const { return BinaryOperation(otXmmBytesInterleaveHigh, src); }

    C_u8x16& operator+=(C_u8x16 const& other) { return BinaryAssignment(otXmmBytesAdd, other); }
    C_u8x16& operator-=(C_u8x16 const& other) { return BinaryAssignment(otXmmBytesSub, other); }
    C_u8x16& operator&=(C_u8x16 const& other) { return BinaryAssignment(otXmmIntAnd, other); }
    C_u8x16& operator|=(C_u8x16 const& other) { return BinaryAssignment(otXmmIntOr , other); }
    C_u8x16& operator^=(C_u8x16 const& other) { return BinaryAssignment(otXmmIntXor, other); }

    C_u8x16& operator+=(u8x16 const& src) { return BinaryAssignment(otXmmBytesAdd, src); }
    C_u8x16& operator-=(u8x16 const& src) { return BinaryAssignment(otXmmBytesSub, src); }
    C_u8x16& operator&=(u8x16 const& src) { return BinaryAssignment(otXmmIntAnd, src); }
    C_u8x16& operator|=(u8x16 const& src) { return BinaryAssignment(otXmmIntOr , src); }
    C_u8x16& operator^=(u8x16 const& src) { return BinaryAssignment(otXmmIntXor, src); }

    C_u8x16 operator+(R_u8x16 const& ref) const { return BinaryReference(otXmmBytesAdd, ref); }
    C_u8x16 operator-(R_u8x16 const& ref) const { return BinaryReference(otXmmBytesSub, ref); }
    C_u8x16 operator&(R_u8x16 const& ref) const { return BinaryReference(otXmmIntAnd, ref); }
    C_u8x16 operator|(R_u8x16 const& ref) const { return BinaryReference(otXmmIntOr , ref); }
    C_u8x16 operator^(R_u8x16 const& ref) const { return BinaryReference(otXmmIntXor, ref); }

    C_u16x8 UnpackToWords() const;

private:
    C_u8x16 BinaryOperation(OpType ot, C_u8x16 const& other) const;
    C_u8x16 BinaryOperation(OpType ot, u8x16 const& src) const;
    C_u8x16& BinaryAssignment(OpType ot, C_u8x16 const& other);
    C_u8x16& BinaryAssignment(OpType ot, u8x16 const& src);
    C_u8x16 BinaryReference(OpType ot, R_u8x16 const& ref) const;
};


inline C_u8x16 C_u16x8::PackSignedSaturate(C_u16x8 const &other) const { return BinaryOperation(otXmmWordsPackSS, other); }
inline C_u8x16 C_u16x8::PackUnsignedSaturate(C_u16x8 const &other) const { return BinaryOperation(otXmmWordsPackUS, other); }
inline C_u8x16 C_u16x8::PackSignedSaturate(u16x8 const &src) const { return BinaryOperation(otXmmWordsPackSS, src); }
inline C_u8x16 C_u16x8::PackUnsignedSaturate(u16x8 const &src) const { return BinaryOperation(otXmmWordsPackUS, src); }

