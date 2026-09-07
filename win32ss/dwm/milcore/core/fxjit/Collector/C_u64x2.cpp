// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 2 64-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::C_u64x2
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u64x2 x = <expression>;
//
//------------------------------------------------------------------------------
C_u64x2::C_u64x2(C_u64x2 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u64x2 variable declared before.
//
//------------------------------------------------------------------------------
C_u64x2 &
C_u64x2::operator=(C_u64x2 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, GetID(), src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::C_u64x2
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          static const u64x2 = ...;
//          C_u64x2 x = u64x2;
//
//------------------------------------------------------------------------------
C_u64x2::C_u64x2(u64x2 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmIntLoad, GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_u64x2 variable declared before,
//      and "c" is u64x2 memory value.
//
//------------------------------------------------------------------------------
C_u64x2 &
C_u64x2::operator=(u64x2 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmIntLoad, GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

#if WPFGFX_FXJIT_X86
//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::C_u64x2
//
//  Synopsis:
//      Construct 128-bit value from 64-bit MMX value.
//      Fill older bits with zeros.
//
//------------------------------------------------------------------------------
C_u64x2::C_u64x2(C_u64x1 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmQWordToXmm, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::operator=
//
//  Synopsis:
//      Construct 128-bit value from 64-bit MMX value.
//      Fill older bits with zeros.
//
//------------------------------------------------------------------------------
C_u64x2 &
C_u64x2::operator=(C_u64x1 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmQWordToXmm, GetID(), src.GetID());
    return *this;
}
#endif //WPFGFX_FXJIT_X86

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u64x2
C_u64x2::BinaryOperation(OpType ot, C_u64x2 const& other) const
{
    C_u64x2 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u64x2 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u64x2
C_u64x2::BinaryOperation(OpType ot, u64x2 const& src) const
{
    C_u64x2 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u64x2&
C_u64x2::BinaryAssignment(OpType ot, C_u64x2 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64x2::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u64x2 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u64x2&
C_u64x2::BinaryAssignment(OpType ot, u64x2 const& src)
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
//      C_u64x2::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u64x2 value, referenced by R_u64x2, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u64x2
C_u64x2::BinaryReference(OpType ot, R_u64x2 const& ref) const
{
    C_u64x2 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID(), ref.m_uBaseVarID);
    pOperator->m_refType = ref.m_refType;
    pOperator->m_uDisplacement = ref.m_uDisplacement;

    return tmp;
}

