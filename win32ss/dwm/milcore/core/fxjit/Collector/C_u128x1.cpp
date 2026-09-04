// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on 128-bits values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u128x1::C_u128x1
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u128x1 x = <expression>;
//
//------------------------------------------------------------------------------
C_u128x1::C_u128x1(C_u128x1 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u128x1::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u128x1 variable declared before.
//
//------------------------------------------------------------------------------
C_u128x1 &
C_u128x1::operator=(C_u128x1 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u128x1::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u128x1
C_u128x1::BinaryOperation(OpType ot, C_u128x1 const& other) const
{
    C_u128x1 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

C_u128x1
C_u128x1::BinaryOperationWithFloat(OpType ot, C_f32x4 const& other) const
{
    C_u128x1 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u128x1::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u128x1&
C_u128x1::BinaryAssignment(OpType ot, C_u128x1 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u128x1::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u128x1 value, referenced by R_u128x1, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u128x1
C_u128x1::BinaryReference(OpType ot, R_u128x1 const& ref) const
{
    return ref.BinaryOperation(*this, ot);
}

