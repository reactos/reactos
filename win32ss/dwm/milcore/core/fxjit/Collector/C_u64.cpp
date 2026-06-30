// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent unsigned integer 64-bit variable.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

#if _AMD64_

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::C_u64
//
//  Synopsis:
//      Default constructor: allocates variable ID of vtUINT64 type.
//
//------------------------------------------------------------------------------
C_u64::C_u64()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtUINT64);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::C_u64
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u64 x = <64-bit expression>;
//
//------------------------------------------------------------------------------
C_u64::C_u64(C_u64 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtUINT64);
    pProgram->AddOperator(otUINT64Assign, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u64 variable declared before.
//
//------------------------------------------------------------------------------
C_u64 &
C_u64::operator=(C_u64 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otUINT64Assign, GetID(), src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::C_u64
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u64 x = <32-bit expression>;
//
//------------------------------------------------------------------------------
C_u64::C_u64(C_u32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtUINT64);
    pProgram->AddOperator(otUINT64Assign32, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::operator C_u32()
//
//------------------------------------------------------------------------------
C_u64::operator C_u32() const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otUINT32Assign64, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::C_u64
//
//  Synopsis:
//      Construct C_u64 of a constant.
//
//  Usage example:
//
//      C_u64 x = 0x00FF00FF00FF00FF;
//
//------------------------------------------------------------------------------
C_u64::C_u64(INT64 imm)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtUINT64);
    SOperator *pOperator = pProgram->AddOperator(otUINT64ImmAssign, GetID());
    pOperator->m_uDisplacement = (UINT_PTR)imm;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u64
C_u64::BinaryOperation(OpType ot, C_u64 const& other) const
{
    C_u64 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::operator>>
//
//  Synopsis:
//      Performs logical shift right.
//
//  Usage example:
//      C_u64 a = ...;
//      C_u64 b = a >> 8;
//
//------------------------------------------------------------------------------
C_u64
C_u64::operator>>(UINT32 shift) const
{
    C_u64 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otUINT64ImmShiftRight, tmp.GetID(), GetID());
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otUINT64Assign, tmp.GetID(), GetID());
    }

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u64::operator<<
//
//  Synopsis:
//      Performs logical shift left.
//
//  Usage example:
//      C_u64 a = ...;
//      C_u64 b = a << 8;
//
//------------------------------------------------------------------------------
C_u64
C_u64::operator<<(UINT32 shift) const
{
    C_u64 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otUINT64ImmShiftLeft, tmp.GetID(), GetID());
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otUINT64Assign, tmp.GetID(), GetID());
    }


    return tmp;
}

#endif //_AMD64_

