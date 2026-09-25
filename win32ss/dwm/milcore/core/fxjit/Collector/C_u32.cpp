// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Prototype class to represent unsigned integer 32-bit variable.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

extern "C" _Success_(return != 0) unsigned char _BitScanReverse(_Out_ unsigned long * Index, _In_ unsigned long Mask);
#pragma intrinsic(_BitScanReverse)

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::C_u32
//
//  Synopsis:
//      Default constructor: allocates variable ID of vtUINT32 type.
//
//------------------------------------------------------------------------------
C_u32::C_u32()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtUINT32);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::C_u32
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u32 x = <expression>;
//
//------------------------------------------------------------------------------
C_u32::C_u32(C_u32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtUINT32);
    pProgram->AddOperator(otUINT32Assign, m_ID, src.m_ID);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u32 variable declared before.
//
//------------------------------------------------------------------------------
C_u32 &
C_u32::operator=(C_u32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otUINT32Assign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::C_u32
//
//  Synopsis:
//      Construct C_u32 of a constant.
//
//  Usage example:
//
//      C_u32 x = 5;
//
//------------------------------------------------------------------------------
C_u32::C_u32(UINT32 imm)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtUINT32);
    SOperator *pOperator = pProgram->AddOperator(otUINT32ImmAssign, m_ID);
    pOperator->m_immediateData = imm;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator++ (preincrement)
//
//  Synopsis:
//      Increase value by 1.
//
//  Usage example:
//      C_u32 x = ...;
//      C_u32 y = ++pa;
//
//      Result: x = y = incremented value of x.
//
//------------------------------------------------------------------------------
C_u32& C_u32::operator++()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otUINT32Increment, m_ID, m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator-- (predecrement)
//
//  Synopsis:
//      Decrease value by 1.
//
//  Usage example:
//      C_u32 x = ...;
//      C_u32 y = --pa;
//
//      Result: x = y = decremented value of x.
//
//------------------------------------------------------------------------------
C_u32& C_u32::operator--()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otUINT32Decrement, m_ID, m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32
C_u32::BinaryOperation(OpType ot, C_u32 const& other) const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and UINT32 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32
C_u32::BinaryOperation(OpType ot, UINT32 src) const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    if (src == 0 && (ot == otUINT32ImmAdd || ot == otUINT32ImmSub))
    {
        pProgram->AddOperator(otUINT32Assign, tmp.GetID(), GetID());
    }
    else
    {
        SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
        pOperator->m_immediateData = src;
    }
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator*
//
//  Synopsis:
//      Multiply by constant
//
//------------------------------------------------------------------------------
C_u32
C_u32::operator*(UINT32 src) const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    if (src == 0)
    {
        SOperator *pOperator = pProgram->AddOperator(otUINT32ImmAssign, tmp.GetID());
        pOperator->m_immediateData = 0;
    }
    else if (src == 1)
    {
        pProgram->AddOperator(otUINT32Assign, tmp.GetID(), GetID());
    }
    else if ((src & (src - 1)) == 0)
    {
        // src is power if two, use shift instead
        unsigned long shift;
        auto success = _BitScanReverse(&shift, src);
        Assert(success != FALSE);

        SOperator *pOperator = pProgram->AddOperator(otUINT32ImmShiftLeft, tmp.m_ID, m_ID);
        pOperator->m_shift = (UINT32)shift;
    }
    else
    {
        SOperator *pOperator = pProgram->AddOperator(otUINT32ImmMul, tmp.GetID(), GetID());
        pOperator->m_immediateData = src;
    }
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u32&
C_u32::BinaryAssignment(OpType ot, C_u32 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u32 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u32&
C_u32::BinaryAssignment(OpType ot, UINT32 src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, GetID(), GetID());
    pOperator->m_immediateData = src;
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and UINT32 value, referenced by R_u32, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32
C_u32::BinaryReference(OpType ot, R_u32 const& ref) const
{
    return ref.BinaryOperation(*this, ot);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator>>
//
//  Synopsis:
//      Performs logical shift right.
//
//  Usage example:
//      C_u32 a = ...;
//      C_u32 b = a >> 8;
//
//------------------------------------------------------------------------------
C_u32
C_u32::operator>>(UINT32 shift) const
{
    C_u32 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otUINT32ImmShiftRight, tmp.m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otUINT32Assign, tmp.m_ID, m_ID);
    }

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator>>=
//
//  Synopsis:
//      Performs logical shift right.
//
//  Usage example:
//      C_u32 a = ...;
//      a >>= 8;
//
//------------------------------------------------------------------------------
C_u32 &
C_u32::operator>>=(UINT32 shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otUINT32ImmShiftRight, m_ID, m_ID);
        pOperator->m_shift = shift;
    }

    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator<<
//
//  Synopsis:
//      Performs logical shift left.
//
//  Usage example:
//      C_u32 a = ...;
//      C_u32 b = a << 8;
//
//------------------------------------------------------------------------------
C_u32
C_u32::operator<<(UINT32 shift) const
{
    C_u32 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otUINT32ImmShiftLeft, tmp.m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otUINT32Assign, tmp.m_ID, m_ID);
    }


    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::operator<<=
//
//  Synopsis:
//      Performs logical shift left.
//
//  Usage example:
//      C_u32 a = ...;
//      a <<= 8;
//
//------------------------------------------------------------------------------
C_u32 &
C_u32::operator<<=(UINT32 shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otUINT32ImmShiftLeft, m_ID, m_ID);
        pOperator->m_shift = shift;
    }

    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::StoreNonTemporal
//
//  Synopsis:
//      Non-temporal store 32-bit value to memory.
//
//  Usage example:
//      C_u32 a = ...;
//      P_u32 p = ...;
//      a.StoreNonTemporal(p);
//      a.StoreNonTemporal(p, 15);
//
//  Assembler: movnti
//  Intrinsic: _mm_stream_si32
//
//------------------------------------------------------------------------------
void
C_u32::StoreNonTemporal(P_u32 const& pUINT32, INT32 nIndex)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(otUINT32StoreNonTemporal, 0, GetID(), pUINT32.GetID());
    pOperator->m_refType = RefType_Base;
    pOperator->m_uDisplacement = nIndex * sizeof(UINT32);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32::Replicate
//
//  Synopsis:
//      Produces 128-bit value containing 4 copies of this 32-bit value.
//
//  Usage example:
//      C_u32 a = ...;
//      C_u32x4 b = a.Replicate();
//
//  Assembler & intrinsic: n/a (complex operation)
//
//------------------------------------------------------------------------------
C_u32x4
C_u32::Replicate() const
{
    C_u32x4 result;
    result = *this;
    return result.Shuffle(0);
}


