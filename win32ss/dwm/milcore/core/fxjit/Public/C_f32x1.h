// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on low floating point value
//      of a vectors of 4 32-bit floating point values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_f32x1
//
//  Synopsis:
//      Represents 128-bit variable in prototype program.
//      Single 128-bit value is treated as single 32-bit floating point value.
//      Upper 96 bits are unused.
//
//      Implementation is based on scalar floating point instructions
//      that keep 96 high order bits of destination operand unchanged.
//      This feature could be utilized but this class is not supposed
//      to guarantee content of high bits. The obstacle are symmetric
//      operations (addition and multiplication). The requirement to
//      convey upper bits from first operand to result prevents optimization
//      based on operand exchange.
//
//------------------------------------------------------------------------------
class C_f32x1 : public C_Variable
{
public:
    C_f32x1();
    C_f32x1(C_f32x1 const &src);
    C_f32x1& operator=(C_f32x1 const& origin);

    C_f32x1(C_u32 const & origin);
    C_f32x1& operator=(C_u32 const& origin);

    C_f32x1(float src);
    C_f32x1& LoadInt(int const * pData);

    C_f32x1 operator+(C_f32x1 const& other) const { return BinaryOperation(otXmmFloat1Add, other); }
    C_f32x1 operator-(C_f32x1 const& other) const { return BinaryOperation(otXmmFloat1Sub, other); }
    C_f32x1 operator*(C_f32x1 const& other) const { return BinaryOperation(otXmmFloat1Mul, other); }
    C_f32x1 operator/(C_f32x1 const& other) const { return BinaryOperation(otXmmFloat1Div, other); }
    C_f32x1       Min(C_f32x1 const& other) const { return BinaryOperation(otXmmFloat1Min, other); }
    C_f32x1       Max(C_f32x1 const& other) const { return BinaryOperation(otXmmFloat1Max, other); }

    C_f32x1 operator+(float const& src) const { return BinaryOperation(otXmmFloat1Add, src); }
    C_f32x1 operator-(float const& src) const { return BinaryOperation(otXmmFloat1Sub, src); }
    C_f32x1 operator*(float const& src) const { return BinaryOperation(otXmmFloat1Mul, src); }
    C_f32x1 operator/(float const& src) const { return BinaryOperation(otXmmFloat1Div, src); }
    C_f32x1       Min(float const& src) const { return BinaryOperation(otXmmFloat1Min, src); }
    C_f32x1       Max(float const& src) const { return BinaryOperation(otXmmFloat1Max, src); }

    C_f32x1& operator+=(C_f32x1 const& other) { return BinaryAssignment(otXmmFloat1Add, other); }
    C_f32x1& operator-=(C_f32x1 const& other) { return BinaryAssignment(otXmmFloat1Sub, other); }
    C_f32x1& operator*=(C_f32x1 const& other) { return BinaryAssignment(otXmmFloat1Mul, other); }
    C_f32x1& operator/=(C_f32x1 const& other) { return BinaryAssignment(otXmmFloat1Div, other); }

    C_f32x1& operator+=(float const& src) { return BinaryAssignment(otXmmFloat1Add, src); }
    C_f32x1& operator-=(float const& src) { return BinaryAssignment(otXmmFloat1Sub, src); }
    C_f32x1& operator*=(float const& src) { return BinaryAssignment(otXmmFloat1Mul, src); }
    C_f32x1& operator/=(float const& src) { return BinaryAssignment(otXmmFloat1Div, src); }

    C_f32x1 operator+(R_f32x1 const& ref) const { return BinaryReference(otXmmFloat1Add, ref); }
    C_f32x1 operator-(R_f32x1 const& ref) const { return BinaryReference(otXmmFloat1Sub, ref); }
    C_f32x1 operator*(R_f32x1 const& ref) const { return BinaryReference(otXmmFloat1Mul, ref); }
    C_f32x1 operator/(R_f32x1 const& ref) const { return BinaryReference(otXmmFloat1Div, ref); }
    C_f32x1       Min(R_f32x1 const& ref) const { return BinaryReference(otXmmFloat1Min, ref); }
    C_f32x1       Max(R_f32x1 const& ref) const { return BinaryReference(otXmmFloat1Max, ref); }

    C_f32x1 Reciprocal() const { return UnaryOperation(otXmmFloat1Reciprocal); }
    C_f32x1 Sqrt      () const { return UnaryOperation(otXmmFloat1Sqrt      ); }
    C_f32x1 Rsqrt     () const { return UnaryOperation(otXmmFloat1Rsqrt     ); }

    C_f32x4 Replicate() const;
    C_f32x4 Interleave(C_f32x1 const& src) const;

private:
    C_f32x1 BinaryOperation(OpType ot, C_f32x1 const& other) const;
    C_f32x1 BinaryOperation(OpType ot, float const& src) const;
    C_f32x1& BinaryAssignment(OpType ot, C_f32x1 const& other);
    C_f32x1& BinaryAssignment(OpType ot, float const& src);
    C_f32x1 BinaryReference(OpType ot, R_f32x1 const& ref) const;

    C_f32x1 UnaryOperation(OpType ot) const;
};



