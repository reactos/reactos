// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of CProgram::Shuffle.
//
//-----------------------------------------------------------------------------
#include "precomp.h"

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::Shuffle
//
//  Synopsis:
//      Reorder operators in the program to allow CPU to do more
//      things in parallel.
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::Shuffle()
{
    HRESULT hr = S_OK;

    //
    // Build local dependency graph:
    // - ignore all cross-span dependencies;
    // - take into account implicit dependencies.
    //
    // After building the list headed in COperator::m_pDependents
    // enumerates all the operators in the span that can't be
    // executed before this operator.
    //

    // look at explicit and implicit dependencies via variables
    for (UINT32 uOp = 0; uOp < m_uOperatorsCount; uOp++)
    {
        COperator *pOperator = m_prgOperators[uOp];
        if (!pOperator->IsControl())
        {
            UINT32 uVar1 = pOperator->m_vOperand1;
            if (uVar1)
            {
                IFC(CheckImplicitDependencies(pOperator, uVar1));

                UINT32 uVar2 = pOperator->m_vOperand2;
                if (uVar2)
                {
                    if (uVar2 != uVar1)
                    {
                        IFC(CheckImplicitDependencies(pOperator, uVar2));
                    }
                    UINT32 uVar3 = pOperator->m_vOperand3;
                    if (uVar3)
                    {
                        if (uVar3 != uVar1 && uVar3 != uVar2)
                        {
                            IFC(CheckImplicitDependencies(pOperator, uVar3));
                        }
                    }
                }
            }
        }

        if (pOperator->ConsumesZF())
        {
            IFC(CheckFlagsDependencies(pOperator));
        }
    }

    if (!m_fEnableMemShuffling)
    {
        // look for implicit dependencies via external memory - i.e.
        // a memory outside of stack frame.
        COperator * pLastOutsideMemoryAware = NULL;
        for (UINT32 uOp = 0; uOp < m_uOperatorsCount; uOp++)
        {
            COperator *pOperator = m_prgOperators[uOp];

            bool fIsOutsideMemoryAware = pOperator->HasOutsideDependency() || pOperator->HasOutsideEffect();
            if (!fIsOutsideMemoryAware)
            {
                // look for implicit dependencies
                if (pOperator->m_refType == RefType_Index_1 ||
                    pOperator->m_refType == RefType_Index_2 ||
                    pOperator->m_refType == RefType_Index_4 ||
                    pOperator->m_refType == RefType_Index_8 ||
                    pOperator->m_refType == RefType_Base)
                    fIsOutsideMemoryAware = true;
            }

            if (fIsOutsideMemoryAware)
            {
                if (pLastOutsideMemoryAware && pLastOutsideMemoryAware->m_uSpanIdx == pOperator->m_uSpanIdx)
                {
                    IFC(AddHook(pLastOutsideMemoryAware, pOperator));
                }
                pLastOutsideMemoryAware = pOperator;
            }
        }
    }

    for (UINT32 uSpan = 0; uSpan < m_uSpanCount; uSpan++)
    {
        OpSpan * pSpan = m_pSpanGraph + uSpan;
        IFC(ShuffleSpan(pSpan));
    }

Cleanup:
    return hr;
}

__checkReturn HRESULT
CProgram::CheckImplicitDependencies(COperator *pOperator, UINT32 varID)
{
    HRESULT hr = S_OK;

    UINT32 uSpanIdx = pOperator->m_uSpanIdx;
    UINT32 uOrder = pOperator->m_uOrder;

    for (COperator * pProvider = m_prgVarSources[varID]; pProvider; pProvider = pProvider->m_pNextVarProvider)
    {
        if (pProvider->m_uSpanIdx == uSpanIdx)
        {
            if (pProvider->m_uOrder < uOrder)
            {
                IFC(AddHook(pProvider, pOperator));
            }
            else if (pProvider->m_uOrder > uOrder)
            {
                IFC(AddHook(pOperator, pProvider));
            }
        }
    }

Cleanup:
    return hr;
}

__checkReturn HRESULT
CProgram::CheckFlagsDependencies(const COperator *pOperator)
{
    HRESULT hr = S_OK;

    UINT32 uSpanIdx = pOperator->m_uSpanIdx;
    UINT32 uOrder = pOperator->m_uOrder;

    COperator *pProvider = NULL;

    for(UINT32 uOp = uOrder; uOp != 0;)
    {
        uOp--;
        COperator *pPrevious = m_prgOperators[uOp];
        if (pPrevious->m_uSpanIdx != uSpanIdx)
        {
            // reached span boundary
            WarpAssert(pPrevious->m_uSpanIdx < uSpanIdx);
            break;
        }

        if (pPrevious->ChangesZF())
        {
            if (pProvider == NULL)
            {
                // there should be no operators affecting ZF between ZF provider and consumer
                WarpAssert(pPrevious->CalculatesZF());
                pProvider = pPrevious;
            }
            else
            {
                // ZF calculated but not consumed
                WarpAssert(!pPrevious->CalculatesZF());
                IFC(AddHook(pPrevious, pProvider));
            }
        }
    }

Cleanup:
    return hr;
}


class CHookList
{
public:
    CHookList()
    {
        m_pHead = NULL;
        m_uSize = 0;
        m_uMaxSize = 0;
    }
    bool IsEmpty() const
    {
        return m_pHead == NULL;
    }
    void AddAsFirst(Hook *pNewHook)
    {
        pNewHook->m_pNext = m_pHead;
        m_pHead = pNewHook;

        if (++m_uSize > m_uMaxSize) m_uMaxSize = m_uSize;
    }
    void Insert(Hook *pNewHook)
    {
        UINT32 uOrder = pNewHook->m_pOperator->m_uOrder;
        Hook *pHook = m_pHead, **ppHook = &m_pHead;
        while(pHook && pHook->m_pOperator->m_uOrder < uOrder)
        {
            ppHook = &pHook->m_pNext;
            pHook = pHook->m_pNext;
        }

        pNewHook->m_pNext = pHook;
        *ppHook = pNewHook;

        if (++m_uSize > m_uMaxSize) m_uMaxSize = m_uSize;
    }
    void Exclude(Hook **ppHook)
    {
        Hook *pHook = *ppHook;
        WarpAssert(pHook);
        *ppHook = pHook->m_pNext;

        m_uSize--;
    }
    Hook **GetHead() { return &m_pHead; }
    UINT32 GetMaxSize() const { return m_uMaxSize; }

private:
    Hook *m_pHead;
    UINT32 m_uSize;
    UINT32 m_uMaxSize;
};

struct ShuffleCtx
{
    UINT32 uFirstOperator;
    UINT32 uNextToScheduleOperator;
    CHookList readyList;
};

__checkReturn HRESULT
CProgram::ShuffleSpan(OpSpan * pSpan)
{
    HRESULT hr = S_OK;

#if DBG
    UINT32 uSpanIdx = static_cast<UINT32>(pSpan - m_pSpanGraph);
#endif
    UINT32 uLongestChainSize = 0;

    //
    // Look at all the operators in the span except last one;
    // see whether an operator is ready to go or there are
    // other operators that should be executed before.
    // Store each operator that has no blockers into ready-to-go list.
    //

    UINT32 uFirstOperator = pSpan->m_uFirst;
    UINT32 uLastOperator = pSpan->m_uLast;

    ShuffleCtx ctx;
    ctx.uFirstOperator = uFirstOperator;
    ctx.uNextToScheduleOperator = uFirstOperator;


    for (UINT32 uOp = uLastOperator; uOp > uFirstOperator;)
    {
        COperator *pOperator = m_prgOperators[--uOp];

        if (pOperator->m_uBlockersCount == 0)
        {
            Hook *pHook = AllocHook();
            IFCOOM(pHook);
            pHook->m_pOperator = pOperator;
            ctx.readyList.AddAsFirst(pHook);
        }
    }

    COperator *pLastSheduledOperator = NULL;
    while (!ctx.readyList.IsEmpty() || pLastSheduledOperator)
    {
        COperator *pOperatorToShedule = NULL;
        if (!ctx.readyList.IsEmpty())
        {
            pOperatorToShedule = ChooseNextOperator(ctx);

            // place choosen operator

            pOperatorToShedule->m_uOrder = ctx.uNextToScheduleOperator;

            WarpAssert(ctx.uNextToScheduleOperator < uLastOperator);
            m_prgOperators[ctx.uNextToScheduleOperator++] = pOperatorToShedule;
        }

        if (pLastSheduledOperator)
        {
            COperator *pOperator = pLastSheduledOperator;
            // unblock consumers
            while (pOperator->m_pDependents)
            {
                Hook *pHook = pOperator->m_pDependents;
                COperator *pConsumer = pHook->m_pOperator;
                WarpAssert(pConsumer->m_uSpanIdx == uSpanIdx);

                pOperator->m_pDependents = pHook->m_pNext;
                RecycleHook(pHook);
                if (--pConsumer->m_uBlockersCount == NULL)
                {
                    Hook *pHook = AllocHook();
                    IFCOOM(pHook);
                    pHook->m_pOperator = pConsumer;
                    ctx.readyList.Insert(pHook);
                }

                // Calculate longest chain size along the way
                UINT32 uChainSize = pOperator->m_uChainSize + 1;
                if (pConsumer->m_uChainSize < uChainSize)
                {
                    pConsumer->m_uChainSize = uChainSize;
                }
                if (uLongestChainSize < uChainSize)
                {
                    uLongestChainSize = uChainSize;
                }
            }
        }
        pLastSheduledOperator = pOperatorToShedule;
    }

    WarpAssert(ctx.uNextToScheduleOperator == uLastOperator);

    pSpan->m_uLongestChainSize = uLongestChainSize;
    pSpan->m_uVariety = ctx.readyList.GetMaxSize();

Cleanup:
    return hr;
}

COperator*
CProgram::ChooseNextOperator(struct ShuffleCtx &ctx)
{
    Hook **ppHook = ctx.readyList.GetHead();
    Hook *pHook = *ppHook;
    COperator *pOperator = pHook->m_pOperator;
    ctx.readyList.Exclude(ppHook);
    RecycleHook(pHook);
    return pOperator;
}

__checkReturn HRESULT
CProgram::AddHook(COperator *pBlocker, COperator *pDependent)
{
    HRESULT hr = S_OK;

    Hook *pHook = AllocHook();
    IFCOOM(pHook);

    pHook->m_pOperator = pDependent;

    pHook->m_pNext = pBlocker->m_pDependents;
    pBlocker->m_pDependents = pHook;

    pDependent->m_uBlockersCount++;
Cleanup:
    return hr;
}

Hook*
CProgram::AllocHook()
{
    Hook *pHook = m_pRecycledHooks;

    if (pHook)
    {
        m_pRecycledHooks = pHook->m_pNext;
    }
    else
    {
        pHook = (Hook*)AllocMem(sizeof(Hook));
    }

    return pHook;
}

void
CProgram::RecycleHook(Hook *pHook)
{
    pHook->m_pNext = m_pRecycledHooks;
    m_pRecycledHooks = pHook;
}


