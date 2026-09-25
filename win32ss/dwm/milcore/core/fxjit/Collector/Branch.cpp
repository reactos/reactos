// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Control transfer classes for prototype routines.
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_Loop::C_Loop
//
//  Synopsis:
//      Construct C_Loop object.
//      Place loop start operator into algorithm description.
//
//------------------------------------------------------------------------------
C_Loop::C_Loop()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_pStartOperator = pProgram->AddOperator(otLoopStart);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_Loop::CountDownAndRepeatIfNonZero
//
//  Synopsis:
//      Place two operators, first to decrement and test 32-value,
//      second to conditional branch to loop start.
//
//  Usage example:
//      C_u32 uCount = ...;
//      C_Loop loop;    // do while (uCount != 0)
//      {
//          // Place loop body operators here
//      }
//
//      loop.CountDownAndRepeatIfNonZero(uCount);
//
//------------------------------------------------------------------------------
void
C_Loop::CountDownAndRepeatIfNonZero(C_u32 & count)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    pProgram->AddOperator(otUINT32DecrementTest, count.GetID(), count.GetID());

    SOperator *pStartOperator = m_pStartOperator;
    WarpAssert(pStartOperator->m_ot == otLoopStart);

    SOperator *pRepeatOperator = pProgram->AddOperator(otLoopRepeatIfNonZero);

    WarpAssert(pStartOperator->m_pLinkedOperator == NULL);
    WarpAssert(pRepeatOperator->m_pLinkedOperator == NULL);

    pStartOperator->m_pLinkedOperator = pRepeatOperator;
    pRepeatOperator->m_pLinkedOperator = pStartOperator;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_Loop::RepeatIfNonZero
//
//  Synopsis:
//      Place two operators, first to test 32-value,
//      second to conditional branch to loop start.
//
//------------------------------------------------------------------------------
void
C_Loop::RepeatIfNonZero(C_u32 const & count)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    pProgram->AddOperator(otUINT32Test, count.GetID(), count.GetID(), count.GetID());

    SOperator *pStartOperator = m_pStartOperator;
    WarpAssert(pStartOperator->m_ot == otLoopStart);

    SOperator *pRepeatOperator = pProgram->AddOperator(otLoopRepeatIfNonZero);

    WarpAssert(pStartOperator->m_pLinkedOperator == NULL);
    WarpAssert(pRepeatOperator->m_pLinkedOperator == NULL);

    pStartOperator->m_pLinkedOperator = pRepeatOperator;
    pRepeatOperator->m_pLinkedOperator = pStartOperator;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_Branch::C_Branch
//
//  Synopsis:
//      Construct C_Branch object.
//
//------------------------------------------------------------------------------
C_Branch::C_Branch()
{
    m_pStartOperator = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_Branch::BranchOnZero
//
//  Synopsis:
//      Place two operators, first to test 32-value,
//      second to conditional branch to branch merge point.
//
//------------------------------------------------------------------------------
void
C_Branch::BranchOnZero(C_u32 &var)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    pProgram->AddOperator(otUINT32Test, 0, var.GetID(), var.GetID());

    WarpAssert(m_pStartOperator == NULL);
    m_pStartOperator = pProgram->AddOperator(otBranchOnZero);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_Branch::BranchOnZeroMask
//
//  Synopsis:
//      Branch if all the four 32-bit components
//      of the arguments are zeros.
//
//  Note:
//      Caller is responsible to provide either all zeros or all ones
//      in every dword of the mask. This is important because
//      SSE4.1 version tests all the 128 bits while regular SSE2
//      variant only looks for sign bits.
//
//------------------------------------------------------------------------------
void
C_Branch::BranchOnZeroMask(C_u32x4 const & mask)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    if (pProgram->m_fUseSSE41)
    {
        pProgram->AddOperator(otXmmIntTest, 0, mask.GetID(), mask.GetID());
    }
    else
    {
        C_u32 Mask32 = mask.AsC_s32x4().ExtractSignBits();
        pProgram->AddOperator(otUINT32Test, 0, Mask32.GetID(), Mask32.GetID());
    }

    WarpAssert(m_pStartOperator == NULL);
    m_pStartOperator = pProgram->AddOperator(otBranchOnZero);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      C_Branch::BranchHere
//
//  Synopsis:
//      Place branch end operator into algorithm description.
//      Link branch start and branch end operators.
//
//------------------------------------------------------------------------------
void
C_Branch::BranchHere()
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();

    SOperator *pStartOperator = m_pStartOperator;
    WarpAssert(pStartOperator->m_ot == otBranchOnZero);

    SOperator *pMergeOperator =  pProgram->AddOperator(otBranchMerge);

    pStartOperator->m_pLinkedOperator = pMergeOperator;
    pMergeOperator->m_pLinkedOperator = pStartOperator;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      C_Control::Call
//
//  Synopsis:
//      Generates a call to external subroutine.
//      Calling conventions: __stdcall.
//      Arbitrary parameters set is not implemented for now;
//      callee has exactly one void *parameter.
//      Callee return value is UINT32 that might be ignored if not required.
//
//------------------------------------------------------------------------------
C_u32
C_Control::Call(void * pCallee, C_pVoid argument)
{
    C_u32 tmp;
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator =  pProgram->AddOperator(otCall, tmp.GetID(), argument.GetID());
    pOperator->m_pData = pCallee;
    return tmp;
}


//------------------------------------------------------------------------------
C_Subroutine::C_Subroutine()
{
    m_pStartOperator = NULL;
    m_pReturnOperator = NULL;
    m_pCallers = NULL;
}

//------------------------------------------------------------------------------
void
C_Subroutine::Call(C_pVoid & pStack)
{
    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    SOperator *pOperator = pProgram->AddOperator(otSubroutineCall, 0, pStack.GetID());

    if (m_pStartOperator)
    {
        pOperator->m_pLinkedOperator = m_pStartOperator;
    }
    else
    {
        // Eventually otSubroutineCall should point to subroutine start operator.
        // If it is not known at this moment, attach this operator to temporary linked list.
        pOperator->m_pLinkedOperator = m_pCallers;
        m_pCallers = pOperator;
    }
}

//------------------------------------------------------------------------------
void
C_Subroutine::Begin()
{
    WarpAssert(!m_pStartOperator);   // should not be called twice

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    pProgram->AddReturnOperator();
    m_pStartOperator = pProgram->AddOperator(otSubroutineStart);

    // unwind the list of callers
    while (m_pCallers)
    {
        SOperator * pOp = m_pCallers;
        m_pCallers = pOp->m_pLinkedOperator;
        pOp->m_pLinkedOperator = m_pStartOperator;
    }
}

//------------------------------------------------------------------------------
void
C_Subroutine::Return(C_pVoid & pStack)
{
    WarpAssert(m_pStartOperator);
    WarpAssert(!m_pReturnOperator);

    CProgram * pProgram = WarpPlatform::GetCurrentProgram();
    m_pReturnOperator = pProgram->AddOperator(otSubroutineReturn, 0, pStack.GetID());
    m_pStartOperator->m_pLinkedOperator = m_pReturnOperator;
}

//------------------------------------------------------------------------------
//
void
C_Subroutine::End()
{
    WarpAssert(m_pStartOperator);   // End() called without Start()
}

//------------------------------------------------------------------------------
C_Subroutine::~C_Subroutine()
{
    WarpAssert(m_pStartOperator);   // subroutine code undefined
    WarpAssert(!m_pCallers);
}


