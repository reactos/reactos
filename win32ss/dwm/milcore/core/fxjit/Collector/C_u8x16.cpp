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

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x16::C_u8x16
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u8x16 x = <expression>;
//
//------------------------------------------------------------------------------
C_u8x16::C_u8x16(C_u8x16 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x16::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u8x16 variable declared before.
//
//------------------------------------------------------------------------------
C_u8x16 &
C_u8x16::operator=(C_u8x16 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x16::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u8x16
C_u8x16::BinaryOperation(OpType ot, C_u8x16 const& other) const
{
    C_u8x16 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x16::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u8x16 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u8x16
C_u8x16::BinaryOperation(OpType ot, u8x16 const& src) const
{
    C_u8x16 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x16::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u8x16&
C_u8x16::BinaryAssignment(OpType ot, C_u8x16 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x16::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u8x16 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u8x16&
C_u8x16::BinaryAssignment(OpType ot, u8x16 const& src)
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
//      C_u8x16::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u8x16 value, referenced by R_u8x16, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u8x16
C_u8x16::BinaryReference(OpType ot, R_u8x16 const& ref) const
{
    C_u8x16 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID(), ref.m_uBaseVarID);
    pOperator->m_refType = ref.m_refType;
    pOperator->m_uDisplacement = ref.m_uDisplacement;

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x16::UnpackToWords
//
//  Synopsis:
//      Zero extend low 8 bytes to 16-bit words.
//
//------------------------------------------------------------------------------
C_u16x8
C_u8x16::UnpackToWords() const
{
    C_u16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        pProgram->AddOperator(otXmmBytesUnpackToWords, tmp.GetID(), GetID());
    }
    else
    {
        static const u8x16 Zero8x16 = {0};
        return InterleaveLow(Zero8x16);
    }
    return tmp;
}


