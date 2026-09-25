// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Class C_LazyVar - the wrapper to hold on one of C_Variable derivatives.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_LazyVar::Alloc
//
//  Synopsis:
//      Allocate an array of C_LazyVar instances.
//
//------------------------------------------------------------------------------
C_LazyVar *
C_LazyVar::Alloc(UINT32 size)
{
    WarpAssert(size);

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    WarpAssert(pProgram);

    if(UINT_MAX/sizeof(C_LazyVar) < size)
        return NULL;

    UINT8 *pMem = pProgram->AllocMem(size*sizeof(C_LazyVar));
    return pMem ? new(pMem) C_LazyVar[size] : NULL;
}

C_LazyVar::operator C_u32x4&()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (m_ID)
    {
        WarpAssert(pProgram->GetVarType(m_ID) == vtXmm);
    }
    else
    {
        m_ID = pProgram->AllocVar(vtXmm);
    }
    return *(C_u32x4*)this;
}

C_LazyVar::operator C_f32x4&()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    if (m_ID)
    {
        WarpAssert(pProgram->GetVarType(m_ID) == vtXmmF4);
    }
    else
    {
        m_ID = pProgram->AllocVar(vtXmmF4);
    }
    return *(C_f32x4*)this;
}

