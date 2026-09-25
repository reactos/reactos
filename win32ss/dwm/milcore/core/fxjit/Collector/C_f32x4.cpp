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

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::C_f32x4
//
//  Synopsis:
//      Constructor: allocate variable ID of vtXmmF4 type.
//
//------------------------------------------------------------------------------
C_f32x4::C_f32x4()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmmF4);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::C_f32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_f32x4 x = <expression>;
//
//------------------------------------------------------------------------------
C_f32x4::C_f32x4(C_f32x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmmF4);
    pProgram->AddOperator(otXmmFloat4Assign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_f32x4 variable declared before.
//
//------------------------------------------------------------------------------
C_f32x4 &
C_f32x4::operator=(C_f32x4 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat4Assign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::C_f32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          static const f32x4 = ...;
//          C_f32x4 x = f32x4;
//
//------------------------------------------------------------------------------
C_f32x4::C_f32x4(f32x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmmF4);
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat4Load, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_f32x4 variable declared before,
//      and "c" is f32x4 memory value.
//
//------------------------------------------------------------------------------
C_f32x4 &
C_f32x4::operator=(f32x4 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat4Load, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::operator C_f32x1()
//
//------------------------------------------------------------------------------
C_f32x4::operator C_f32x1() const
{
    C_f32x1 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat1Assign, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::BinaryOperation(OpType ot, C_f32x4 const& other) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

C_f32x4
C_f32x4::BinaryOperationWithInt(OpType ot, C_u128x1 const& other) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and f32x4 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::BinaryOperation(OpType ot, f32x4 const& src) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_f32x4&
C_f32x4::BinaryAssignment(OpType ot, C_f32x4 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and f32x4 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_f32x4&
C_f32x4::BinaryAssignment(OpType ot, f32x4 const& src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and f32x4 value, referenced by R_f32x4, returning result in new variable.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::BinaryReference(OpType ot, R_f32x4 const& ref) const
{
    return ref.BinaryOperation(*this, ot);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::UnpackHigh
//
//  Synopsis:
//      Performs an interleaved unpack of the high-order single-precision
//      floating-point values.
//
//  Operation:
//      result.floats[0] = this.floats[2];
//      result.floats[1] =  src.floats[2];
//      result.floats[2] = this.floats[3];
//      result.floats[3] =  src.floats[3];
//
//  Usage example:
//      C_f32x4 a = ...;
//      C_f32x4 b = ...;
//      C_f32x4 c = a.UnpackHigh(b);
//      Result: c calculated as described; a and b unchanged.
//
//  Assembler: unpckhps
//  Intrinsic: _mm_unpackhi_ps
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::UnpackHigh(C_f32x4 const& src) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat4UnpackHigh, tmp.m_ID, m_ID, src.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::UnpackLow
//
//  Synopsis:
//      Performs an interleaved unpack of the low-order single-precision
//      floating-point values.
//
//  Operation:
//      result.floats[0] = this.floats[0];
//      result.floats[1] =  src.floats[0];
//      result.floats[2] = this.floats[1];
//      result.floats[3] =  src.floats[1];
//
//  Usage example:
//      C_f32x4 a = ...;
//      C_f32x4 b = ...;
//      C_f32x4 c = a.UnpackLow(b);
//      Result: c calculated as described; a and b unchanged.
//
//  Assembler: unpcklps
//  Intrinsic: _mm_unpacklo_ps
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::UnpackLow(C_f32x4 const& src) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat4UnpackLow, tmp.m_ID, m_ID, src.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::Shuffle
//
//  Synopsis:
//      Moves two floating-point values of this instance into the low quadword;
//      moves two floating-point values from the source operand into
//      the high quadword.
//
//  Operation:
//      result.floats[0] = this.floats[(uOrder     ) & 3];
//      result.floats[1] = this.floats[(uOrder >> 2) & 3];
//      result.floats[2] =  src.floats[(uOrder >> 4) & 3];
//      result.floats[3] =  src.floats[(uOrder >> 6) & 3];
//
//  Usage example:
//      C_f32x4 a = ...;
//      C_f32x4 b = ...;
//      C_f32x4 c = a.Shuffle(b, 0x1B); // 0x1B == (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0)
//      Result:
//          c.floats[0] = a.floats[3];
//          c.floats[1] = a.floats[2];
//          c.floats[2] = b.floats[1];
//          c.floats[3] = b.floats[0];
//          a and b unchanged.
//
//  Assembler: shufps
//  Intrinsic: _mm_shuffle_ps
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::Shuffle(C_f32x4 const& src, UINT8 uOrder) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat4Shuffle, tmp.m_ID, m_ID, src.m_ID);
    pOperator->m_bImmediateByte = uOrder;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::UnaryOperation
//
//  Synopsis:
//      Add the operator to execute unary operation on this variable,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::UnaryOperation(OpType ot) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::CrossOperation
//
//  Synopsis:
//      Unary operation that returns C_u32x4.
//
//------------------------------------------------------------------------------
C_u32x4
C_f32x4::CrossOperation(OpType ot) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::IntFloor
//
//  Synopsis:
//      Perform per-component conversion floating point values to closest
//      integers that are less than or equal to given.
//
//  Assembler: n/a (complex operation)
//  Intrinsic: n/a
//
//------------------------------------------------------------------------------
C_u32x4
C_f32x4::IntFloor() const
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        C_f32x4 tmp;
        pProgram->AddOperator(otXmmFloat4Floor, tmp.GetID(), GetID());
        return tmp.ToInt32x4();
    }
    else
    {
        C_u32x4 result = this->ToInt32x4();     // convert given value to integer (rounding mode doesn't matter)
        C_f32x4 rounded = result.ToFloat4();    // convert back to float

        //
        // Can't calculate "C_u32x4 correction" as "rounded > *this"
        // because it returns C_f32x4
        //
        C_u32x4 correction;
        {
            CProgram * pProgram = WarpPlatform::GetCurrentProgram();
            pProgram->AddOperator(otXmmFloat4CmpLT, correction.GetID(), GetID(), rounded.GetID());
        }

        //
        // When rounded value is greater than given, "correction" is filled with ones so that
        // follwing addition will decrease the result by 1.
        //
        return result + correction;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::IntCeil
//
//  Synopsis:
//      Perform per-component conversion floating point values to closest
//      integers that are greater than or equal to given.
//
//  Assembler: n/a (complex operation)
//  Intrinsic: n/a
//
//------------------------------------------------------------------------------
C_u32x4
C_f32x4::IntCeil() const
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        C_f32x4 tmp;
        pProgram->AddOperator(otXmmFloat4Ceil, tmp.GetID(), GetID());
        return tmp.ToInt32x4();
    }
    else
    {
        C_u32x4 result = this->ToInt32x4();     // convert given value to integer (rounding mode doesn't matter)
        C_f32x4 rounded = result.ToFloat4();    // convert back to float

        //
        // Can't calculate "C_u32x4 correction" as "rounded < *this"
        // because it returns C_f32x4
        //
        C_u32x4 correction;
        pProgram->AddOperator(otXmmFloat4CmpLT, correction.GetID(), rounded.GetID(), GetID());

        //
        // When rounded value is less than given, "correction" is filled with ones so that
        // following subtraction will increase the result by 1.
        //
        return result - correction;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::IntNear
//
//  Synopsis:
//      Perform per-component conversion floating point values to closest
//      integers. Half-integer values are rounded up.
//
//  Assembler: n/a (complex operation)
//  Intrinsic: n/a
//
//  Warning:
//      There exists popular solution to add 0.5 to given value, then
//      calculate Floor. Unfortunately it sometimes lies because of
//      rounding on addition.
//      Examples:
//          1: Given float = 0.49999997 (0x3effffff).
//             adding 0.5 pushes result to next binade so that least bit of
//             mantissa goes away. Result of addition is precise 1.0
//             (0x3f800000) which gives integer 1 while we need 0.
//          2: Given float = 8388609.0 (0x4b000001).
//             An attempt to add 0.5 actually increases this value by 1:
//             8388609.0f + 0.5f = 8388610.0 (0x4b000002).
//
//------------------------------------------------------------------------------
C_u32x4
C_f32x4::IntNear() const
{
    static const f32x4 mhalf = {-.5,-.5,-.5,-.5};

    // Convert given value to integer, assuming default SSE rounding mode
    // (nearest, with half-integers going to nearest even).
    C_u32x4 result = this->ToInt32x4();

    // convert back to float
    C_f32x4 rounded = result.ToFloat4();

    C_f32x4 delta = rounded - *this;

    //
    // Can't calculate "C_u32x4 correction" as "delta.Compare(CompareType_EQ, mhalf)"
    // because it returns C_f32x4
    //
    C_u32x4 correction;
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator * pOperator = pProgram->AddOperator(otXmmFloat4CmpEQ, correction.GetID(), delta.GetID());
        pOperator->m_refType = RefType_Static;
        pOperator->m_uDisplacement = pProgram->SnapData(mhalf);
    }

    return result - correction;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::Fabs
//
//  Synopsis:
//      Perform per-component absolute value calculation.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::Fabs() const
{
    static const u32x4 mask = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    return *this & *(f32x4*)&mask;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::LoadUnaligned
//
//  Synopsis:
//      Load 128-bit value from memory using address that's not aligned to 16-bytes boundary.
//
//------------------------------------------------------------------------------
void
C_f32x4::LoadUnaligned(P_f32x4 const& pXmmFloat4, INT32 nIndex)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat4LoadUnaligned, m_ID, pXmmFloat4.GetID());
    pOperator->m_nOffset = nIndex*(sizeof(f32x4));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::StoreUnaligned
//
//  Synopsis:
//      Store 128-bit value to memory using address that's not necessary aligned to 16-bytes boundary.
//
//------------------------------------------------------------------------------
void
C_f32x4::StoreUnaligned(P_f32x4 const& pXmmFloat4, INT32 nIndex)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat4StoreUnaligned, 0, pXmmFloat4.GetID(), m_ID);
    pOperator->m_nOffset = nIndex*(sizeof(f32x4));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::StoreNonTemporal
//
//  Synopsis:
//      Non-temporal store 128-bit value to memory.
//      The 128-bit value is assumed to contain four packed single precision
//      floating point values.
//
//  Usage example:
//      C_f32x4 a = ...;
//      P_f32x4 p = ...;
//      a.StoreNonTemporal(p);
//
//  Assembler: movntps
//  Intrinsic: _mm_stream_ps
//
//------------------------------------------------------------------------------
void
C_f32x4::StoreNonTemporal(P_f32x4 const& ptr, INT32 nIndex)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(otXmmFloat4StoreNonTemporal, 0, GetID(), ptr.GetID());
    pOperator->m_refType = RefType_Base;
    pOperator->m_uDisplacement = nIndex * sizeof(f32x4);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::ExtractSignBits
//
//  Synopsis:
//      Extracts the sign bits from the packed single-precision floating-point values,
//      formats them into a 4-bit mask.
//
//
//  Operation:
//      result = 0;
//      if (this.floats[0] < 0) result |= 1;
//      if (this.floats[1] < 0) result |= 2;
//      if (this.floats[2] < 0) result |= 4;
//      if (this.floats[3] < 0) result |= 8;

//  Usage example:
//      C_f32x4 a = ...;
//      C_u32 b = a.ExtractSignBits();
//
//  Assembler: movmskps
//  Intrinsic: _mm_movemask_ps
//
//------------------------------------------------------------------------------
C_u32
C_f32x4::ExtractSignBits() const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat4ExtractSignBits, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::operator~
//
//  Synopsis:
//      Performs a bitwise NOT operation (each 1 is set to 0, and each 0 is
//      set to 1), returning result in a new variable.
//
//  Implementation note:
//      Operator otXmmFloat4Not works identically to otXmmFloat4Xor.
//      The only difference is revealed in CProgram::OptimizeAndNot()
//      that can consider the second operand to be all-ones and do better job.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::operator~() const
{
    static const u32x4 sc_ones = {UINT32(-1), UINT32(-1), UINT32(-1), UINT32(-1)};
    return BinaryOperation(otXmmFloat4Not, *(f32x4*)&sc_ones);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::Load64
//
//  Synopsis:
//      Load low 64-bit of 128-bit value to memory.
//      Fill remaining bits with zeros.
//
//  Usage example:
//      C_f32x4 a = ...;
//      P_u8 p = ...;
//      a.Load64(p);
//
//  Assembler: movq
//  Intrinsic: _mm_loadl_epi64
//
//  Issue 2007/05/21 mikhaill:
//      This operation mixes SSE and SSE2 instructions that was said to be
//      deprecated because future CPUs can handle this slow. Hopefully,
//      future CPUs will provide alternative way to do this.
//
//------------------------------------------------------------------------------
void
C_f32x4::Load64(P_u8 const & ptr)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmIntLoad64, m_ID, ptr.GetID());
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::Store64
//
//  Synopsis:
//      Store low 64-bit of 128-bit value to memory.
//
//  Assembler: movq
//  Intrinsic: _mm_storel_epi64
//
//  Issue 2007/05/21 mikhaill:
//      This operation mixes SSE and SSE2 instructions that was said to be
//      deprecated because future CPUs can handle this slow. Hopefully,
//      future CPUs will provide alternative way to do this.
//
//------------------------------------------------------------------------------
void
C_f32x4::Store64(P_u8 const & ptr) const
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmIntStore64, 0, ptr.GetID(), m_ID);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::Blend
//
//  Synopsis:
//      Select components of "src", marked with ones in "mask", and copy them
//      to result. Copy remaining bits from this instance.
//      I.e.:   return (src & mask) | (*this & ~mask);
//
//  Note:
//      Caller is responsible to provide either all zeros or all ones
//      in every dword of the mask. This is important because
//      regular SSE version tests all the 128 bits while SSE4.1
//      variant only looks for sign bits of every byte of mask.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::Blend(C_f32x4 const & src, C_f32x4 const & mask) const
{
    C_f32x4 result;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        pProgram->AddOperator(otXmmBytesBlend, result.GetID(), GetID(), src.GetID(), mask.GetID());
    }
    else
    {
        C_f32x4 t1, t2;
        pProgram->AddOperator(otXmmFloat4And, t1.GetID(), src.GetID(), mask.GetID());
        pProgram->AddOperator(otXmmFloat4AndNot, t2.GetID(), mask.GetID(), GetID());
        pProgram->AddOperator(otXmmFloat4Or, result.GetID(), t1.GetID(), t2.GetID());
    }
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::MinNumber
//
//  Synopsis:
//      Calculates per-component minimum value of given src and *this.
//      When one of component values is a NaN and another is not a NaN,
//      selects the one that's non a NaN.
//      When both src and *this components are NaNs,
//      selects the component from this value.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::MinNumber(C_f32x4 const & src) const
{
    return Blend(OrderedMin(src), src == src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x4::MaxNumber
//
//  Synopsis:
//      Calculates per-component maximum value of given src and *this.
//      When one of component values is a NaN and another is not a NaN,
//      selects the one that's non a NaN.
//      When both src and *this components are NaNs,
//      selects the component from this value.
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x4::MaxNumber(C_f32x4 const & src) const
{
    return Blend(OrderedMax(src), src == src);
}


