// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on a vector of 8 8-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

#if WPFGFX_FXJIT_X86

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x8::C_u8x8
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u8x8 x = <expression>;
//
//------------------------------------------------------------------------------
C_u8x8::C_u8x8(C_u8x8 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmAssign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x8::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u8x8 variable declared before.
//
//------------------------------------------------------------------------------
C_u8x8 &
C_u8x8::operator=(C_u8x8 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x8::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u8x8
C_u8x8::BinaryOperation(OpType ot, C_u8x8 const& other) const
{
    C_u8x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x8::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u8x8 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u8x8
C_u8x8::BinaryOperation(OpType ot, u8x8 const& src) const
{
    C_u8x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x8::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u8x8&
C_u8x8::BinaryAssignment(OpType ot, C_u8x8 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u8x8::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u8x8 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u8x8&
C_u8x8::BinaryAssignment(OpType ot, u8x8 const& src)
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
//      C_u8x8::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u8x8 value, referenced by R_u8x8, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u8x8
C_u8x8::BinaryReference(OpType ot, R_u8x8 const& ref) const
{
    C_u8x8 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID(), ref.m_uBaseVarID);
    pOperator->m_refType = ref.m_refType;
    pOperator->m_uDisplacement = ref.m_uDisplacement;

    return tmp;
}

#endif //WPFGFX_FXJIT_X86

