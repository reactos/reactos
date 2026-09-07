// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Base class for integer 128-bit prototype variables.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::C_XmmValue
//
//  Synopsis:
//      Constructor: just allocate variable ID of vtXmm type.
//
//------------------------------------------------------------------------------
C_XmmValue::C_XmmValue()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtXmm);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::operator=
//
//  Synopsis:
//      Serves the statement like C_XmmValue x = <expression>;
//
//------------------------------------------------------------------------------
C_XmmValue &
C_XmmValue::operator=(C_XmmValue &src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmAssign, m_ID, src.m_ID);
    return *this;
}

#if WPFGFX_FXJIT_X86
//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::operator C_MmValue
//
//  Synopsis:
//      Serves the statement like C_MmValue x = <expression>;
//
//------------------------------------------------------------------------------
C_XmmValue::operator C_MmValue() const
{
    C_MmValue tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmConvertToMm, tmp.GetID(), GetID());
    return tmp;
}
#endif //WPFGFX_FXJIT_X86

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::GetLowDWord
//
//  Synopsis:
//      Fetch low double word.
//
//  Usage example:
//      C_XmmValue a = ...;
//      C_u32 b = a.GetLowDWord();
//      Result: b = a & 0xFFFFFFFF; a unchanged.
//
//  Assembler: movd
//  Intrinsic: _mm_cvtsi128_si32
//
//------------------------------------------------------------------------------
C_u32
C_XmmValue::GetLowDWord()
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmGetLowDWord, tmp.GetID(), m_ID);
    return tmp;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::Load64
//
//  Synopsis:
//      Load low 64-bit of 128-bit value to memory.
//      Fill remaining bits with zeros.
//
//  Usage example:
//      C_XmmValue a = ...;
//      P_u8 p = ...;
//      a.Load64(p);
//
//  Assembler: movq
//  Intrinsic: _mm_loadl_epi64
//
//------------------------------------------------------------------------------
void
C_XmmValue::Load64(P_u8 const & ptr)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmIntLoad64, m_ID, ptr.GetID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::Store64
//
//  Synopsis:
//      Store low 64-bit of 128-bit value to memory.
//
//  Usage example:
//      C_XmmValue a = ...;
//      P_u8 p = ...;
//      a.Store64(p);
//
//  Assembler: movq
//  Intrinsic: _mm_storel_epi64
//
//------------------------------------------------------------------------------
void
C_XmmValue::Store64(P_u8 const & ptr) const
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmIntStore64, 0, ptr.GetID(), m_ID);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::SetZero
//
//  Synopsis:
//      Fill data with zeros.
//
//  Usage example:
//      C_XmmValue zero;
//      zero.SetZero();
//
//  Assembler: pxor
//  Intrinsic: _mm_setzero_si128
//
//------------------------------------------------------------------------------
C_XmmValue &
C_XmmValue::SetZero()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmSetZero, m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::StoreNonTemporal
//
//  Synopsis:
//      Non-temporal store 128-bit value to memory.
//      The 128-bit value is assumed to contain integer data.
//
//  Usage example:
//      C_XmmValue a = ...;
//      P_u128x1 p = ...;
//      a.StoreNonTemporal(p);
//
//  Assembler: movntdq
//  Intrinsic: _mm_stream_si128
//
//------------------------------------------------------------------------------
void
C_XmmValue::StoreNonTemporal(P_u128x1 const & ptr, INT32 nIndex) const
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otXmmStoreNonTemporal, 0, GetID(), ptr.GetID());
    pOperator->m_refType = RefType_Base;
    pOperator->m_uDisplacement = nIndex * 16;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      C_XmmValue::StoreNonTemporalMasked
//
//  Synopsis:
//      Non-temporal store bytes of this value selected by given mask.
//
//------------------------------------------------------------------------------
void
C_XmmValue::StoreNonTemporalMasked(P_u8 const& ptr, C_XmmValue const& mask) const
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otXmmStoreNonTemporalMasked, 0, GetID(), mask.GetID(), ptr.GetID());
}

