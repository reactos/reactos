// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent unsigned integer 64-bit variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u64
//
//  Synopsis:
//      Represents unsigned integer 64-bit variable in prototype program.
//
//------------------------------------------------------------------------------
class C_u64 : public C_Variable
{
public:
    C_u64();
    C_u64(C_u64 const & origin);
    C_u64& operator=(C_u64 const& origin);
    C_u64(C_u32 const & origin);
    C_u64& operator=(C_u32 const& origin);

    operator C_u32() const;

    C_u64(INT64 imm);
    C_u64(UINT32 imm);

    C_u64 operator+(C_u64 const& other) const { return BinaryOperation(otUINT64Add, other); }
    C_u64 operator-(C_u64 const& other) const { return BinaryOperation(otUINT64Sub, other); }
    C_u64 operator*(C_u64 const& other) const { return BinaryOperation(otUINT64Mul, other); }
    C_u64 operator&(C_u64 const& other) const { return BinaryOperation(otUINT64And, other); }
    C_u64 operator|(C_u64 const& other) const { return BinaryOperation(otUINT64Or , other); }
    C_u64 operator^(C_u64 const& other) const { return BinaryOperation(otUINT64Xor, other); }

    C_u64& operator+=(C_u64 const& other) { return BinaryAssignment(otUINT64Add, other); }
    C_u64& operator-=(C_u64 const& other) { return BinaryAssignment(otUINT64Sub, other); }
    C_u64& operator*=(C_u64 const& other) { return BinaryAssignment(otUINT64Mul, other); }
    C_u64& operator&=(C_u64 const& other) { return BinaryAssignment(otUINT64And, other); }
    C_u64& operator|=(C_u64 const& other) { return BinaryAssignment(otUINT64Or , other); }
    C_u64& operator^=(C_u64 const& other) { return BinaryAssignment(otUINT64Xor, other); }

    C_u64 operator>>(UINT32 shift) const;
    C_u64& operator>>=(UINT32 shift);

    C_u64 operator<<(UINT32 shift) const;
    C_u64& operator<<=(UINT32 shift);

private:
    C_u64 BinaryOperation(OpType ot, C_u64 const& other) const;
    C_u64& BinaryAssignment(OpType ot, C_u64 const& other);
};

