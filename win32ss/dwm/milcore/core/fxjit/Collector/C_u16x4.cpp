// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 4 unsigned integer 16-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

#if WPFGFX_FXJIT_X86

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::C_u16x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u16x4 x = <expression>;
//
//------------------------------------------------------------------------------
C_u16x4::C_u16x4(C_u16x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmAssign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u16x4 variable declared before.
//
//------------------------------------------------------------------------------
C_u16x4 &
C_u16x4::operator=(C_u16x4 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::C_u16x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          static const u16x4 = ...;
//          C_u16x4 x = u16x4;
//
//------------------------------------------------------------------------------
C_u16x4::C_u16x4(u16x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otMmLoad, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_u16x4 variable declared before,
//      and "c" is u16x4 memory value.
//
//------------------------------------------------------------------------------
C_u16x4 &
C_u16x4::operator=(u16x4 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otMmLoad, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u16x4
C_u16x4::BinaryOperation(OpType ot, C_u16x4 const& other) const
{
    C_u16x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u16x4 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u16x4
C_u16x4::BinaryOperation(OpType ot, u16x4 const& src) const
{
    C_u16x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u16x4&
C_u16x4::BinaryAssignment(OpType ot, C_u16x4 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u16x4 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u16x4&
C_u16x4::BinaryAssignment(OpType ot, u16x4 const& src)
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
//      C_u16x4::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u16x4 value, referenced by R_u16x4, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u16x4
C_u16x4::BinaryReference(OpType ot, R_u16x4 const& ref) const
{
    C_u16x4 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID(), ref.m_uBaseVarID);
    pOperator->m_refType = ref.m_refType;
    pOperator->m_uDisplacement = ref.m_uDisplacement;

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::operator*
//
//      Execute per-component multiplication.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          result.words[i] = this.words[i] * src.words[i];
//      }
//
//  Usage example:
//      C_u16x4 a = ...;
//      C_u16x4 b = ...;
//      C_u16x4 c = a * b;
//      Result: c = a * b; a and b unchanged.
//
//  Note that high bits of each product are clipped off, so
//  signed vs. unsigned words does not make a difference.
//
//------------------------------------------------------------------------------
C_u16x4
C_u16x4::operator*(C_u16x4 const& src) const
{
    C_u16x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmWordsMul, tmp.m_ID, m_ID, src.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::operator*=
//
//      Execute per-component multiplication.
//
//  Operation:
//      for (int i = 0; i < 8; i++)
//      {
//          this.words[i] *= src.words[i];
//      }
//
//  Usage example:
//      C_u16x4 a = ...;
//      C_u16x4 b = ...;
//      a *= b;
//
//------------------------------------------------------------------------------
C_u16x4&
C_u16x4::operator*=(C_u16x4 const& src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmWordsMul, m_ID, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::operator<<
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u16x4 a = ...;
//      C_u16x4 b = a << 8;
//
//------------------------------------------------------------------------------
C_u16x4
C_u16x4::operator<<(int shift) const
{
    C_u16x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otMmWordsShiftLeft, tmp.GetID(), m_ID);
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otMmAssign, tmp.GetID(), m_ID);
    }
    return tmp;

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::operator>>
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u16x4 a = ...;
//      C_u16x4 b = a >> 8;
//
//------------------------------------------------------------------------------
C_u16x4
C_u16x4::operator>>(int shift) const
{
    C_u16x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otMmWordsShiftRight, tmp.GetID(), m_ID);
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otMmAssign, tmp.GetID(), m_ID);
    }
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x4::operator>>=
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u16x4 a = ...;
//      a >>= 8;
//
//------------------------------------------------------------------------------
C_u16x4&
C_u16x4::operator>>=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otMmWordsShiftRight, m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    return *this;
}

#endif //WPFGFX_FXJIT_X86

