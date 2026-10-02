// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      SIMD operations on vectors of 4 32-bit values.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::C_u32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u32x4 x = <C_u32x4 expression>;
//
//------------------------------------------------------------------------------
C_u32x4::C_u32x4(C_u32x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <C_u32x4 expression>;
//      where "x" is C_u32x4 variable declared before.
//
//------------------------------------------------------------------------------
C_u32x4 const&
C_u32x4::operator=(C_u32x4 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::C_u32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          C_u32x4 x = <C_u32 expression>;
//
//------------------------------------------------------------------------------
C_u32x4::C_u32x4(C_u32 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmLoadDWord, m_ID, src.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <C_u32 expression>;
//      where "x" is C_u32x4 variable declared before.
//
//------------------------------------------------------------------------------
C_u32x4 &
C_u32x4::operator=(C_u32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmLoadDWord, m_ID, src.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::C_u32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          static const u32x4 = ...;
//          C_u32x4 x = u32x4;
//
//------------------------------------------------------------------------------
C_u32x4::C_u32x4(u32x4 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmIntLoad, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_u32x4 variable declared before,
//      and "c" is u32x4 memory value.
//
//------------------------------------------------------------------------------
C_u32x4 &
C_u32x4::operator=(u32x4 const & src)
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
//      C_u32x4::C_u32x4
//
//  Synopsis:
//      Copy constructor.
//      Serves statements like following:
//          UINT32 a = ...;
//          C_u32x4 x = a;
//
//------------------------------------------------------------------------------
C_u32x4::C_u32x4(UINT32 const &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmLoadDWord, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = c;
//      where "x" is C_u32x4 variable declared before,
//      and "c" is UINT32 memory value.
//
//------------------------------------------------------------------------------
C_u32x4 &
C_u32x4::operator=(UINT32 const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmLoadDWord, m_ID);
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::BinaryOperation(OpType ot, C_u32x4 const& other) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, tmp.GetID(), GetID(), other.m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::BinaryOperation
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u32x4 value, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::BinaryOperation(OpType ot, u32x4 const& src) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(ot, tmp.GetID(), GetID());
    pOperator->m_refType = RefType_Static;
    pOperator->m_uDisplacement = pProgram->SnapData(src);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this and other variables,
//      returning result in this variable.
//
//------------------------------------------------------------------------------
C_u32x4&
C_u32x4::BinaryAssignment(OpType ot, C_u32x4 const& other)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(ot, GetID(), GetID(), other.GetID());
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::BinaryAssignment
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u32x4 value, returning result in this variable.
//
//------------------------------------------------------------------------------
C_u32x4&
C_u32x4::BinaryAssignment(OpType ot, u32x4 const& src)
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
//      C_u32x4::BinaryReference
//
//  Synopsis:
//      Add the operator to execute binary operation on this variable and
//      and u32x4 value, referenced by R_u32x4, returning result in new variable.
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::BinaryReference(OpType ot, R_u32x4 const& ref) const
{
    return ref.BinaryOperation(*this, ot);
}

C_f32x4
C_u32x4::As_f32x4() const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator<<
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u32x4 a = ...;
//      C_u32x4 b = a << 8;
//
//  Assembler: pslld
//  Intrinsic: _mm_slli_epi32
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::operator<<(int shift) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otXmmDWordsShiftLeft, tmp.m_ID, m_ID);
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
//      C_u32x4::operator<<
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u32x4 a = ...;
//      C_u32x4 b = ...;
//      C_u32x4 c = a << b;
//
//  Assembler: pslld
//  Intrinsic: _mm_slli_epi32
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::operator<<(C_u32x4 const& src) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmDWordsShiftLeft, tmp.GetID(), GetID(), src.GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator<<=
//
//  Synopsis:
//      Performs per-component left shift.
//
//  Usage example:
//      C_u32x4 a = ...;
//      a <<= 8;
//
//  Assembler: pslld
//  Intrinsic: _mm_slli_epi32
//
//------------------------------------------------------------------------------
C_u32x4&
C_u32x4::operator<<=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otXmmDWordsShiftLeft, m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator>>
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u32x4 a = ...;
//      C_u32x4 b = a >> 8;
//
//  Assembler: psrld
//  Intrinsic: _mm_srli_epi32
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::operator>>(int shift) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (shift)
    {
        SOperator *pOperator = pProgram->AddOperator(otXmmDWordsShiftRight, tmp.m_ID, m_ID);
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
//      C_u32x4::operator>>=
//
//  Synopsis:
//      Performs per-component logical shift right.
//
//  Usage example:
//      C_u32x4 a = ...;
//      a >>= 8;
//
//  Assembler: psrld
//  Intrinsic: _mm_srli_epi32
//
//------------------------------------------------------------------------------
C_u32x4&
C_u32x4::operator>>=(int shift)
{
    if (shift)
    {
        CProgram * pProgram = WarpPlatform::GetCurrentProgram();
        SOperator *pOperator = pProgram->AddOperator(otXmmDWordsShiftRight, m_ID, m_ID);
        pOperator->m_shift = shift;
    }
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::ShiftRight
//
//  Synopsis:
//      Right shift by 1 double word.
//
//  Operation:
//      dwords[0] = dwords[1];
//      dwords[1] = dwords[2];
//      dwords[2] = dwords[3];
//      dwords[3] = 0;
//
//  Usage example:
//      C_u32x4 a = ...;
//      a.ShiftRight();
//      Result: a shifted
//      C_u32x4 b = a.ShiftRight();
//      Result: a shifted and copied to b.
//
//  Assembler: psrldq
//  Intrinsic: _mm_srli_si128
//
//------------------------------------------------------------------------------
C_u32x4&
C_u32x4::ShiftRight()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmDWordsShiftRight32, m_ID, m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::ToFloat4
//
//  Synopsis:
//      Perform per-component conversion to integer 32-bit value,
//      in correspondence with current SSE rounding mode.
//
//  Assembler: cvtps2dq
//  Intrinsic: _mm_cvtps_epi32
//
//------------------------------------------------------------------------------
C_f32x4
C_u32x4::ToFloat4() const
{
    C_f32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmDWordsToFloat4, tmp.GetID(), GetID());
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::operator~
//
//  Synopsis:
//      Performs a bitwise NOT operation (each 1 is set to 0, and each 0 is
//      set to 1), returning result in a new variable.
//
//  Implementation note:
//      Operator otXmmIntNot works identically to otXmmIntXor.
//      The only difference is revealed in CProgram::OptimizeAndNot()
//      that can consider the second operand to be all-ones and do better job.
//
//------------------------------------------------------------------------------
C_XmmValue
C_u32x4::operator~() const
{
    static const u32x4 sc_ones = {UINT32(-1), UINT32(-1), UINT32(-1), UINT32(-1)};
    return BinaryOperation(otXmmIntNot, sc_ones);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::Shuffle
//
//  Synopsis:
//      Copies doublewords from this instance and inserts them in the results
//      at locations selected with the uOrder operand.
//
//  Operation:
//      result.dwords[0] = this.dwords[(uOrder     ) & 3];
//      result.dwords[1] = this.dwords[(uOrder >> 2) & 3];
//      result.dwords[2] = this.dwords[(uOrder >> 4) & 3];
//      result.dwords[3] = this.dwords[(uOrder >> 6) & 3];
//
//  Usage example:
//      C_u32x4 a = ...;
//      C_u32x4 b = a.Shuffle(0x1B); // 0x1B == (0 << 6) | (1 << 4) | (2 << 2) | (3 << 0)
//      Result:
//          b.dwords[0] = a.dwords[3];
//          b.dwords[1] = a.dwords[2];
//          b.dwords[2] = a.dwords[1];
//          b.dwords[3] = a.dwords[0];
//          a unchanged.
//
//  Assembler: pshufd
//  Intrinsic: _mm_shuffle_epi32
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::Shuffle(UINT8 uOrder) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmDWordsShuffle, tmp.m_ID, m_ID);
    pOperator->m_bImmediateByte = uOrder;
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::GetElement
//
//  Synopsis:
//      Fetches indexed dword value via memory.
//
//------------------------------------------------------------------------------
C_u32
C_u32x4::GetElement(UINT32 uIndex) const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmDWordsGetElement, tmp.GetID(), m_ID);
    pOperator->m_bImmediateByte = (UINT8)(uIndex & 3);
    return tmp;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::ExtractElement
//
//  Synopsis:
//      Fetches indexed dword value using instruction PEXTRD (SSE4.1).
//
//------------------------------------------------------------------------------
C_u32
C_u32x4::ExtractElement(UINT32 uIndex) const
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram->m_fUseSSE41);
    SOperator *pOperator = pProgram->AddOperator(otXmmDWordsExtractElement, tmp.GetID(), GetID());
    pOperator->m_bImmediateByte = (UINT8)(uIndex & 3);
    return tmp;
}

C_u32x4
C_u32x4::InsertElement(C_u32 const & src, UINT32 uIndex) const
{
    C_u32x4 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram->m_fUseSSE41);
    SOperator *pOperator = pProgram->AddOperator(otXmmDWordsInsertElement, tmp.GetID(), GetID(), src.GetID());
    pOperator->m_bImmediateByte = (UINT8)(uIndex & 3);
    return tmp;
}

IntValueUnpacker::IntValueUnpacker(C_u32x4 const & src)
{
    m_data = src;
    m_uCount = 0;
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_fUseSSE41 = pProgram->m_fUseSSE41;
    m_fAvoidMOVDs = pProgram->m_fAvoidMOVDs;
}

C_u32 IntValueUnpacker::GetValue()
{
    if (m_fUseSSE41)
    {
        return m_data.ExtractElement(m_uCount++);
    }
    else if (m_fAvoidMOVDs)
    {
        return m_data.GetElement(m_uCount++);
    }
    else
    {
        C_u32 result = m_data.GetLowDWord();
        m_data.ShiftRight();
        return result;
    }
}


IntValuePacker::IntValuePacker()
{
    m_uCount = 0;

    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_fUseSSE41 = pProgram->m_fUseSSE41;
}

void
IntValuePacker::AddValue(const C_u32& src)
{
    if (m_fUseSSE41)
    {
        if (m_uCount == 0)
        {
            m_result = (C_u32x4)src;
        }
        else
        {
            m_result = m_result.InsertElement(src, m_uCount);
        }
    }
    else
    {
        switch (m_uCount)
        {
        case 0:
        case 2:
            m_temp = src;
            break;

        case 1:
            m_low = ((C_u32x4)m_temp).InterleaveLow((C_u32x4)src);
            break;

        case 3:
            m_high = ((C_u32x4)m_temp).InterleaveLow((C_u32x4)src);
            m_result = m_low.InterleaveLow(m_high);
            break;

        default:
            NO_DEFAULT;
        }
    }

    m_uCount++;
}

C_u32x4
IntValuePacker::Result()
{
    WarpAssert(m_uCount == 4);
    return m_result;
}

#if DBG
void
C_Variable::AssertSSE41()
{
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);
    WarpAssert(pProgram->m_fUseSSE41);
}
#endif


//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::Blend
//
//  Synopsis:
//      Select components of "src", marked with ones in "mask", and copy them
//      to result. Copy remaining bits from this instance.
//      I.e.:   return (src & mask) | (*this & ~mask);
//
//  Note:
//      Caller is responsible to provide either all zeros or all ones
//      in every dword of the mask. This is important because
//      regular SSE2 version tests all the 128 bits while SSE4.1
//      variant only looks for sign bits of every byte of mask.
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::Blend(C_u32x4 const & src, C_u32x4 const & mask) const
{
    C_u32x4 result;

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        pProgram->AddOperator(otXmmBytesBlend, result.GetID(), GetID(), src.GetID(), mask.GetID());
    }
    else
    {
        C_u32x4 t1, t2;
        pProgram->AddOperator(otXmmIntAnd, t1.GetID(), src.GetID(), mask.GetID());
        pProgram->AddOperator(otXmmIntAndNot, t2.GetID(), mask.GetID(), GetID());
        pProgram->AddOperator(otXmmIntOr, result.GetID(), t1.GetID(), t2.GetID());
    }
    return result;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::Max
//
//  Synopsis:
//      Computes per-component maximum of unsigned 32-bit integers.
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::Max(C_u32x4 const& other) const
{
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        return BinaryOperation(otXmmDWordsUnsignedMax, other);
    }
    else
    {
        u32x4 shift = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
        C_u32x4 mask = (other + shift).AsC_s32x4() > (*this + shift).AsC_s32x4();
        return Blend(other, mask);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_u32x4::Min
//
//  Synopsis:
//      Computes per-component minimum of unsigned 32-bit integers.
//
//------------------------------------------------------------------------------
C_u32x4
C_u32x4::Min(C_u32x4 const& other) const
{
    const CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (pProgram->m_fUseSSE41)
    {
        return BinaryOperation(otXmmDWordsUnsignedMin, other);
    }
    else
    {
        u32x4 shift = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
        C_u32x4 mask = (*this + shift).AsC_s32x4() > (other + shift).AsC_s32x4();
        return Blend(other, mask);
    }
}


