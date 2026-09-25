// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 8 unsigned integer 16-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::C_u16x8
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u16x8 x = <expression>;
//
//------------------------------------------------------------------------------
C_u16x8::C_u16x8(C_u16x8 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_u16x8 variable declared before.
//
//------------------------------------------------------------------------------
C_u16x8 &
C_u16x8::operator=(C_u16x8 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::C_u16x8
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          static const u16x8 = ...;
//          C_u16x8 x = u16x8;
//
//------------------------------------------------------------------------------
C_u16x8::C_u16x8(u16x8 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmIntLoad, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_u16x8 variable declared before,
//      and "c" is u16x8 memory value.
//
//------------------------------------------------------------------------------
C_u16x8 &
C_u16x8::operator=(u16x8 const & src)
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
//      C_u16x8::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u16x8
C_u16x8::BinaryOperation(OpType ot, C_u16x8 const& other) const
{
    C_u16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u16x8 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u16x8
C_u16x8::BinaryOperation(OpType ot, u16x8 const& src) const
{
    C_u16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u16x8&
C_u16x8::BinaryAssignment(OpType ot, C_u16x8 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u16x8 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u16x8&
C_u16x8::BinaryAssignment(OpType ot, u16x8 const& src)
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
//      C_u16x8::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u16x8 value, referenced by R_u16x8, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u16x8
C_u16x8::BinaryReference(OpType ot, R_u16x8 const& ref) const
{
    C_u16x8 tmp;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator * pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID(), ref.m_uBaseVarID);
    pOperator->m_refType = ref.m_refType;
    pOperator->m_uDisplacement = ref.m_uDisplacement;

    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::operator<<
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u16x8 a = ...;
//      C_u16x8 b = a << 8;
//
//  Assembler: psllw
//  Intrinsic: _mm_slli_epi16
//
//------------------------------------------------------------------------------
C_u16x8
C_u16x8::operator<<(int shift) const
{
    C_u16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otXmmWordsShiftLeft, tmp.GetID(), m_ID);
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otXmmAssign, tmp.GetID(), m_ID);
    }
    return tmp;

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::operator>>
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u16x8 a = ...;
//      C_u16x8 b = a >> 8;
//
//  Assembler: psrlw
//  Intrinsic: _mm_srli_epi16
//
//------------------------------------------------------------------------------
C_u16x8
C_u16x8::operator>>(int shift) const
{
    C_u16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otXmmWordsShiftRight, tmp.GetID(), m_ID);
        pOperator->m_shift = shift;
    }
    else
    {
        pProgram->AddOperator(otXmmAssign, tmp.GetID(), m_ID);
    }
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::operator>>=
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u16x8 a = ...;
//      a >>= 8;
//
//  Assembler: psrlw
//  Intrinsic: _mm_srli_epi16
//
//------------------------------------------------------------------------------
C_u16x8&
C_u16x8::operator>>=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otXmmWordsShiftRight, m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::ShuffleLow
//
//  Synopsis:
//      Copies 4 words from the low quadword of this instance at word locations
//      selected with the uOrder operand; composes the low quadword result
//      of these 4 words; copies high quadword from this instance to high quadword
//      of the result value.
//
//  Operation:
//      result.words[0] = this.words[(uOrder     ) & 3];
//      result.words[1] = this.words[(uOrder >> 2) & 3];
//      result.words[2] = this.words[(uOrder >> 4) & 3];
//      result.words[3] = this.words[(uOrder >> 6) & 3];
//      result.words[4] = this.words[4];
//      result.words[5] = this.words[5];
//      result.words[6] = this.words[6];
//      result.words[7] = this.words[7];
//
//  Usage example:
//      C_u16x8 a = ...;
//      C_u16x8 b = a.ShuffleLow(0x1B); // 0x1B == (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0)
//      Result:
//          b.dwords[0] = a.dwords[3];
//          b.dwords[1] = a.dwords[2];
//          b.dwords[2] = a.dwords[1];
//          b.dwords[3] = a.dwords[0];
//          a unchanged.
//
//  Assembler: pshuflw
//  Intrinsic: _mm_shufflelo_epi16
//
//------------------------------------------------------------------------------
C_u16x8
C_u16x8::ShuffleLow(UINT8 uOrder) const
{
    C_u16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmWordsShuffleLow, tmp.GetID(), GetID());
    pOperator->m_bImmediateByte = uOrder;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::ShuffleHigh
//
//  Synopsis:
//      Copies 4 words from the high quadword of this instance at word locations
//      selected with the uOrder operand; composes the high quadword result
//      of these 4 words; copies low quadword from this instance to low quadword
//      of result value.
//
//  Operation:
//      result.words[0] = this.words[0];
//      result.words[1] = this.words[1];
//      result.words[2] = this.words[2];
//      result.words[3] = this.words[3];
//      result.words[4] = this.words[4 + ((uOrder     ) & 3)];
//      result.words[5] = this.words[4 + ((uOrder >> 2) & 3)];
//      result.words[6] = this.words[4 + ((uOrder >> 4) & 3)];
//      result.words[7] = this.words[4 + ((uOrder >> 6) & 3)];
//
//  Usage example:
//      C_u16x8 a = ...;
//      C_u16x8 b = a.ShuffleHigh(0x1B); // 0x1B == (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0)
//      Result:
//          b.dwords[4] = a.dwords[7];
//          b.dwords[5] = a.dwords[6];
//          b.dwords[6] = a.dwords[5];
//          b.dwords[7] = a.dwords[4];
//          a unchanged.
//
//  Assembler: pshufhw
//  Intrinsic: _mm_shufflehi_epi16
//
//------------------------------------------------------------------------------
C_u16x8
C_u16x8::ShuffleHigh(UINT8 uOrder) const
{
    C_u16x8 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmWordsShuffleHigh, tmp.GetID(), GetID());
    pOperator->m_bImmediateByte = uOrder;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u16x8::UnpackToDWords
//
//  Synopsis:
//      Zero extend low 8 bytes to 16-bit words.
//
//------------------------------------------------------------------------------
C_u32x4
C_u16x8::UnpackToDWords() const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        pProgram->AddOperator(otXmmWordsUnpackToDWords, tmp.GetID(), GetID());
    }
    else
    {
        static const u16x8 Zero16x8 = {0};
        return InterleaveLow(Zero16x8);
    }
    return tmp;
}


