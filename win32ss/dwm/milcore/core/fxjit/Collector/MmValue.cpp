// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Base class for integer 64-bit prototype variables.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

#if WPFGFX_FXJIT_X86

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_MmValue::C_MmValue
//
//  Synopsis:
//      Default constructor: just allocate variable ID of vtMm type.
//
//------------------------------------------------------------------------------
C_MmValue::C_MmValue()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_ID = pProgram->AllocVar(vtMm);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_MmValue::operator=
//
//  Synopsis:
//      Serves statements like following:
//          x = <expression>;
//      where "x" is C_MmValue variable declared before.
//
//------------------------------------------------------------------------------
C_MmValue &
C_MmValue::operator=(C_MmValue const & src)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddOperator(otMmAssign, m_ID, src.m_ID);
    return *this;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_MmValue::StoreNonTemporal
//
//  Synopsis:
//      Non-temporal store 64-bit value to memory.
//
//  Usage example:
//      C_MmValue a = ...;
//      C_pVoid p = ...;
//      a.StoreNonTemporal(p);
//
//  Assembler: movntq
//  Intrinsic: _mm_stream_pi
//
//------------------------------------------------------------------------------
void
C_MmValue::StoreNonTemporal(C_pVoid const &pointer, INT32 nIndex)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otMmStoreNonTemporal, 0, pointer.GetID(), m_ID);
    pOperator->m_nOffset = nIndex*(sizeof(u8x8));
}

#endif //WPFGFX_FXJIT_X86

