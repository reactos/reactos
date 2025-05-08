// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 2 64-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u64x2
//
//  Synopsis:
//      Represents in-memory value of C_u64x2.
//
//------------------------------------------------------------------------------
struct u64x2
{
    UINT64 qwords[2];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u64x2
//
//  Synopsis:
//      Represents integer 128-bit variable in prototype program.
//      Single 128-bit value is treated as an array of 2 64-bit words.
//
//
//------------------------------------------------------------------------------
class C_u64x2 : public C_XmmValue
{
public:
    C_u64x2() {}

    C_u64x2(C_u64x2 const & origin);
    C_u64x2& operator=(C_u64x2 const& origin);

    C_u64x2(u64x2 const & origin);
    C_u64x2& operator=(u64x2 const& origin);

#if WPFGFX_FXJIT_X86
    C_u64x2(C_u64x1 const & origin);
    C_u64x2& operator=(C_u64x1 const& origin);
#endif

    C_u64x2 operator+(C_u64x2 const& other) const { return BinaryOperation(otXmmQWordsAdd, other); }
    C_u64x2 operator-(C_u64x2 const& other) const { return BinaryOperation(otXmmQWordsSub, other); }
    C_u64x2 operator&(C_u64x2 const& other) const { return BinaryOperation(otXmmIntAnd, other); }
    C_u64x2 operator|(C_u64x2 const& other) const { return BinaryOperation(otXmmIntOr , other); }
    C_u64x2 operator^(C_u64x2 const& other) const { return BinaryOperation(otXmmIntXor, other); }

    C_u128x1 InterleaveLow (C_u64x2 const& other) const { return BinaryOperation(otXmmQWordsInterleaveLow , other); }
    C_u128x1 InterleaveHigh(C_u64x2 const& other) const { return BinaryOperation(otXmmQWordsInterleaveHigh, other); }

    C_u64x2 operator+(u64x2 const& src) const { return BinaryOperation(otXmmQWordsAdd, src); }
    C_u64x2 operator-(u64x2 const& src) const { return BinaryOperation(otXmmQWordsSub, src); }
    C_u64x2 operator&(u64x2 const& src) const { return BinaryOperation(otXmmIntAnd, src); }
    C_u64x2 operator|(u64x2 const& src) const { return BinaryOperation(otXmmIntOr , src); }
    C_u64x2 operator^(u64x2 const& src) const { return BinaryOperation(otXmmIntXor, src); }

    C_u64x2& operator+=(C_u64x2 const& other) { return BinaryAssignment(otXmmQWordsAdd, other); }
    C_u64x2& operator-=(C_u64x2 const& other) { return BinaryAssignment(otXmmQWordsSub, other); }
    C_u64x2& operator&=(C_u64x2 const& other) { return BinaryAssignment(otXmmIntAnd, other); }
    C_u64x2& operator|=(C_u64x2 const& other) { return BinaryAssignment(otXmmIntOr , other); }
    C_u64x2& operator^=(C_u64x2 const& other) { return BinaryAssignment(otXmmIntXor, other); }

    C_u64x2& operator+=(u64x2 const& src) { return BinaryAssignment(otXmmQWordsAdd, src); }
    C_u64x2& operator-=(u64x2 const& src) { return BinaryAssignment(otXmmQWordsSub, src); }
    C_u64x2& operator&=(u64x2 const& src) { return BinaryAssignment(otXmmIntAnd, src); }
    C_u64x2& operator|=(u64x2 const& src) { return BinaryAssignment(otXmmIntOr , src); }
    C_u64x2& operator^=(u64x2 const& src) { return BinaryAssignment(otXmmIntXor, src); }

    C_u64x2 operator+(R_u64x2 const& ref) const { return BinaryReference(otXmmQWordsAdd, ref); }
    C_u64x2 operator-(R_u64x2 const& ref) const { return BinaryReference(otXmmQWordsSub, ref); }
    C_u64x2 operator&(R_u64x2 const& ref) const { return BinaryReference(otXmmIntAnd, ref); }
    C_u64x2 operator|(R_u64x2 const& ref) const { return BinaryReference(otXmmIntOr , ref); }
    C_u64x2 operator^(R_u64x2 const& ref) const { return BinaryReference(otXmmIntXor, ref); }

private:
    C_u64x2 BinaryOperation(OpType ot, C_u64x2 const& other) const;
    C_u64x2 BinaryOperation(OpType ot, u64x2 const& src) const;
    C_u64x2& BinaryAssignment(OpType ot, C_u64x2 const& other);
    C_u64x2& BinaryAssignment(OpType ot, u64x2 const& src);
    C_u64x2 BinaryReference(OpType ot, R_u64x2 const& ref) const;
};



