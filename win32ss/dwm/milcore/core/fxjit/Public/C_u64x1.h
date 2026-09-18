// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on 64-bit values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      u64x1
//
//  Synopsis:
//      Represents in-memory value of C_u64x1.
//
//------------------------------------------------------------------------------
struct u64x1
{
    UINT64 data[1];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u64x1
//
//  Synopsis:
//      Represents integer 64-bit variable in prototype program.
//
//------------------------------------------------------------------------------
class C_u64x1 : public C_MmValue
{
public:
    C_u64x1() {}

    C_u64x1(C_u64x1 const & origin);
    C_u64x1& operator=(C_u64x1 const& origin);

    C_u64x1(u64x1 const & origin);
    C_u64x1& operator=(u64x1 const& origin);

    C_u64x1 operator+(C_u64x1 const& other) const { return BinaryOperation(otMmQWordAdd, other); }
    C_u64x1 operator-(C_u64x1 const& other) const { return BinaryOperation(otMmQWordSub, other); }
    C_u64x1 operator&(C_u64x1 const& other) const { return BinaryOperation(otMmQWordAnd, other); }
    C_u64x1 operator|(C_u64x1 const& other) const { return BinaryOperation(otMmQWordOr , other); }
    C_u64x1 operator^(C_u64x1 const& other) const { return BinaryOperation(otMmQWordXor, other); }

    C_u64x1 operator+(u64x1 const& src) const { return BinaryOperation(otMmQWordAdd, src); }
    C_u64x1 operator-(u64x1 const& src) const { return BinaryOperation(otMmQWordSub, src); }
    C_u64x1 operator&(u64x1 const& src) const { return BinaryOperation(otMmQWordAnd, src); }
    C_u64x1 operator|(u64x1 const& src) const { return BinaryOperation(otMmQWordOr , src); }
    C_u64x1 operator^(u64x1 const& src) const { return BinaryOperation(otMmQWordXor, src); }

    C_u64x1& operator+=(C_u64x1 const& other) { return BinaryAssignment(otMmQWordAdd, other); }
    C_u64x1& operator-=(C_u64x1 const& other) { return BinaryAssignment(otMmQWordSub, other); }
    C_u64x1& operator&=(C_u64x1 const& other) { return BinaryAssignment(otMmQWordAnd, other); }
    C_u64x1& operator|=(C_u64x1 const& other) { return BinaryAssignment(otMmQWordOr , other); }
    C_u64x1& operator^=(C_u64x1 const& other) { return BinaryAssignment(otMmQWordXor, other); }

    C_u64x1& operator+=(u64x1 const& src) { return BinaryAssignment(otMmQWordAdd, src); }
    C_u64x1& operator-=(u64x1 const& src) { return BinaryAssignment(otMmQWordSub, src); }
    C_u64x1& operator&=(u64x1 const& src) { return BinaryAssignment(otMmQWordAnd, src); }
    C_u64x1& operator|=(u64x1 const& src) { return BinaryAssignment(otMmQWordOr , src); }
    C_u64x1& operator^=(u64x1 const& src) { return BinaryAssignment(otMmQWordXor, src); }

    C_u64x1 operator+(R_u64x1 const& ref) const { return BinaryReference(otMmQWordAdd, ref); }
    C_u64x1 operator-(R_u64x1 const& ref) const { return BinaryReference(otMmQWordSub, ref); }
    C_u64x1 operator&(R_u64x1 const& ref) const { return BinaryReference(otMmQWordAnd, ref); }
    C_u64x1 operator|(R_u64x1 const& ref) const { return BinaryReference(otMmQWordOr , ref); }
    C_u64x1 operator^(R_u64x1 const& ref) const { return BinaryReference(otMmQWordXor, ref); }

private:
    C_u64x1 BinaryOperation(OpType ot, C_u64x1 const& other) const;
    C_u64x1 BinaryOperation(OpType ot, u64x1 const& src) const;
    C_u64x1& BinaryAssignment(OpType ot, C_u64x1 const& other);
    C_u64x1& BinaryAssignment(OpType ot, u64x1 const& src);
    C_u64x1 BinaryReference(OpType ot, R_u64x1 const& ref) const;
};



