// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of class CMapper.
//
//-----------------------------------------------------------------------------
#include "precomp.h"

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::CMapper
//
//  Synopsis:
//      Constructor.
//
//------------------------------------------------------------------------------
CMapper::CMapper(CProgram * pProgram)
    : m_pProgram(pProgram)
    , m_uVarCount(pProgram->GetVarsCount())
{
    m_prgOffsets = NULL;
    m_ppOperators = m_pProgram->GetOperators();
    m_ppInstructions = m_pProgram->GetInstructions();
    m_uOperatorsCount = m_pProgram->GetOperatorsCount();

    m_uFrameSize = 0;
    m_uFrameAlignment = 0;

    m_pVarsUsedInLoop = NULL;
    m_uBitArraySize = CBitArray::GetSizeInDWords(m_uVarCount);

    m_pMapContext = NULL;

    // initialize register groups
#if WPFGFX_FXJIT_X86
    {
        static const CRegID regsR32[] =
        {
            CRegID(reg_eax),
            CRegID(reg_ebx),
            CRegID(reg_ecx),
            CRegID(reg_edx),
            CRegID(reg_esi),
            CRegID(reg_edi),
            CRegID(reg_ebp)
        };
        m_RegisterGroupGPR.Init(regsR32, m_pProgram->IsEBP_Allowed() ? 7 : 6);
    }

    {
        static const CRegID regsMMX[] =
        {
            CRegID(reg_mm0),
            CRegID(reg_mm1),
            CRegID(reg_mm2),
            CRegID(reg_mm3),
            CRegID(reg_mm4),
            CRegID(reg_mm5),
            CRegID(reg_mm6),
            CRegID(reg_mm7)
        };
        m_RegisterGroupMMX.Init(regsMMX, 8);
    }

    {
        static const CRegID regsXMM[] =
        {
            CRegID(reg_xmm0),
            CRegID(reg_xmm1),
            CRegID(reg_xmm2),
            CRegID(reg_xmm3),
            CRegID(reg_xmm4),
            CRegID(reg_xmm5),
            CRegID(reg_xmm6),
            CRegID(reg_xmm7)
        };
        m_RegisterGroupXMM.Init(regsXMM, 8);
    }

#else //_AMD64_
    {
        static const CRegID regsR64[] =
        {
            CRegID(reg_rax),
            CRegID(reg_rbx),
            CRegID(reg_rcx),
            CRegID(reg_rdx),
            CRegID(reg_rsi),
            CRegID(reg_rdi),
            CRegID(reg_r8),
            CRegID(reg_r9),
            CRegID(reg_r10),
            CRegID(reg_r11),
            CRegID(reg_r12),
            CRegID(reg_r13),
            CRegID(reg_r14),
            CRegID(reg_r15),
            CRegID(reg_rbp)
        };
        m_RegisterGroupGPR.Init(regsR64, m_pProgram->IsEBP_Allowed() ? 15 : 14);
    }

    {
        static const CRegID regsXMM[] =
        {
            CRegID(reg_xmm0),
            CRegID(reg_xmm1),
            CRegID(reg_xmm2),
            CRegID(reg_xmm3),
            CRegID(reg_xmm4),
            CRegID(reg_xmm5),
            CRegID(reg_xmm6),
            CRegID(reg_xmm7),
            CRegID(reg_xmm8),
            CRegID(reg_xmm9),
            CRegID(reg_xmm10),
            CRegID(reg_xmm11),
            CRegID(reg_xmm12),
            CRegID(reg_xmm13),
            CRegID(reg_xmm14),
            CRegID(reg_xmm15),
        };
        m_RegisterGroupXMM.Init(regsXMM, 16);
    }
#endif

}

enum RegHistory : UINT8
{
    rh_unused,
    rh_used,
    rh_scratched,
};

class CMapContext
{
public:
    CMapContext(
        ExtRegState * pRegState,
        CMapContext * pNextInStack,
        COperator * pOpShuffleHolder,
        bool fLoop
        )
        : m_pRegState(pRegState)
        , m_pNextInStack(pNextInStack)
        , m_pOpShuffleHolder(pOpShuffleHolder)
        , m_fLoop(fLoop)
    {
        for (UINT32 u = 0; u < g_uRegsTotal; u++)
            m_history[u] = rh_unused;
    }

    void TraceOperator(COperator const *pOp)
    {
        for (CShuffleRecord * psr = pOp->m_pShuffles; psr; psr = psr->m_pNext)
        {
            CRegID regSrcID = psr->GetRegSrc();
            if (regSrcID.IsDefined())
                ConsiderUsed(regSrcID);

            CRegID regDstID = psr->GetRegDst();
            if (regDstID.IsDefined())
                ConsiderChanged(regDstID);
        }

        if (pOp->m_rOperand1.IsDefined())
        {
            ConsiderUsed(pOp->m_rOperand1);
        }
        if (pOp->m_rOperand2.IsDefined())
        {
            ConsiderUsed(pOp->m_rOperand2);
        }
        if (pOp->m_rOperand3.IsDefined())
        {
            ConsiderUsed(pOp->m_rOperand3);
        }
        if (pOp->m_rResult.IsDefined())
        {
            ConsiderChanged(pOp->m_rResult);
        }
    }

    CMapContext * MergeLoop()
    {
        if (m_pNextInStack)
        {
            for (UINT32 u = 0; u < g_uRegsTotal; u++)
            {
                RegHistory & rhParent = m_pNextInStack->m_history[u];
                if (rhParent == rh_unused)
                    rhParent = m_history[u];
            }
        }
        return m_pNextInStack;
    }

    CMapContext * MergeBranch()
    {
        if (m_pNextInStack)
        {
            for (UINT32 u = 0; u < g_uRegsTotal; u++)
            {
                // If a register is scratched inside bypassed sbuppet, it should be restored
                // in EqualizeBranchRegState so we should not marck scratches.
                // This will not happen however if the register did not contain anything.

                RegHistory rh = m_history[u];
                if (rh == rh_used || (rh == rh_scratched && m_pRegState->m_uVarID[u] == 0))
                {
                    RegHistory & rhParent = m_pNextInStack->m_history[u];
                    if (rhParent == rh_unused)
                        rhParent = rh;
                }
            }
        }
        return m_pNextInStack;
    }

    RegHistory GetRegHistory(CRegID regID) const
    {
        WarpAssert(regID.IsDefined());
        return m_history[regID.Index()];
    }

    COperator * GetEvictionLocation(COperator * pCurrentOp, UINT32 uVarID, CRegID regID)
    {
        if (m_pOpShuffleHolder)
        {
            UINT32 i = regID.Index();
            RegHistory & rh = m_history[i];
            if (rh == rh_unused)
            {
                WarpAssert(!m_pRegState->m_rgfIsInMemory[i]);
                m_pRegState->m_rgfIsInMemory[i] = true;

                COperator * pOpShuffleHolder = m_pOpShuffleHolder;
                if (m_pNextInStack)
                {
                    pOpShuffleHolder = m_pNextInStack->GetEvictionLocation(pOpShuffleHolder, uVarID, regID);
                    if (pOpShuffleHolder == m_pOpShuffleHolder)
                    {
                        m_pNextInStack->ConsiderUsed(regID);
                    }
                }
                return pOpShuffleHolder;
            }
        }
        return pCurrentOp;
    }

private:
    void ConsiderUsed(CRegID regID)
    {
        WarpAssert(regID.IsDefined());
        RegHistory & rh = m_history[regID.Index()];
        if (rh == rh_unused)
            rh = rh_used;
    }

    void ConsiderChanged(CRegID regID)
    {
        WarpAssert(regID.IsDefined());
        RegHistory & rh = m_history[regID.Index()];
        if (rh == rh_unused)
            rh = rh_scratched;
    }

private:
    COperator * const m_pOpShuffleHolder;
    ExtRegState * const m_pRegState;
    CMapContext * const m_pNextInStack;
    bool const m_fLoop;
    RegHistory m_history[g_uRegsTotal];
};


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::MapProgram
//
//  Synopsis:
//      Execute mapping procedure.
//      For every operator in the program allocate particular registers
//      for operand and result values. Mark operators properly when
//      it is necessary to load and/or store registers values in stack
//      frame memory.
//      For every variable figure out whether it needs a slot in stack frame,
//      and allocate it if necessary.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::MapProgram()
{
    HRESULT hr = S_OK;

    CMapContext idleCtx(NULL, NULL, NULL, false);
    m_pMapContext = &idleCtx;

    WarpAssert(m_prgOffsets == NULL);
    m_prgOffsets = reinterpret_cast<UINT32*>(m_pProgram->AllocMem(m_uVarCount * sizeof(UINT32)));
    IFCOOM(m_prgOffsets);

    for (UINT32 i = 0; i < m_uVarCount; i++)
        m_prgOffsets[i] = (UINT32)-1;

    WarpAssert(m_pVarsUsedInLoop == NULL);
    m_pVarsUsedInLoop = (CBitArray*)(m_pProgram->AllocMem(m_uBitArraySize * sizeof(UINT32)));
    IFCOOM(m_pVarsUsedInLoop);
    m_pVarsUsedInLoop->Clear(m_uBitArraySize);

    IFC(m_locator.Init(m_pProgram));
    m_locator.ConsiderSetValue(m_pProgram->GetFramePointerID(), CRegID(gbp));

#if WPFGFX_FXJIT_X86
#else // _AMD64_
    m_locator.ConsiderSetValue(m_pProgram->GetArgument1ID(), CRegID(reg_rcx));
#endif

    for (m_uOpIdx = 0; m_uOpIdx < m_uOperatorsCount; m_uOpIdx++)
    {
        m_pOp = m_ppOperators[m_uOpIdx];

        if (m_pOp->IsLoopStart())
        {
            // MapLoop will handle all the operators in the loop
            // body, including final repeat loop operator.
            IFC(MapLoop());
        }
        else if (m_pOp->IsBranchSplit())
        {
            // MapBranch will handle all the operators in the branch
            // span, including final branch merge operator.
            IFC(MapBranch());
        }
        else if (m_pOp->m_ot == otSubroutineCall)
        {
            IFC(MapSubroutineCall());
        }
        else if (m_pOp->m_ot == otSubroutineStart)
        {
            IFC(MapSubroutineStart());
        }
        else if (m_pOp->m_ot == otSubroutineReturn)
        {
            IFC(MapSubroutineReturn());
        }
        else
        {
            IFC(MapOperator());
        }
    }

    AllocateStackFrame();

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::MapLoop
//
//  Synopsis:
//      Execute mapping procedure for operators between loop start and
//      loop repeat operators, inclusively.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::MapLoop()
{
    HRESULT hr = S_OK;
    const COperator * pOpLinked = (COperator*)(m_pOp->m_pLinkedOperator);
    WarpAssert(pOpLinked);
    UINT32 uOpRepeatLoop = pOpLinked->m_uOrder;

    //
    // The processor enters repeated snippet of code
    // either implicitly from loop prologue (first pass), or after
    // executing the loop (next passes). Preceding codes should provide
    // equal register mapping. We could achieve this by reshuffling
    // registers at the end of the loop, but this is not desirable
    // because of expense. Instead we prefer to reshuffle before entering
    // the loop; it is better because this code is not executed repeatedly.
    //
    // We'll snap the register state at start and then modify it
    // whenever it is possible to pull shuffling instruction out of loop.
    //

    ExtRegState regStateAtStart;
    m_locator.SnapRegState(&regStateAtStart);

    CMapContext ctx(&regStateAtStart, m_pMapContext, m_pOp, true);
    m_pMapContext = &ctx;

    for (m_uOpIdx++; m_uOpIdx < uOpRepeatLoop; m_uOpIdx++)
    {
        m_pOp = m_ppOperators[m_uOpIdx];

        if (m_pOp->IsLoopStart())
        {
            // Nested MapLoop will handle all the operators in the loop
            // body, including final repeat loop operator.
            IFC(MapLoop());
        }
        else if (m_pOp->IsBranchSplit())
        {
            // MapBranch will handle all the operators in the branch
            // span, including final branch merge operator.
            IFC(MapBranch());
        }
        else if (m_pOp->m_ot == otSubroutineCall)
        {
            IFC(MapSubroutineCall());
        }
        else
        {
            IFC(MapOperator());
        }
    }

    m_pOp = m_ppOperators[m_uOpIdx];
    WarpAssert(m_pOp->IsLoopRepeat());

    // Shuffle registers to get proper state to jump to loop repeat point
    IFC(EqualizeLoopRegState(&regStateAtStart));

    // Consider variables that go out of scope after repeat loop operator
    {
        const COperator *pOp = m_ppOperators[m_uOpIdx + 1];
        UINT32 uSpanIdx = pOp->m_uSpanIdx;
        const OpSpan * pSpan = m_pProgram->GetSpanGraph() + uSpanIdx;
        const CBitArray * pVarsInUse = pSpan->m_pVarsInUseBefore;
        m_locator.ConsiderScope(pVarsInUse);
    }

    ctx.TraceOperator(m_pOp);

    m_pMapContext = ctx.MergeLoop();

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::MapBranch
//
//  Synopsis:
//      Execute mapping procedure for operators between branch split and
//      branch merge operators, inclusively.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::MapBranch()
{
    HRESULT hr = S_OK;
    const COperator * pOpLinked = (COperator*)(m_pOp->m_pLinkedOperator);
    WarpAssert(pOpLinked);
    UINT32 uOpBranchMerge = pOpLinked->m_uOrder;

    //
    // The processor enters merge point either after executing the whole
    // span affected by condition or bypassing it. We should provide
    // equal register mapping for both variants.
    //
    // We'll snap the register state at start and then modify it
    // whenever it is possible to pull shuffling instruction out
    // of bypassed code snippet.
    //
    ExtRegState regStateAtSplit;
    m_locator.SnapRegState(&regStateAtSplit);

    CMapContext ctx(&regStateAtSplit, m_pMapContext, m_pOp, false);
    m_pMapContext = &ctx;

    // Current operator is split one; it has an argument so we need to map it.
    IFC(MapOperator());

    for (m_uOpIdx++; m_uOpIdx < uOpBranchMerge; m_uOpIdx++)
    {
        m_pOp = m_ppOperators[m_uOpIdx];

        if (m_pOp->IsLoopStart())
        {
            // MapLoop will handle all the operators in the loop
            // body, including final repeat loop operator.
            IFC(MapLoop());
        }
        else if (m_pOp->IsBranchSplit())
        {
            // Nested MapBranch will handle all the operators in the branch
            // span, including final branch merge operator.
            IFC(MapBranch());
        }
        else if (m_pOp->m_ot == otSubroutineCall)
        {
            IFC(MapSubroutineCall());
        }
        else
        {
            IFC(MapOperator());
        }
    }

    m_pOp = m_ppOperators[m_uOpIdx];
    WarpAssert(m_pOp->IsBranchMerge());

    // Consider variables that go out of scope after branch merge operator
    {
        UINT32 uSpanIdx = m_pOp->m_uSpanIdx;
        OpSpan * pSpan = m_pProgram->GetSpanGraph() + uSpanIdx;
        const CBitArray * pVarsInUse = pSpan->m_pVarsInUseAfter;

        // Shuffle registers at branch merge to get their state compatible
        // with the one that appears on branch bypass
        IFC(EqualizeBranchRegState(&regStateAtSplit, pVarsInUse));
    }

    ctx.TraceOperator(m_pOp);

    m_pMapContext = ctx.MergeBranch();

Cleanup:
    return hr;
}

__checkReturn HRESULT
CMapper::MapSubroutineCall()
{
    HRESULT hr = S_OK;

    IFC(FreeRegs());

    IFC(MapOperator());

    IFC(FreeRegs());

    // Assuming for now that subroutine will free all the registers too.
    // We only need to take care of m_locator.

    UINT32 uSpanIdx = m_pOp->m_uSpanIdx;
    const OpSpan * pNextSpan = m_pProgram->GetSpanGraph() + uSpanIdx + 1;
    const CBitArray * pUsage = pNextSpan->m_pVarsInUseBefore;

    m_locator.Setup(pUsage);

Cleanup:
    return hr;
}

__checkReturn HRESULT
CMapper::MapSubroutineStart()
{
    HRESULT hr = S_OK;

    // Assuming for now that subroutine starts with empty registers.
    IFC(FreeRegs());

    UINT32 uSpanIdx = m_pOp->m_uSpanIdx;
    const OpSpan * pSpan = m_pProgram->GetSpanGraph() + uSpanIdx;
    const CBitArray * pUsage = pSpan->m_pVarsInUseBefore;

    m_locator.Setup(pUsage);

Cleanup:
    return hr;
}

__checkReturn HRESULT
CMapper::MapSubroutineReturn()
{
    HRESULT hr = S_OK;

    // For now subroutine returns with empty registers.
    IFC(FreeRegs());

    IFC(MapOperator());

    IFC(FreeRegs());

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::EqualizeLoopRegState
//
//  Synopsis:
//      Compose the sequence of moving data to/from/across register
//      that will cause desired content of all the registers.
//
//      This routine compares current register content (that's given
//      in m_locator) with desired content (given in pDesiredRegState).
//
//      Resulting data are represented in a chain of CShuffleRecord instances
//      hooked up to the current operator that is "LoopRepeat". This causes
//      assembling moving instructions that are executed before jumping
//      to the loop start point.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::EqualizeLoopRegState(ExtRegState * const  pDesiredRegState)
{
    HRESULT hr = S_OK;

    for (UINT32 i = 0; i < g_uRegsTotal; i++)
    {
        CRegID regID = CRegID(i);
        RegHistory rh = m_pMapContext->GetRegHistory(regID);
        if (rh == rh_unused)
            continue;

        UINT32 uVar = m_locator.GetVarID(regID);
        if (rh == rh_scratched)
        {
            if (uVar)
            {
                WarpAssert(regID == m_locator.GetRegID(uVar));
                if (!m_locator.IsInMemory(uVar))
                {
                    IFC(SaveReg(m_pOp, uVar, regID));
                    m_locator.ConsiderSaveReg(regID);
                }
            }
            continue;
        }

        UINT32 uVarDesired = pDesiredRegState->m_uVarID[i];
        bool fInMemory = pDesiredRegState->m_rgfIsInMemory[i];

        if (uVar == uVarDesired)
        {
            // A luck happened: register is occupied with the required value.
            // However it might be a difference on loop start point: whetner this
            // variable is also in memory or not.
            if (uVar && fInMemory && !m_locator.IsInMemory(uVar))
            {
                IFC(SaveReg(m_pOp, uVar, regID));
                m_locator.ConsiderSaveReg(regID);
            }
            continue;
        }

        if (uVar)
        {
            WarpAssert(regID == m_locator.GetRegID(uVar));
            if (!m_locator.IsInMemory(uVar))
            {
                IFC(SaveReg(m_pOp, uVar, regID));
                m_locator.ConsiderSaveReg(regID);
            }
            m_locator.ConsiderFreeReg(regID);
        }

        if (uVarDesired)
        {
            if (m_locator.IsInRegister(uVarDesired))
            {
                CRegID currentRegID = m_locator.GetRegID(uVarDesired);

                IFC(MoveReg(m_pOp, regID, currentRegID, m_pProgram->GetVarType(uVarDesired)));
                m_locator.ConsiderMoveRegToReg(regID, currentRegID);

                if (fInMemory && !m_locator.IsInMemory(uVarDesired))
                {
                    IFC(SaveReg(m_pOp, uVar, regID));
                    m_locator.ConsiderSaveReg(regID);
                }
            }
            else
            {
                IFC(LoadReg(m_pOp, regID, uVarDesired));
                m_locator.ConsiderLoadReg(uVarDesired, regID);
            }
        }
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::EqualizeBranchRegState
//
//  Synopsis:
//      Compose the sequence of moving data to/from/across resigter
//      that will cause desired content of all the registers.
//
//      This routine compares current register content (that's given
//      in m_locator - i.e. on main flow) with content that'll appear
//      on alternative flow (given in pAltRegState).
//
//      Resulting data are represented in a chain of CShuffleRecord instances
//      hooked up to the current operator that is branch merge. This causes
//      generating instructions that are executed before the merge point.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::EqualizeBranchRegState(ExtRegState * const pAltRegState, const CBitArray * pVarsInUse)
{
    HRESULT hr = S_OK;

    for (UINT32 i = 0; i < g_uRegsTotal; i++)
    {
        CRegID regID = CRegID(i);
        UINT32 uVar = m_locator.GetVarID(regID);
        UINT32 uAltVar = pAltRegState->m_uVarID[i];
        bool fInMemory = pAltRegState->m_rgfIsInMemory[i];

        if (uVar && !pVarsInUse->Get(uVar))
        {
            m_locator.ConsiderVarOutOfScope(uVar);
            WarpAssert(m_locator.GetVarID(regID) == 0);
            uVar = 0;
        }

        if (uVar == uAltVar)
        {
            // A luck happened: register is occupied with the required value.
            // However it might be a difference on branch merge point: whetner this
            // variable is also in memory or not.
            if (uVar && !fInMemory)
            {
                m_locator.ConsiderOutOfMemory(uVar);
            }
        }
        else
        {
            if (uVar)
            {
                WarpAssert(regID == m_locator.GetRegID(uVar));
                if (!m_locator.IsInMemory(uVar))
                {
                    IFC(SaveReg(m_pOp, uVar, regID));
                    m_locator.ConsiderSaveReg(regID);
                }
                m_locator.ConsiderFreeReg(regID);
            }

            if (uAltVar && pVarsInUse->Get(uAltVar))
            {
                if (m_locator.IsInRegister(uAltVar))
                {
                    CRegID currentRegID = m_locator.GetRegID(uAltVar);

                    IFC(MoveReg(m_pOp, regID, currentRegID, m_pProgram->GetVarType(uAltVar)));
                    m_locator.ConsiderMoveRegToReg(regID, currentRegID);
                }
                else
                {
                    WarpAssert(m_locator.IsInMemory(uAltVar));
                    IFC(LoadReg(m_pOp, regID, uAltVar));
                    m_locator.ConsiderLoadReg(uAltVar, regID);

                }

                if (!fInMemory)
                {
                    m_locator.ConsiderOutOfMemory(uAltVar);
                }
            }
        }
    }

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::AllocateStackFrame
//
//  Synopsis:
//      For each variable detect whether it was ever stored in stack frame
//      memory; if so then reserve space for it.
//
//  Stack format notes:
//      The natural design of stack frame assumes that it contains call
//      parameters, return address and all the local variables.
//      Unfortunately this it not feasible because generated program needs
//      stack frame aligned to 16-bytes boundary.
//      Therefore we operate two stack frames: "primary" and "current".
//      "Current" stack frame is referred to as just "stack frame".
//
//      Primary stack frame is the one pointed by esp when the generated
//      code starts. It is used to save registers to saticfy calling
//      conventions. Pointer to primary stack is then stored in ebp register,
//      while esp is used to allocate aligned space.
//      See CAssembleContext::AssemblePrologue for details.
//
//      Pointer to primary stack is pre-defined variable. Mapper can store it
//      in current stack frame, so far obtaining ebp for other operations.
//
//------------------------------------------------------------------------------
void
CMapper::AllocateStackFrame()
{
    // Start allocating stack slots from biggest variables
    // that require 16-byte aligning.
    UINT32 uOffset = 0;
    for (UINT32 uVarID = 0; uVarID < m_uVarCount; uVarID++)
    {
        VariableType vt = m_pProgram->GetVarType(uVarID);
        // sometimes XmmF1 is casted to XmmF4 producing
        // AV of misaligned data. Allocate 16 bytes for now.
        if (vt == vtXmm || vt == vtXmmF4 || vt == vtXmmF1)
        {
            if (m_locator.WasInMemory(uVarID))
            {
                // this value needs a slot; allocate it
                m_prgOffsets[uVarID] = uOffset;
                uOffset += 16;//sizeof(__m128i);
            }
        }
    }

    WarpAssert(m_uFrameAlignment == 0);
    if (uOffset)
    {
        // If we have xmm slots in stack frame then
        // stack frame should be aligned to 16 bits.
        m_uFrameAlignment = 0xF;
    }

    UINT32 uOffsetSaved = uOffset;

    // do the same for 8- and 4-bytes variables
    for (UINT32 uVarID = 0; uVarID < m_uVarCount; uVarID++)
    {
        VariableType vt = m_pProgram->GetVarType(uVarID);
#if WPFGFX_FXJIT_X86
        if (vt == vtMm)
#else //_AMD64_
        if (vt == vtUINT64 || vt == vtPointer)
#endif
        {
            if (m_locator.WasInMemory(uVarID))
            {
                m_prgOffsets[uVarID] = uOffset;
                uOffset += 8;
            }
        }
    }

    if (uOffsetSaved != uOffset)
    {
        // If we have any 64-bit slots in stack frame then
        // we'll align stack frame to 8 bits.
        m_uFrameAlignment |= 0x7;
    }

    for (UINT32 uVarID = 0; uVarID < m_uVarCount; uVarID++)
    {
        VariableType vt = m_pProgram->GetVarType(uVarID);
#if WPFGFX_FXJIT_X86
        // 
        // sometimes XmmF1 is casted to XmmF4 producing
        // AV of misaligned data. Allocate 16 bytes for now.
        if (vt == vtUINT32 || vt == vtPointer /*|| vt == vtXmmF1*/)
#else //_AMD64_
        if (vt == vtUINT32 /*|| vt == vtXmmF1*/)
#endif
        {
            if (m_locator.WasInMemory(uVarID))
            {
                m_prgOffsets[uVarID] = uOffset;
                uOffset += sizeof(UINT32*);
            }
        }
    }

    // all the variables are inspected; now we know
    // stack frame size and offset for each variable.
    m_uFrameSize = uOffset;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::GetVarOffset
//
//  Synopsis:
//      Returns the offset from stack frame bottom to the location
//      of given variable.
//      This routine should not be called for the variable that
//      has not allocated space in stack frame.
//
//------------------------------------------------------------------------------
UINT32
CMapper::GetVarOffset(UINT32 uVarID) const
{
    WarpAssert(uVarID < m_uVarCount);

    UINT32 uOffset = m_prgOffsets[uVarID];
    WarpAssert(uOffset < m_uFrameSize);

    return uOffset;
}

void
CMapper::SetAllocException(CRegID regID)
{
    if (!regID.IsDefined())
        return;

    for (UINT32 u = 0; u < m_uAllocExceptionCount; u++)
    {
        if (m_regAllocExceptions[u] == regID)
        {
            return;
        }
    }
    WarpAssert (m_uAllocExceptionCount < sc_uMaxAllocExceptionCount);
    m_regAllocExceptions[m_uAllocExceptionCount++] = regID;
}

bool
CMapper::IsAllocException(CRegID regID) const
{
    for (UINT32 u = 0; u < m_uAllocExceptionCount; u++)
    {
        if (m_regAllocExceptions[u] == regID)
        {
            return true;
        }
    }
    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::AllocRegister
//
//  Synopsis:
//      Choose the register of given type (XMM/MMX/R32) required for some
//      operation.
//      The routine first looks for the resigter that's not currently occupied.
//      If all the registers are busy, it chooses one of them using the criteria
//      of "biggest distance to consumer".
//
//------------------------------------------------------------------------------
CRegID
CMapper::AllocRegister(RegisterGroup * prg)

{
    UINT32 uStartIndex = prg->m_uRotation;
    UINT32 uNextIndex = prg->Next(uStartIndex);
    prg->m_uRotation = uNextIndex;

    CRegID regsToEvict[g_uRegsInGroup];
    UINT32 uEvictCount = 0;

    for (UINT32 u = uNextIndex; ; u = prg->Next(u))
    {
        CRegID regCandidate = prg->m_prgRegs[u];
        if (!IsAllocException(regCandidate))
        {
            UINT32 uVarID = m_locator.GetVarID(regCandidate);
            if (uVarID == 0)
                return regCandidate;

            if(g_uRegsInGroup==uEvictCount)
                break;

            regsToEvict[uEvictCount++] = regCandidate;
        }

        if (u == uStartIndex) break;
    }

    // All the registers are busy, choose one of them that's least desired
    WarpAssert(uEvictCount > 0);

    UINT32 uBiggestDistance = 0;
    UINT32 uVictimIndex = 0;

    for (UINT32 u = 0; u < uEvictCount; u++)
    {
        UINT32 uVarID = m_locator.GetVarID(regsToEvict[u]);
        UINT32 uDistance = m_pProgram->GetDistanceToConsumer(m_pOp, uVarID);
        WarpAssert(uDistance > 0);

        if (uDistance > uBiggestDistance)
        {
            uBiggestDistance = uDistance;
            uVictimIndex = u;
        }
    }
    return regsToEvict[uVictimIndex];
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::AcquireRegister
//
//  Synopsis:
//      Allocate the register via AllocRegister(), see whether it is busy,
//      schedule saving if necessary.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::AcquireRegister(RegisterGroup * prg, CRegID &regID, UINT32 uVarToLoad)
{
    HRESULT hr = S_OK;

    regID = AllocRegister(prg);

    // See what is currently contained in this register.
    UINT32 uVarID = m_locator.GetVarID(regID);

    if (uVarID != 0)
    {
        WarpAssert(uVarID != m_pOp->m_vResult  );
        WarpAssert(uVarID != m_pOp->m_vOperand1);
        WarpAssert(uVarID != m_pOp->m_vOperand2);
        WarpAssert(uVarID != m_pOp->m_vOperand3);

        if (!m_locator.IsInMemory(uVarID))
        {
            // We are about to evict the register.
            // See whether this eviction can be moved out of loop.
            COperator * pEvictionHolder = m_pMapContext->GetEvictionLocation(m_pOp, uVarID, regID);
            IFC(SaveReg(pEvictionHolder, uVarID, regID));

            m_locator.ConsiderSaveReg(regID);
        }

        m_locator.ConsiderFreeReg(regID);

    }

    if (uVarToLoad)
    {
        IFC(LoadReg(m_pOp, regID, uVarToLoad));
        m_locator.ConsiderLoadReg(uVarToLoad, regID);
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::FreeRegs
//
//  Synopsis:
//      Compose the sequence of move instructions to get
//      all the registers free.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::FreeRegs()
{
    HRESULT hr = S_OK;

    for (UINT32 i = 0; i < g_uRegsTotal; i++)
    {
        CRegID regID = CRegID(i);
        UINT32 vID = m_locator.GetVarID(regID);

        if (vID)
        {
            WarpAssert(regID == m_locator.GetRegID(vID));

            if (!m_locator.IsInMemory(vID))
            {
                IFC(SaveReg(m_pOp, vID, regID));
                m_locator.ConsiderSaveReg(regID);
            }

            m_locator.ConsiderFreeReg(regID);
        }
    }

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::SaveRegs
//
//  Synopsis:
//      Compose the sequence of move instructions to get
//      all the registers content saved in stack frame memory.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::SaveRegs(const CBitArray * pVars)
{
    HRESULT hr = S_OK;

    for (UINT32 i = 0; i < g_uRegsTotal; i++)
    {
        CRegID regID = CRegID(i);
        UINT32 vID = m_locator.GetVarID(regID);

        if (vID && pVars->Get(vID))
        {
            WarpAssert(regID == m_locator.GetRegID(vID));

            if (!m_locator.IsInMemory(vID))
            {
                IFC(SaveReg(m_pOp, vID, regID));
                m_locator.ConsiderSaveReg(regID);
            }
        }
    }

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMapper::MapOperator
//
//  Synopsis:
//      Executes mapping procedure for single operator.
//      Allocates particular registers for operands and result values.
//      Mark operators properly when it is necessary to load and/or store
//      registers values in stack frame memory.
//------------------------------------------------------------------------------
__checkReturn HRESULT
CMapper::MapOperator()
{
    HRESULT hr = S_OK;

    UINT32 vResult   = m_pOp->m_vResult;
    UINT32 vOperand1 = m_pOp->m_vOperand1;
    UINT32 vOperand2 = m_pOp->m_vOperand2;
    UINT32 vOperand3 = m_pOp->m_vOperand3;

    bool fOperand1InUseAfter = false;
    bool fOperand2InUseAfter = false;
    bool fOperand3InUseAfter = false;

    {
        CInstruction * pIn = m_ppInstructions[m_pOp->m_uOrder];
        WarpAssert(pIn->m_pOperator == m_pOp);

        for (InstructionHook * pHook = pIn->m_pProviders; pHook; pHook = pHook->m_pNext)
        {
            CInstruction * pInProvider = pHook->m_pProvider;
            UINT32 uVarID;
            if (pInProvider->m_pOperator)
            {
                uVarID = pInProvider->m_pOperator->m_vResult;
            }
            else
            {
#ifndef offsetof
#define offsetof(s, m) ((UINT_PTR)(&((s *)NULL)->m))
#endif
                const CConnector * pConnector = (CConnector*)((char*)pInProvider - offsetof(CConnector, m_instruction));
                uVarID = pConnector->m_uVarID;
            }
            WarpAssert(uVarID);

            pInProvider->m_uConsumersCount--;

            bool fReachedZero = (pInProvider->m_uConsumersCount == 0);
            if (!fReachedZero)
            {
                if (m_pOp->m_vOperand1 == uVarID) fOperand1InUseAfter = true;
                if (m_pOp->m_vOperand2 == uVarID) fOperand2InUseAfter = true;
                if (m_pOp->m_vOperand3 == uVarID) fOperand3InUseAfter = true;
            }
        }
    }

    RegisterType rtResult   = m_pProgram->GetRegType(vResult  );
    RegisterType rtOperand1 = m_pProgram->GetRegType(vOperand1);
    RegisterType rtOperand2 = m_pProgram->GetRegType(vOperand2);
    RegisterType rtOperand3 = m_pProgram->GetRegType(vOperand3);

    RegisterGroup * prgResult   = GetRegisterGroup(rtResult  );
    RegisterGroup * prgOperand1 = GetRegisterGroup(rtOperand1);
    RegisterGroup * prgOperand2 = GetRegisterGroup(rtOperand2);
    RegisterGroup * prgOperand3 = GetRegisterGroup(rtOperand3);

    CRegID & rOperand1(m_pOp->m_rOperand1);
    CRegID & rOperand2(m_pOp->m_rOperand2);
    CRegID & rOperand3(m_pOp->m_rOperand3);
    CRegID & rResult  (m_pOp->m_rResult  );

    ClearAllocExceptions();

    if (m_pOp->IsIrregular())
    {
        switch (m_pOp->m_ot)
        {
        case otCall:
            IFC(MapCall());
            break;
        case otUINT32Div:
        case otUINT32Rem:
        case otINT32Div:
        case otINT32Rem:
            IFC(PreAllocRegister(CRegID(gax), vOperand1));
            IFC(PreAllocRegister(CRegID(gdx), 0));
            break;
        case otUINT32ShiftLeft:
        case otUINT32ShiftRight:
        case otINT32ShiftRight:
            IFC(PreAllocRegister(CRegID(gcx), vOperand2));
            break;
        case otXmmStoreNonTemporalMasked:
            IFC(PreAllocRegister(CRegID(gdi), vOperand3));
            break;
        case otXmmBytesBlend:
            IFC(PreAllocRegister(CRegID(reg_xmm0), vOperand3));
            break;
        default:
            NO_DEFAULT;
        }
    }

    rOperand1 = m_locator.GetRegID(vOperand1);
    rOperand2 = m_locator.GetRegID(vOperand2);
    rOperand3 = m_locator.GetRegID(vOperand3);
    rResult   = m_locator.GetRegID(vResult);

    // if any of operands or result are already placed in registers, declare
    // these register "exceptions" so that AllocRegister will not touch them.

    SetAllocException(rResult  );
    SetAllocException(rOperand1);
    SetAllocException(rOperand2);
    SetAllocException(rOperand3);

    bool fOperand1NeedsLoad = false;

    if (vOperand1 != 0)
    {
        // Operand 1 is relevant.

        if (m_pOp->m_ot == otXmmDWordsGetElement)
        {
            // for now otXmmDWordsGetElement is the only
            // operator that needs the operand in memory rather than in register.

            if (!m_locator.IsInMemory(vOperand1))
            {
                WarpAssert(m_locator.IsInRegister(vOperand1));
                rOperand1 = m_locator.GetRegID(vOperand1);

                IFC(SaveReg(m_pOp, vOperand1, rOperand1));
                m_locator.ConsiderSaveReg(rOperand1);
            }
        }
        else
        {
            // Allocate the register for it unless it's already in register.
            if (!m_locator.IsInRegister(vOperand1))
            {
                WarpAssert(m_locator.IsInMemory(vOperand1));
                if (m_pOp->m_refType == RefType_Direct && m_pOp->CanTakeOperand1FromMemory())
                {
                    //
                    // Only unary operators CanTakeOperand1FromMemory
                    //
                    WarpAssert(vOperand2 == 0);

                    //
                    // Do not allocate register for operand1, instead use memory form
                    // of instruction, like following:
                    // pshuflw xmm0, oword ptr [esp + offset]
                    //
                    WarpAssert(!rOperand1.IsDefined());
                }
                else
                {
                    IFC(AcquireRegister(prgOperand1, rOperand1, vOperand1));

                    SetAllocException(rOperand1);

                    fOperand1NeedsLoad = true;

                    if (vOperand2 == vOperand1)
                        rOperand2 = rOperand1;

                    if (vOperand3 == vOperand1)
                        rOperand3 = rOperand1;

                    if (vResult == vOperand1)
                        rResult = rOperand1;
                }
            }
        }
    }

    if (vOperand2 != 0)
    {
        // Operand 2 is relevant.

        // Allocate the register for operand 2 unless it's already in register
        // or the operator can take it directly from memory.
        if (!m_locator.IsInRegister(vOperand2))
        {
            WarpAssert(m_locator.IsInMemory(vOperand2));

            if (m_pOp->m_refType == RefType_Direct && m_pOp->CanTakeOperand2FromMemory())
            {
                // take it from memory
                WarpAssert(!rOperand2.IsDefined());
            }
            else
            {
                IFC(AcquireRegister(prgOperand2, rOperand2, vOperand2));

                SetAllocException(rOperand2);

                if (vOperand3 == vOperand2)
                    rOperand3 = rOperand2;

                if (vResult == vOperand2)
                    rResult = rOperand2;
            }
        }
    }

    if (vOperand3 != 0)
    {
        // Operand 3 is relevant.

        // Allocate the register for operand 3 unless it's already in register.
        if (!m_locator.IsInRegister(vOperand3))
        {
            WarpAssert(m_locator.IsInMemory(vOperand3));

            IFC(AcquireRegister(prgOperand3, rOperand3, vOperand3));

            SetAllocException(rOperand3);

            if (vResult == vOperand3)
                rResult = rOperand3;
        }
    }

    if (vResult != 0)
    {
        // The operator has result value, so we need a register for it.
        // It might happen that the result is directed to the variable that
        // also serves here as one of operands. If so, then we'll have
        // m_rResult already defined. Otherwise, we need to decide
        // which register the result should go to.

        if (m_pOp->IsIrregular())
        {
            switch (m_pOp->m_ot)
            {
            case otCall:
                rResult = CRegID(gax);
                break;
            case otUINT32Div:
            case otINT32Div:
                WarpAssert(rOperand1 == gax);
                rResult = CRegID(gax);
                if (vOperand1 != vResult && fOperand1InUseAfter && !m_locator.IsInMemory(vOperand1))
                {
                    IFC(SaveReg(m_pOp, vOperand1, rResult));
                    m_locator.ConsiderSaveReg(rResult);
                }
                break;
            case otUINT32Rem:
            case otINT32Rem:
                WarpAssert(rOperand1 == gax);
                rResult = CRegID(gdx);
                if (vOperand1 != vResult && fOperand1InUseAfter && !m_locator.IsInMemory(vOperand1))
                {
                    IFC(SaveReg(m_pOp, vOperand1, rResult));
                    m_locator.ConsiderSaveReg(rResult);
                }
                break;
            }
        }

        if (!rResult.IsDefined())
        {
            if (vOperand1 != 0 && rtOperand1 == rtResult && !fOperand1InUseAfter && rOperand1.IsDefined())
            {
                // Operand1 and Result will share the register
                rResult = rOperand1;
            }
            else if (vOperand2 != 0 && rtOperand2 == rtResult && !fOperand2InUseAfter && rOperand2.IsDefined())
            {
                // Operand2 and Result will share the register
                rResult = rOperand2;
            }
            else if (vOperand1 != 0 && rtOperand1 == rtResult && fOperand1NeedsLoad && rOperand1.IsDefined())
            {
                // Operand1 and Result will share the register
                rResult = rOperand1;
            }
            else
            {
                IFC(AcquireRegister(prgResult, rResult, 0));
            }
        }

        WarpAssert(rResult.IsDefined());

#if WPFGFX_FXJIT_X86
#else // _AMD64_
        //
        // The lack of checking for RefType_Direct on 32-bit platform
        // does not look reasonable. However many tests slow down on
        // an attempt to introduce it. This should be investigated.
        //
        if (m_pOp->m_refType == RefType_Direct)
#endif
        if (rResult != rOperand1 && rResult == rOperand2 && !m_pOp->CanSwapOperands())
        {
            // Bad luck. We've decided to use same register for Operand2 and Result,
            // but operator can't do so because it can't swap operands.
            // This is like a desire to calculate (eax - ebx) and store result in ebx.
            // The command is "sub eax,ebx" but result gets to register allocated
            // for operand 1, i.e. eax.
            // So far - rethink:
            rResult = rOperand1;

            if (fOperand1InUseAfter && !m_locator.IsInMemory(vOperand1))
            {
                IFC(SaveReg(m_pOp, vOperand1, rOperand1));
                m_locator.ConsiderSaveReg(rOperand1);
            }
        }
    }

    m_pMapContext->TraceOperator(m_pOp);

    // Take care of variables life time.
    // If anything gets out of scope, registers should be considered free.
    if (vOperand1 != 0 && !fOperand1InUseAfter)
    {
        m_locator.ConsiderVarOutOfScope(vOperand1);
    }

    if (vOperand2 != 0 && !fOperand2InUseAfter)
    {
        m_locator.ConsiderVarOutOfScope(vOperand2);
    }

    if (vOperand3 != 0 && !fOperand3InUseAfter)
    {
        m_locator.ConsiderVarOutOfScope(vOperand3);
    }

    if (vResult != 0)
    {
        // Such operator should be excluded by CProgram::RemoveUnused().
        WarpAssert(m_pOp->m_pConsumers || m_pOp->HasOutsideEffect() || m_pOp->CalculatesZF() || m_pOp->IsControl());

        m_locator.ConsiderSetValue(vResult, rResult);
    }

Cleanup:
    return hr;
}
__checkReturn HRESULT
CMapper::MapCall()
{
    HRESULT hr = S_OK;

#if WPFGFX_FXJIT_X86

    IFC(FreeRegister(CRegID(reg_eax)));
    IFC(FreeRegister(CRegID(reg_ecx)));
    IFC(FreeRegister(CRegID(reg_edx)));

    IFC(FreeRegister(CRegID(reg_mm0)));
    IFC(FreeRegister(CRegID(reg_mm1)));
    IFC(FreeRegister(CRegID(reg_mm2)));
    IFC(FreeRegister(CRegID(reg_mm3)));
    IFC(FreeRegister(CRegID(reg_mm4)));
    IFC(FreeRegister(CRegID(reg_mm5)));
    IFC(FreeRegister(CRegID(reg_mm6)));
    IFC(FreeRegister(CRegID(reg_mm7)));

    IFC(FreeRegister(CRegID(reg_xmm0)));
    IFC(FreeRegister(CRegID(reg_xmm1)));
    IFC(FreeRegister(CRegID(reg_xmm2)));
    IFC(FreeRegister(CRegID(reg_xmm3)));
    IFC(FreeRegister(CRegID(reg_xmm4)));
    IFC(FreeRegister(CRegID(reg_xmm5)));
    IFC(FreeRegister(CRegID(reg_xmm6)));
    IFC(FreeRegister(CRegID(reg_xmm7)));

#else // _AMD64_

    IFC(FreeRegister(CRegID(reg_rax)));
    IFC(FreeRegister(CRegID(reg_rcx)));
    IFC(FreeRegister(CRegID(reg_rdx)));
    IFC(FreeRegister(CRegID(reg_r8 )));
    IFC(FreeRegister(CRegID(reg_r9 )));
    IFC(FreeRegister(CRegID(reg_r10)));
    IFC(FreeRegister(CRegID(reg_r11)));

    IFC(FreeRegister(CRegID(reg_xmm0 )));
    IFC(FreeRegister(CRegID(reg_xmm1 )));
    IFC(FreeRegister(CRegID(reg_xmm2 )));
    IFC(FreeRegister(CRegID(reg_xmm3 )));
    IFC(FreeRegister(CRegID(reg_xmm4 )));
    IFC(FreeRegister(CRegID(reg_xmm5 )));

#endif

    // get call parameter in ecx/rcx
    {
        UINT32 uVarID = m_pOp->m_vOperand1;
        WarpAssert(uVarID != 0);

        CRegID regID(gcx);
        WarpAssert(m_locator.GetVarID(regID) == 0);

        if (m_locator.IsInRegister(uVarID))
        {
            CRegID currentRegID = m_locator.GetRegID(uVarID);

            IFC(MoveReg(m_pOp, regID, currentRegID, m_pProgram->GetVarType(uVarID)));
        }
        else
        {
            IFC(LoadReg(m_pOp, regID, uVarID));
        }
    }

Cleanup:
    return hr;
}

//-----------------------------------------------------------------------------
// If the register is busy then save it in stack frame.
// This routine serves MapCall() that needs lots of register to be saved
// to prevent the content by changing by callee routine.
// It does not store register in exception list.
__checkReturn HRESULT
CMapper::FreeRegister(CRegID regID)
{
    HRESULT hr = S_OK;

    // Look what is currently contained in the register
    UINT32 uCurrentVarID = m_locator.GetVarID(regID);
    if (uCurrentVarID)
    {
        // the register is occupied by some variable
        WarpAssert(regID == m_locator.GetRegID(uCurrentVarID));

        if (!m_locator.IsInMemory(uCurrentVarID))
        {
            IFC(SaveReg(m_pOp, uCurrentVarID, regID));
            m_locator.ConsiderSaveReg(regID);
        }

        m_locator.ConsiderFreeReg(regID);
    }


Cleanup:
    return hr;
}


//-----------------------------------------------------------------------------
// Ensure desired variable to be in a desired register.
// Put a register into exception list so that subsequent calls to AllocRegister
// would not touch it.
__checkReturn HRESULT
CMapper::PreAllocRegister(CRegID regID, UINT32 uVarID)
{
    HRESULT hr = S_OK;

    // Look what is currently contained in the register
    UINT32 uCurrentVarID = m_locator.GetVarID(regID);
    if (uCurrentVarID == uVarID)
    {
        // Desired register already contains desired variable,
        // or (if given uVarID is zero) the register is not occupied.
        // In both cases do nothing.
    }
    else
    {
        if (uCurrentVarID)
        {
            // the register is occupied by some variable
            WarpAssert(regID == m_locator.GetRegID(uCurrentVarID));

            if (!m_locator.IsInMemory(uCurrentVarID))
            {
                IFC(SaveReg(m_pOp, uCurrentVarID, regID));
                m_locator.ConsiderSaveReg(regID);
            }

            m_locator.ConsiderFreeReg(regID);
        }

        if (uVarID)
        {
            // Get uVarID loaded into regID.

            if (m_locator.IsInRegister(uVarID))
            {
                CRegID currentRegID = m_locator.GetRegID(uVarID);

                IFC(MoveReg(m_pOp, regID, currentRegID, m_pProgram->GetVarType(uVarID)));
                m_locator.ConsiderMoveRegToReg(regID, currentRegID);
            }
            else
            {
                IFC(LoadReg(m_pOp, regID, uVarID));

                m_locator.ConsiderLoadReg(uVarID, regID);
            }
        }
    }
    SetAllocException(regID);

Cleanup:
    return hr;
}


__checkReturn HRESULT
CMapper::SaveReg(COperator * pOp, UINT32 uVarID, CRegID regSrc)
{
    HRESULT hr = S_OK;
    UINT8 * pMem = m_pProgram->AllocMem(sizeof(CShuffleRecord));
    IFCOOM(pMem);
    VariableType vt = m_pProgram->GetVarType(uVarID);
    CShuffleRecord * psr = new(pMem) CShuffleRecord(uVarID, regSrc, vt);
    HookShuffleRecord(pOp, psr);

Cleanup:
    return hr;
}

__checkReturn HRESULT
CMapper::LoadReg(COperator * pOp, CRegID regDst, UINT32 uVarID)
{
    HRESULT hr = S_OK;
    UINT8 * pMem = m_pProgram->AllocMem(sizeof(CShuffleRecord));
    IFCOOM(pMem);
    VariableType vt = m_pProgram->GetVarType(uVarID);
    CShuffleRecord * psr = new(pMem) CShuffleRecord(regDst, uVarID, vt);
    HookShuffleRecord(pOp, psr);

Cleanup:
    return hr;
}

__checkReturn HRESULT
CMapper::MoveReg(COperator * pOp, CRegID regDst, CRegID regSrc, VariableType vt)
{
    HRESULT hr = S_OK;
    UINT8 * pMem = m_pProgram->AllocMem(sizeof(CShuffleRecord));
    IFCOOM(pMem);
    CShuffleRecord * psr = new(pMem) CShuffleRecord(regDst, regSrc, vt);
    HookShuffleRecord(pOp, psr);

Cleanup:
    return hr;
}

void
CMapper::HookShuffleRecord(COperator * pOp, CShuffleRecord * psr)
{
    CShuffleRecord ** pp;
    for (pp = &pOp->m_pShuffles; *pp; pp = &(*pp)->m_pNext) {}
    *pp = psr;
}


