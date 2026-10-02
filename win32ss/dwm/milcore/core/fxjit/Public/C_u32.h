// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent unsigned integer 32-bit variable.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_u32
//
//  Synopsis:
//      Represents unsigned integer 32-bit variable in prototype program.
//
//------------------------------------------------------------------------------
class C_u32 : public C_Variable
{
public:
    C_u32();
    C_u32(C_u32 const & origin);
    C_u32& operator=(C_u32 const& origin);

    C_s32 const & Signed() const { return *(C_s32 const *)this; }
    C_s32 & Signed() { return *(C_s32 *)this; }

    C_u32(UINT32 imm);

    C_u32& operator++();
    C_u32& operator--();

    C_u32 operator+(C_u32 const& other) const { return BinaryOperation(otUINT32Add, other); }
    C_u32 operator-(C_u32 const& other) const { return BinaryOperation(otUINT32Sub, other); }
    C_u32 operator*(C_u32 const& other) const { return BinaryOperation(otUINT32Mul, other); }
    C_u32 operator/(C_u32 const& other) const { return BinaryOperation(otUINT32Div, other); }
    C_u32 operator%(C_u32 const& other) const { return BinaryOperation(otUINT32Rem, other); }
    C_u32 operator&(C_u32 const& other) const { return BinaryOperation(otUINT32And, other); }
    C_u32 operator|(C_u32 const& other) const { return BinaryOperation(otUINT32Or , other); }
    C_u32 operator^(C_u32 const& other) const { return BinaryOperation(otUINT32Xor, other); }

    C_u32 operator+(UINT32 src) const { return BinaryOperation(otUINT32ImmAdd, src); }
    C_u32 operator-(UINT32 src) const { return BinaryOperation(otUINT32ImmSub, src); }
    C_u32 operator*(UINT32 src) const;
    C_u32 operator&(UINT32 src) const { return BinaryOperation(otUINT32ImmAnd, src); }
    C_u32 operator|(UINT32 src) const { return BinaryOperation(otUINT32ImmOr , src); }
    C_u32 operator^(UINT32 src) const { return BinaryOperation(otUINT32ImmXor, src); }

    C_u32& operator+=(C_u32 const& other) { return BinaryAssignment(otUINT32Add, other); }
    C_u32& operator-=(C_u32 const& other) { return BinaryAssignment(otUINT32Sub, other); }
    C_u32& operator*=(C_u32 const& other) { return BinaryAssignment(otUINT32Mul, other); }
    C_u32& operator/=(C_u32 const& other) { return BinaryAssignment(otUINT32Div, other); }
    C_u32& operator%=(C_u32 const& other) { return BinaryAssignment(otUINT32Rem, other); }
    C_u32& operator&=(C_u32 const& other) { return BinaryAssignment(otUINT32And, other); }
    C_u32& operator|=(C_u32 const& other) { return BinaryAssignment(otUINT32Or , other); }
    C_u32& operator^=(C_u32 const& other) { return BinaryAssignment(otUINT32Xor, other); }

    C_u32& operator+=(UINT32 src) { return BinaryAssignment(otUINT32ImmAdd, src); }
    C_u32& operator-=(UINT32 src) { return BinaryAssignment(otUINT32ImmSub, src); }
    C_u32& operator*=(UINT32 src) { return BinaryAssignment(otUINT32ImmMul, src); }
    C_u32& operator&=(UINT32 src) { return BinaryAssignment(otUINT32ImmAnd, src); }
    C_u32& operator|=(UINT32 src) { return BinaryAssignment(otUINT32ImmOr , src); }
    C_u32& operator^=(UINT32 src) { return BinaryAssignment(otUINT32ImmXor, src); }

    C_u32 operator+(R_u32 const& ref) const { return BinaryReference(otUINT32Add, ref); }
    C_u32 operator-(R_u32 const& ref) const { return BinaryReference(otUINT32Sub, ref); }
    C_u32 operator*(R_u32 const& ref) const { return BinaryReference(otUINT32Mul, ref); }
    C_u32 operator/(R_u32 const& ref) const { return BinaryReference(otUINT32Div, ref); }
    C_u32 operator%(R_u32 const& ref) const { return BinaryReference(otUINT32Rem, ref); }
    C_u32 operator&(R_u32 const& ref) const { return BinaryReference(otUINT32And, ref); }
    C_u32 operator|(R_u32 const& ref) const { return BinaryReference(otUINT32Or , ref); }
    C_u32 operator^(R_u32 const& ref) const { return BinaryReference(otUINT32Xor, ref); }

    C_u32 operator>>(UINT32 shift) const;
    C_u32& operator>>=(UINT32 shift);

    C_u32 operator<<(UINT32 shift) const;
    C_u32& operator<<=(UINT32 shift);

    C_u32 operator<<(C_u32 const& other) const { return BinaryOperation(otUINT32ShiftLeft, other); }
    C_u32& operator<<=(C_u32 const& other) { return BinaryAssignment(otUINT32ShiftLeft, other); }

    C_u32 operator>>(C_u32 const& other) const { return BinaryOperation(otUINT32ShiftRight, other); }
    C_u32& operator>>=(C_u32 const& other) { return BinaryAssignment(otUINT32ShiftRight, other); }

    void StoreNonTemporal(P_u32 const& pUINT32, INT32 nIndex = 0);

    C_u32x4 Replicate() const;

protected:
    C_u32 BinaryOperation(OpType ot, C_u32 const& other) const;
    C_u32 BinaryOperation(OpType ot, UINT32 src) const;
    C_u32& BinaryAssignment(OpType ot, C_u32 const& other);
    C_u32& BinaryAssignment(OpType ot, UINT32 src);
    C_u32 BinaryReference(OpType ot, R_u32 const& ref) const;
};

