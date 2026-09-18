// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 2 32-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

#if WPFGFX_FXJIT_X86

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::C_u32x2
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u32x2 x = <C_u32x2 expression>;
//
//------------------------------------------------------------------------------
C_u32x2::C_u32x2(C_u32x2 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmAssign, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <C_u32x2 expression>;
//      where "x" is C_u32x2 variable declared before.
//
//------------------------------------------------------------------------------
C_u32x2 &
C_u32x2::operator=(C_u32x2 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmAssign, GetID(), src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::C_u32x2
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u32x2 x = <C_u32 expression>;
//
//------------------------------------------------------------------------------
C_u32x2::C_u32x2(C_u32 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmLoadDWord, GetID(), src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <C_u32 expression>;
//      where "x" is C_u32x2 variable declared before.
//
//------------------------------------------------------------------------------
C_u32x2 &
C_u32x2::operator=(C_u32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmLoadDWord, GetID(), src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::C_u32x2
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          static const u32x2 = ...;
//          C_u32x2 x = u32x2;
//
//------------------------------------------------------------------------------
C_u32x2::C_u32x2(u32x2 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otMmLoad, GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_u32x2 variable declared before,
//      and "c" is u32x2 memory value.
//
//------------------------------------------------------------------------------
C_u32x2 &
C_u32x2::operator=(u32x2 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otMmLoad, GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::C_u32x2
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          UINT32 a = ...;
//          C_u32x2 x = a;
//
//------------------------------------------------------------------------------
C_u32x2::C_u32x2(UINT32 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otMmLoadDWord, GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_u32x2 variable declared before,
//      and "c" is UINT32 memory value.
//
//------------------------------------------------------------------------------
C_u32x2 &
C_u32x2::operator=(UINT32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otMmLoadDWord, GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32x2
C_u32x2::BinaryOperation(OpType ot, C_u32x2 const& other) const
{
    C_u32x2 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u32x2 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32x2
C_u32x2::BinaryOperation(OpType ot, u32x2 const& src) const
{
    C_u32x2 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u32x2&
C_u32x2::BinaryAssignment(OpType ot, C_u32x2 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u32x2 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u32x2&
C_u32x2::BinaryAssignment(OpType ot, u32x2 const& src)
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
//      C_u32x2::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u32x2 value, referenced by R_u32x2, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32x2
C_u32x2::BinaryReference(OpType ot, R_u32x2 const& ref) const
{
    C_u32x2 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID(), ref.m_uBaseVarID);
    pOperator->m_refType = ref.m_refType;
    pOperator->m_uDisplacement = ref.m_uDisplacement;

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator<<
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u32x2 a = ...;
//      C_u32x2 b = a << 8;
//
//------------------------------------------------------------------------------
C_u32x2
C_u32x2::operator<<(int shift) const
{
    C_u32x2 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otMmDWordsShiftLeft, tmp.GetID(), GetID());
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otMmAssign, tmp.GetID(), GetID());
    }
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator<<
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u32x2 a = ...;
//      C_u32x2 b = ...;
//      C_u32x2 c = a << b;
//
//------------------------------------------------------------------------------
C_u32x2
C_u32x2::operator<<(C_u32x2 const& src) const
{
    C_u32x2 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmDWordsShiftLeft, tmp.GetID(), GetID(), src.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator<<=
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u32x2 a = ...;
//      a <<= 8;
//
//------------------------------------------------------------------------------
C_u32x2&
C_u32x2::operator<<=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otMmDWordsShiftLeft, GetID(), GetID());
        pOperator->m_shift = shift;
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator>>
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u32x2 a = ...;
//      C_u32x2 b = a >> 8;
//
//------------------------------------------------------------------------------
C_u32x2
C_u32x2::operator>>(int shift) const
{
    C_u32x2 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otMmDWordsShiftRight, tmp.GetID(), GetID());
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otMmAssign, tmp.GetID(), GetID());
    }

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator>>=
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u32x2 a = ...;
//      a >>= 8;
//
//------------------------------------------------------------------------------
C_u32x2&
C_u32x2::operator>>=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otMmDWordsShiftRight, GetID(), GetID());
        pOperator->m_shift = shift;
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x2::operator~
//
//  Synopsis:
//      Performs a bitwise NOT operation (each 1 is set to 0, and each 0 is
//      set to 1), returning result in a new variable.
//
//  Implementation note:
//      Operator otMmQWordNot works identically to otMmQWordXor.
//      The only difference is revealed in CProgram::OptimizeAndNot()
//      that can consider the second operand to be all-ones and do better job.
//
//------------------------------------------------------------------------------
C_MmValue
C_u32x2::operator~() const
{
    static const u32x2 sc_ones = {UINT32(-1), UINT32(-1)};
    return BinaryOperation(otMmQWordNot, sc_ones);
}

#endif //WPFGFX_FXJIT_X86

