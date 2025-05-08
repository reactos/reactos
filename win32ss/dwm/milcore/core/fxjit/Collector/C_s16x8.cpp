// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 8 signed integer 16-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s16x8::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_s16x8
C_s16x8::BinaryOperation(OpType ot, C_s16x8 const& other) const
{
    C_s16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s16x8::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and s16x8 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_s16x8
C_s16x8::BinaryOperation(OpType ot, s16x8 const& src) const
{
    C_s16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s16x8::operator>>
//
//  Synopsis:
//      Performs per-component arithmetic shift right.
//
//  Usage example:
//      C_u32x4 a = ...;
//      C_u32x4 b = a.AsC_s16x8() >> 8;
//
//  Assembler: psraw
//  Intrinsic: _mm_srai_epi16
//
//------------------------------------------------------------------------------
C_s16x8
C_s16x8::operator>>(int shift) const
{
    C_s16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otXmmWordsSignedShiftRight, tmp.m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otXmmAssign, tmp.m_ID, m_ID);
    }

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s16x8::operator>>=
//
//  Synopsis:
//      Performs per-component arithmetic shift right.
//
//  Usage example:
//      C_s16x8 a = ...;
//      a >>= 8;
//
//  Assembler: psraw
//  Intrinsic: _mm_srai_epi16
//
//------------------------------------------------------------------------------
C_s16x8&
C_s16x8::operator>>=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otXmmWordsSignedShiftRight, m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    return *this;
}


