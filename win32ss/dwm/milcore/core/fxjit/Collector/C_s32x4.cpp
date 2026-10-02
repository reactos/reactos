// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 4 signed integer 32-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::C_s32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_s32x4 x = <C_s32x4 expression>;
//
//------------------------------------------------------------------------------
C_s32x4::C_s32x4(C_s32x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <C_s32x4 expression>;
//      where "x" is C_s32x4 variable declared before.
//
//------------------------------------------------------------------------------
C_s32x4 &
C_s32x4::operator=(C_s32x4 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::C_s32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_s32x4 x = <C_u32 expression>;
//
//------------------------------------------------------------------------------
C_s32x4::C_s32x4(C_u32 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmLoadDWord, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <C_u32 expression>;
//      where "x" is C_s32x4 variable declared before.
//
//------------------------------------------------------------------------------
C_s32x4 &
C_s32x4::operator=(C_u32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmLoadDWord, m_ID, src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::C_s32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          static const s32x4 = ...;
//          C_s32x4 x = s32x4;
//
//------------------------------------------------------------------------------
C_s32x4::C_s32x4(s32x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmIntLoad, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_s32x4 variable declared before,
//      and "c" is s32x4 memory value.
//
//------------------------------------------------------------------------------
C_s32x4 &
C_s32x4::operator=(s32x4 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmIntLoad, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::operator>>
//
//  Synopsis:
//      Performs per-component arithmetic shift right.
//
//  Usage example:
//      C_s32x4 a = ...;
//      C_s32x4 b = a.AsC_s32x4() >> 8;
//
//  Assembler: psrad
//  Intrinsic: _mm_srai_epi32
//
//------------------------------------------------------------------------------
C_s32x4
C_s32x4::operator>>(int shift) const
{
    C_s32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otXmmDWordsSignedShiftRight, tmp.m_ID, m_ID);
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
//      C_s32x4::operator>>=
//
//  Synopsis:
//      Performs per-component arithmetic shift right.
//
//  Usage example:
//      C_s32x4 a = ...;
//      a >>= 8;
//
//  Assembler: psrad
//  Intrinsic: _mm_srai_epi32
//
//------------------------------------------------------------------------------
C_s32x4&
C_s32x4::operator>>=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otXmmDWordsSignedShiftRight, m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::ExtractSignBits
//
//  Synopsis:
//      Extracts the sign bits from the four 32-bit values,
//      formats them into a 4-bit mask.
//
//  Assembler: movmskps
//  Intrinsic: _mm_movemask_ps
//
//  Note:
//      This routine is an exception of common rule since
//      it mixes integer SSE2 and floating point SSE instructions.
//------------------------------------------------------------------------------
C_u32
C_s32x4::ExtractSignBits() const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmFloat4ExtractSignBits, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::Max
//
//  Synopsis:
//      Computes per-component maximum of signed 32-bit integers.
//
//------------------------------------------------------------------------------
C_s32x4
C_s32x4::Max(C_s32x4 const& other) const
{
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        return BinaryOperation(otXmmDWordsSignedMax, other);
    }
    else
    {
        C_s32x4 mask = other > *this;
        return Blend(other, mask);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_s32x4::Min
//
//  Synopsis:
//      Computes per-component minimum of signed 32-bit integers.
//
//------------------------------------------------------------------------------
C_s32x4
C_s32x4::Min(C_s32x4 const& other) const
{
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        return BinaryOperation(otXmmDWordsSignedMin, other);
    }
    else
    {
        C_s32x4 mask = *this > other;
        return Blend(other, mask);
    }
}


