// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 4 32-bit floating point values.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Struct:
//      f32x4
//
//  Synopsis:
//      Represents in-memory value of C_f32x4.
//
//------------------------------------------------------------------------------
struct f32x4
{
    float floats[4];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      C_f32x4
//
//  Synopsis:
//      Represents 128-bit variable in prototype program.
//      Single 128-bit value is treated as an array of 4 32-bit floating point values.
//
//
//------------------------------------------------------------------------------
class C_f32x4 : public C_Variable
{
public:
    C_f32x4();

    C_f32x4(C_f32x4 const & origin);
    C_f32x4& operator=(C_f32x4 const& origin);

    C_f32x4(f32x4 const & origin);
    C_f32x4& operator=(f32x4 const& origin);

    operator C_f32x1() const;

    C_f32x4 operator+(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Add, other); }
    C_f32x4 operator-(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Sub, other); }
    C_f32x4 operator*(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Mul, other); }
    C_f32x4 operator/(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Div, other); }
    C_f32x4 operator&(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4And, other); }
    C_f32x4 operator|(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Or , other); }
    C_f32x4 operator^(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Xor, other); }
    C_f32x4       Min(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Min, other); }
    C_f32x4       Max(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4Max, other); }
    C_f32x4    AndNot(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4AndNot, other); }
    //
    // Following two operations differ from regular Min and Max only when at least one of operands is NaN.
    // In this case "other" component is selected, while Min and Max might return either "other" or "this".
    // Ordered operations might infer minor performance losses because optimizer is disallowed to swap operands.
    //
    C_f32x4 OrderedMin(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4OrderedMin, other); }
    C_f32x4 OrderedMax(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4OrderedMax, other); }


    C_f32x4 operator==(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4CmpEQ , other); }
    C_f32x4 operator< (C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4CmpLT , other); }
    C_f32x4 operator<=(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4CmpLE , other); }
    C_f32x4 operator!=(C_f32x4 const& other) const { return BinaryOperation(otXmmFloat4CmpNEQ, other); }
    C_f32x4 operator>=(C_f32x4 const& other) const { return other <= *this; }
    C_f32x4 operator> (C_f32x4 const& other) const { return other <  *this; }

    C_f32x4 operator&(C_u128x1 const& other) const { return BinaryOperationWithInt(otXmmFloat4And, other); }
    C_f32x4 operator|(C_u128x1 const& other) const { return BinaryOperationWithInt(otXmmFloat4Or , other); }
    C_f32x4 operator^(C_u128x1 const& other) const { return BinaryOperationWithInt(otXmmFloat4Xor, other); }
    C_f32x4    AndNot(C_u128x1 const& other) const { return BinaryOperationWithInt(otXmmFloat4AndNot, other); }

    C_f32x4 operator+(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Add, src); }
    C_f32x4 operator-(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Sub, src); }
    C_f32x4 operator*(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Mul, src); }
    C_f32x4 operator/(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Div, src); }
    C_f32x4 operator&(f32x4 const& src) const { return BinaryOperation(otXmmFloat4And, src); }
    C_f32x4 operator|(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Or , src); }
    C_f32x4 operator^(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Xor, src); }
    C_f32x4       Min(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Min, src); }
    C_f32x4       Max(f32x4 const& src) const { return BinaryOperation(otXmmFloat4Max, src); }
    C_f32x4 OrderedMin(f32x4 const& src) const { return BinaryOperation(otXmmFloat4OrderedMin, src); }
    C_f32x4 OrderedMax(f32x4 const& src) const { return BinaryOperation(otXmmFloat4OrderedMax, src); }

    C_f32x4 operator==(f32x4 const& src) const { return BinaryOperation(otXmmFloat4CmpEQ , src); }
    C_f32x4 operator< (f32x4 const& src) const { return BinaryOperation(otXmmFloat4CmpLT , src); }
    C_f32x4 operator<=(f32x4 const& src) const { return BinaryOperation(otXmmFloat4CmpLE , src); }
    C_f32x4 operator!=(f32x4 const& src) const { return BinaryOperation(otXmmFloat4CmpNEQ, src); }
    //
    // Following two comparisons might infer minor perf losses due to necessity to switch operands
    // that might need a register. However it would be wrong to implement these operators as
    // BinaryOperation(otXmmFloat4CmpNLT/NLE...) because it will treat NaNs wrong way.
    //
    C_f32x4 operator>=(f32x4 const& src) const { C_f32x4 other = src; return other <= *this; }
    C_f32x4 operator> (f32x4 const& src) const { C_f32x4 other = src; return other <  *this; }

    C_f32x4& operator+=(C_f32x4 const& other) { return BinaryAssignment(otXmmFloat4Add, other); }
    C_f32x4& operator-=(C_f32x4 const& other) { return BinaryAssignment(otXmmFloat4Sub, other); }
    C_f32x4& operator*=(C_f32x4 const& other) { return BinaryAssignment(otXmmFloat4Mul, other); }
    C_f32x4& operator/=(C_f32x4 const& other) { return BinaryAssignment(otXmmFloat4Div, other); }
    C_f32x4& operator&=(C_f32x4 const& other) { return BinaryAssignment(otXmmFloat4And, other); }
    C_f32x4& operator|=(C_f32x4 const& other) { return BinaryAssignment(otXmmFloat4Or , other); }
    C_f32x4& operator^=(C_f32x4 const& other) { return BinaryAssignment(otXmmFloat4Xor, other); }

    C_f32x4& operator+=(f32x4 const& src) { return BinaryAssignment(otXmmFloat4Add, src); }
    C_f32x4& operator-=(f32x4 const& src) { return BinaryAssignment(otXmmFloat4Sub, src); }
    C_f32x4& operator*=(f32x4 const& src) { return BinaryAssignment(otXmmFloat4Mul, src); }
    C_f32x4& operator/=(f32x4 const& src) { return BinaryAssignment(otXmmFloat4Div, src); }
    C_f32x4& operator&=(f32x4 const& src) { return BinaryAssignment(otXmmFloat4And, src); }
    C_f32x4& operator|=(f32x4 const& src) { return BinaryAssignment(otXmmFloat4Or , src); }
    C_f32x4& operator^=(f32x4 const& src) { return BinaryAssignment(otXmmFloat4Xor, src); }

    C_f32x4 operator+(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Add, ref); }
    C_f32x4 operator-(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Sub, ref); }
    C_f32x4 operator*(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Mul, ref); }
    C_f32x4 operator/(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Div, ref); }
    C_f32x4 operator&(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4And, ref); }
    C_f32x4 operator|(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Or , ref); }
    C_f32x4 operator^(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Xor, ref); }
    C_f32x4       Min(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Min, ref); }
    C_f32x4       Max(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4Max, ref); }
    C_f32x4 OrderedMin(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4OrderedMin, ref); }
    C_f32x4 OrderedMax(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4OrderedMax, ref); }

    C_f32x4 operator==(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4CmpEQ , ref); }
    C_f32x4 operator< (R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4CmpLT , ref); }
    C_f32x4 operator<=(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4CmpLE , ref); }
    C_f32x4 operator!=(R_f32x4 const& ref) const { return BinaryReference(otXmmFloat4CmpNEQ, ref); }
    C_f32x4 operator>=(R_f32x4 const& ref) const;
    C_f32x4 operator> (R_f32x4 const& ref) const;

    C_f32x4 UnpackHigh(C_f32x4 const& other) const;
    C_f32x4 UnpackLow(C_f32x4 const& other) const;
    C_f32x4 Shuffle(C_f32x4 const& other, UINT8 uOrder) const;

    C_f32x4 Reciprocal() const { return UnaryOperation(otXmmFloat4Reciprocal); }
    C_f32x4 Sqrt      () const { return UnaryOperation(otXmmFloat4Sqrt      ); }
    C_f32x4 Rsqrt     () const { return UnaryOperation(otXmmFloat4Rsqrt     ); }
    C_u32x4 ToInt32x4 () const { return CrossOperation(otXmmFloat4ToInt32x4 ); }
    C_u32x4 AsInt32x4 () const { return CrossOperation(otXmmAssign          ); }
    C_u32x4 Truncate  () const { return CrossOperation(otXmmFloat4Truncate  ); }
    C_f32x4 operator~ () const;

    void LoadUnaligned(P_f32x4 const& pXmmFloat4, INT32 nIndex = 0);
    void StoreUnaligned(P_f32x4 const& pXmmFloat4, INT32 nIndex = 0);
    void StoreNonTemporal(P_f32x4 const& pXmmFloat4, INT32 nIndex = 0);
    C_u32 ExtractSignBits() const;

    void Load64(P_u8 const & ptr);
    void Store64(P_u8 const & ptr) const;

    // complex operations
    C_u32x4 IntFloor() const;
    C_u32x4 IntCeil() const;
    C_u32x4 IntNear() const;
    C_f32x4 Fabs() const;

    C_f32x4 Blend(C_f32x4 const & src, C_f32x4 const & mask) const;
    C_f32x4 Blend(C_f32x4 const & src, C_u32x4 const & mask) const { return Blend(src, mask.As_f32x4()); }

    C_f32x4 MinNumber(C_f32x4 const& other) const;
    C_f32x4 MaxNumber(C_f32x4 const& other) const;

private:
    C_f32x4 BinaryOperation(OpType ot, C_f32x4 const& other) const;
    C_f32x4 BinaryOperationWithInt(OpType ot, C_u128x1 const& other) const;
    C_f32x4 BinaryOperation(OpType ot, f32x4 const& src) const;
    C_f32x4& BinaryAssignment(OpType ot, C_f32x4 const& other);
    C_f32x4& BinaryAssignment(OpType ot, f32x4 const& src);
    C_f32x4 BinaryReference(OpType ot, R_f32x4 const& ref) const;

    C_f32x4 UnaryOperation(OpType ot) const;
    C_u32x4 CrossOperation(OpType ot) const;
};



