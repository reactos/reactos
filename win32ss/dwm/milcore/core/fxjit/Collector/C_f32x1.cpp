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

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::C_f32x1
//
//  Synopsis:
//      Constructor: allocate variable ID of vtXmmF1 type.
//
//------------------------------------------------------------------------------
C_f32x1::C_f32x1()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmmF1);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::C_f32x1
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_f32x1 x = <expression>;
//
//------------------------------------------------------------------------------
C_f32x1::C_f32x1(C_f32x1 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmmF1);
    pProgram->AddOperator(otXmmFloat1Assign, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::C_f32x1
//
//  Synopsis:
//      Convert integer to float.
//
//  Assembler: cvtsi2ss
//  Intrinsic: _mm_cvtsi32_ss
//
//------------------------------------------------------------------------------
C_f32x1::C_f32x1(C_u32 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmmF1);
    pProgram->AddOperator(otXmmFloat1FromInt, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::C_f32x1
//
//  Synopsis:
//      Construct C_f32x1 and load given constant.
//
//------------------------------------------------------------------------------
C_f32x1::C_f32x1(float src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmmF1);

    SOperator *pOperator = pProgram->AddOperator(otXmmFloat1Load, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::operator=
//
//  Synopsis:
//      Convert integer to float.
//
//  Assembler: cvtsi2ss
//  Intrinsic: _mm_cvtsi32_ss
//
//------------------------------------------------------------------------------
C_f32x1&
C_f32x1::operator=(C_u32 const& src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat1FromInt, GetID(), src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_f32x1 variable declared before.
//
//------------------------------------------------------------------------------
C_f32x1 &
C_f32x1::operator=(C_f32x1 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat1Assign, GetID(), src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::LoadInt
//
//  Synopsis:
//      Load integer value from memory.
//
//------------------------------------------------------------------------------
C_f32x1&
C_f32x1::LoadInt(int const * pData)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat1LoadInt, GetID());
    pOperator->m_pData = (void*)pData;
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_f32x1
C_f32x1::BinaryOperation(OpType ot, C_f32x1 const& other) const
{
    C_f32x1 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and float value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_f32x1
C_f32x1::BinaryOperation(OpType ot, float const& src) const
{
    C_f32x1 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_f32x1&
C_f32x1::BinaryAssignment(OpType ot, C_f32x1 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and XmmFloat1 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_f32x1&
C_f32x1::BinaryAssignment(OpType ot, float const& src)
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
//      C_f32x1::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and float value, referenced by R_f32x1, returning result in new variable.
//
//------------------------------------------------------------------------------
C_f32x1
C_f32x1::BinaryReference(OpType ot, R_f32x1 const& ref) const
{
    return ref.BinaryOperation(*this, ot);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::Replicate
//
//  Synopsis:
//      Create C_f32x4 value, fill each of four its components
//      with floating point value contained in this instance.
//
//  Operation:
//      result.floats[0] =
//      result.floats[1] =
//      result.floats[2] =
//      result.floats[3] = this.floats[0];
//
//  Assembler: shufps
//  Intrinsic: _mm_shuffle_ps
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x1::Replicate() const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmFloat4Shuffle, tmp.GetID(), GetID(), GetID());
    pOperator->m_bImmediateByte = 0;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_f32x1::Interleave
//
//  Synopsis:
//      Performs an interleaved unpack of the low-order single-precision
//      floating-point values.
//
//  Operation:
//      result.floats[0] = this;
//      result.floats[1] = src;
//      result.floats[2] = undefined;
//      result.floats[3] = undefined;
//
//  Usage example:
//      C_f32x1 a = ...;
//      C_f32x1 b = ...;
//      C_f32x4 c = a.Interleave(b);
//      Result: c calculated as described; a and b unchanged.
//
//  Assembler: unpcklps
//  Intrinsic: _mm_unpacklo_ps
//
//------------------------------------------------------------------------------
C_f32x4
C_f32x1::Interleave(C_f32x1 const& src) const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat1Interleave, tmp.GetID(), GetID(), src.GetID());
    return tmp;
}


