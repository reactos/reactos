// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of class CBubbler.
//
//-----------------------------------------------------------------------------
#include "precomp.h"

CBubbler::CBubbler(CProgram * pProgram)
    : m_pProgram(pProgram)
{
    m_ppOperators = m_pProgram->GetOperators();
    m_uOperatorsCount = m_pProgram->GetOperatorsCount();
}

void
CBubbler::BubbleMoves()
{
    for (UINT32 uOpIdx = 1; uOpIdx < m_uOperatorsCount; uOpIdx++)
    {
        COperator * pOp = m_ppOperators[uOpIdx];

        // detach the whole list of shuffle record
        CShuffleRecord * pList = pOp->m_pShuffles;
        pOp->m_pShuffles = NULL;

        while (pList)
        {
            CShuffleRecord * psr = pList;
            pList = psr->m_pNext;
            psr->m_pNext = NULL;

            // Hook to the end of the operator list
            CShuffleRecord ** pp;
            for (pp = &pOp->m_pShuffles; *pp; pp = &(*pp)->m_pNext) {}
            *pp = psr;

            BubbleRecord(psr, pOp);
        }
    }
}

#if DBG
UINT32 g_uDbgCount = 0;
UINT32 g_uDbgLimit = UINT32(-1);
#endif

void
CBubbler::BubbleRecord(CShuffleRecord * psr, COperator * pOp)
{
    for(;;)
    {
        CShuffleRecord ** ppPrev = FindPrecedingRecord(psr, pOp);
        if (ppPrev)
        {
            // There are at least one shuffle record preceding given one.
            // Try to move given toward list head.
            CShuffleRecord * pPrev = *ppPrev;
            WarpAssert(pPrev->m_pNext == psr);
            if (!CanExchange(pPrev, psr))
                return;

#if DBG
            if (g_uDbgCount++ > g_uDbgLimit)
                return;
#endif

            AssertValid(pOp);

            // Do bubbling
            pPrev->m_pNext = psr->m_pNext;
            psr->m_pNext = pPrev;
            *ppPrev = psr;

            AssertValid(pOp);
        }
        else
        {
            // Given record is at the very begin on shuffle record list.
            // Try to move it to the end of the list of previous operator.
            WarpAssert(psr == pOp->m_pShuffles);

            if (pOp->m_uOrder == 0)
                return;

            COperator * pOpPrev = m_ppOperators[pOp->m_uOrder - 1];
            if (!CanExchangeWithInstruction(pOpPrev, psr))
                return;

#if DBG
            if (g_uDbgCount++ > g_uDbgLimit)
                return;
#endif

            // Do bubbling
            pOp->m_pShuffles = psr->m_pNext;
            psr->m_pNext = NULL;

            AssertValid(pOp);

            pOp = pOpPrev;

            // Hook to the end of the operator list
            CShuffleRecord ** pp;
            for (pp = &pOp->m_pShuffles; *pp; pp = &(*pp)->m_pNext) {}
            *pp = psr;

            AssertValid(pOp);
        }
    }
}

CShuffleRecord **
CBubbler::FindPrecedingRecord(CShuffleRecord *psr, COperator * pOp)
{
    if (psr == pOp->m_pShuffles)
        return NULL;

    for (CShuffleRecord **pp = &pOp->m_pShuffles; *pp; pp = &(*pp)->m_pNext)
    {
        if ((*pp)->m_pNext == psr)
            return pp;
    }
    WarpError("CShuffleRecord *psr should be in this operator");
    return NULL;
}

#if DBG
void
CBubbler::AssertValid(COperator* pOp)
{
    UINT32 n = 0;
    for (CShuffleRecord **pp = &pOp->m_pShuffles; *pp; pp = &(*pp)->m_pNext)
    {
        n++;
        WarpAssert(n < 100);
    }
}
#endif

bool
CBubbler::CanExchange(CShuffleRecord *pPrev, CShuffleRecord *pNext)
{
    CRegID regSrcPrev = pPrev->GetRegSrc();
    CRegID regDstPrev = pPrev->GetRegDst();

    CRegID regSrcNext = pNext->GetRegSrc();
    CRegID regDstNext = pNext->GetRegDst();

    if (regSrcPrev.IsDefined())
    {
        if (regDstPrev.IsDefined())
        {
            // pPrev is reg to reg
                                        if (regSrcNext.IsDefined())
                                        {
                                            if (regDstNext.IsDefined())
                                            {
                                                // pNext is reg to reg
                                                return CanExchange(regSrcPrev, regDstPrev, regSrcNext, regDstNext);
                                            }
                                            else
                                            {
                                                // pNext is reg to mem
                                                return CanExchange(regSrcPrev, regDstPrev, regSrcNext, pNext->GetVarID());
                                            }
                                        }
                                        else
                                        {
                                                WarpAssert(regDstNext.IsDefined());
                                                // pNext is mem to reg
                                                return CanExchange(regSrcPrev, regDstPrev, pNext->GetVarID(), regDstNext);
                                        }
        }
        else
        {
            // pPrev is reg to mem
                                        if (regSrcNext.IsDefined())
                                        {
                                            if (regDstNext.IsDefined())
                                            {
                                                // pNext is reg to reg
                                                return CanExchange(regSrcPrev, pPrev->GetVarID(), regSrcNext, regDstNext);
                                            }
                                            else
                                            {
                                                // pNext is reg to mem
                                                return CanExchange(regSrcPrev, pPrev->GetVarID(), regSrcNext, pNext->GetVarID());
                                            }
                                        }
                                        else
                                        {
                                                WarpAssert(regDstNext.IsDefined());
                                                // pNext is mem to reg
                                                return CanExchange(regSrcPrev, pPrev->GetVarID(), pNext->GetVarID(), regDstNext);
                                        }
        }
    }
    else
    {
            WarpAssert(regDstPrev.IsDefined());
            // pPrev is mem to reg
                                        if (regSrcNext.IsDefined())
                                        {
                                            if (regDstNext.IsDefined())
                                            {
                                                // pNext is reg to reg
                                                return CanExchange(pPrev->GetVarID(), regDstPrev, regSrcNext, regDstNext);
                                            }
                                            else
                                            {
                                                // pNext is reg to mem
                                                return CanExchange(pPrev->GetVarID(), regDstPrev, regSrcNext, pNext->GetVarID());
                                            }
                                        }
                                        else
                                        {
                                                WarpAssert(regDstNext.IsDefined());
                                                // pNext is mem to reg
                                                return CanExchange(pPrev->GetVarID(), regDstPrev, pNext->GetVarID(), regDstNext);
                                        }
    }
}

bool
CBubbler::CanExchange(CRegID prevSrc, CRegID prevDst, CRegID nextSrc, CRegID nextDst)
{
    if (prevDst == nextSrc) return false;
    if (nextDst == prevSrc) return false;
    return true;
}

bool
CBubbler::CanExchange(CRegID prevDst, CRegID nextSrc)
{
    if (prevDst == nextSrc) return false;
    return true;
}

bool
CBubbler::CanExchange(CRegID prevSrc, CRegID prevDst, UINT32 nextSrc, CRegID nextDst)
{
    if (prevDst == nextSrc) return false;
    if (nextDst == prevSrc) return false;
    return true;
}

bool
CBubbler::CanExchange()
{
    return true;
}

bool
CBubbler::CanExchange(CRegID prevSrc, UINT32 prevDst, UINT32 nextSrc, CRegID nextDst)
{
    if (prevDst == nextSrc) return false;
    if (nextDst == prevSrc) return false;
    return true;
}


bool
CBubbler::CanExchange(UINT32 prevSrc, CRegID prevDst, CRegID nextSrc, CRegID nextDst)
{
    if (prevDst == nextSrc) return false;
    if (nextDst == prevSrc) return false;
    return true;
}

bool
CBubbler::CanExchange(UINT32 prevSrc, CRegID prevDst, CRegID nextSrc, UINT32 nextDst)
{
    if (prevDst == nextSrc) return false;
    if (nextDst == prevSrc) return false;
    return true;
}

bool
CBubbler::CanExchange(UINT32 prevSrc, CRegID prevDst, UINT32 nextSrc, CRegID nextDst)
{
    if (prevDst == nextSrc) return false;
    if (nextDst == prevSrc) return false;
    return true;
}


bool
CBubbler::CanExchangeWithInstruction(COperator * pOperator, CShuffleRecord * psr)
{
    if (pOperator->NoBubble())
        return false;

    CRegID regSrcNext = psr->GetRegSrc();
    CRegID regDstNext = psr->GetRegDst();

    if (regSrcNext.IsDefined())
    {
        if (regDstNext.IsDefined())
        {
            // pNext is reg to reg
            return CanExchangeWithInstruction(pOperator, regSrcNext, regDstNext);
        }
        else
        {
            // pNext is reg to mem
            return CanExchangeWithInstruction(pOperator, regSrcNext, psr->GetVarID());
        }
    }
    else
    {
            WarpAssert(regDstNext.IsDefined());
            // pNext is mem to reg
            return CanExchangeWithInstruction(pOperator, psr->GetVarID(), regDstNext);
    }
}

bool
CBubbler::CanExchangeWithInstruction(COperator * pOperator, CRegID nextSrc, CRegID nextDst)
{
    if (pOperator->m_rResult == nextSrc) return false;
    if (nextDst == pOperator->m_rResult) return false;
    if (nextDst == pOperator->m_rOperand1) return false;
    if (nextDst == pOperator->m_rOperand2) return false;
    if (nextDst == pOperator->m_rOperand3) return false;
    return true;
}

bool
CBubbler::CanExchangeWithInstruction(COperator * pOperator, CRegID nextSrc, UINT32 nextDst)
{
    if (pOperator->m_rResult == nextSrc) return false;
    if (nextDst == pOperator->m_vOperand1) return false;
    if (nextDst == pOperator->m_vOperand2) return false;
    if (nextDst == pOperator->m_vOperand3) return false;
    return true;
}

bool
CBubbler::CanExchangeWithInstruction(COperator * pOperator, UINT32 nextSrc, CRegID nextDst)
{
    if (pOperator->m_vResult == nextSrc) return false;
    if (nextDst == pOperator->m_rResult) return false;
    if (nextDst == pOperator->m_rOperand1) return false;
    if (nextDst == pOperator->m_rOperand2) return false;
    if (nextDst == pOperator->m_rOperand3) return false;
    return true;
}

