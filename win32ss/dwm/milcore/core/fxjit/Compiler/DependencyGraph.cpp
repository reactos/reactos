// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Operator dependency graph implementation.
//
//-----------------------------------------------------------------------------
#include "precomp.h"


//------------------------------------------------------------------------
//
//  Member:
//      CProgram::BuildSpanGraph
//
//  Synopsis:
//      Split the program given in m_prgOperators[m_uOperatorsCount] array
//      to linear pieces, each represented by struct OpSpan.
//      Provide linking info that specifies for each span which span[s] can
//      follow this one.
//      Return the head span (first to execute) in CProgram::m_pSpanGraph.
//
//      Along the way: compose CProgram::m_prgVarSources array.
//      Each entry of this array serves as a head of list
//      enumerating all the operators that generate a value for this
//      variable. List starts in m_prgVarSources[varID] and continues
//      via COperator::m_pNextVarProvider.
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::BuildSpanGraph()
{
    HRESULT hr = S_OK;

    // Not supposed to be called twice
    WarpAssert(m_pSpanGraph == NULL);

    m_pSpanGraph = (OpSpan*)AllocMem(sizeof(OpSpan) * m_uSpanCount);
    IFCOOM(m_pSpanGraph);

    OpSpan *pSpan = NULL;
    OpSpan *pPreviousSpan = NULL;
    UINT32 uSpanIdx = 0;
    for (UINT32 uOp = 0; uOp < m_uOperatorsCount; uOp++)
    {
        if (!pSpan)
        {
            // initialize current OpSpan
            WarpAssert(uSpanIdx < m_uSpanCount);
            pSpan = m_pSpanGraph + uSpanIdx;
            pSpan->m_uFirst = uOp;
            pSpan->m_pConsumers = NULL;
            pSpan->m_pProviders = NULL;
            pSpan->m_fInTodoList = false;
            pSpan->m_fInDoneList = false;
            pSpan->m_pInputs = NULL;
            pSpan->m_pOutputs = NULL;

            // Connect with previous if it exists.
            // pPreviousSpan is NULL for the very first span in the program,
            // and also after the span containing return from the program or subroutine.
            if (pPreviousSpan)
            {
                IFC(AddSpanLink(pSpan, pPreviousSpan));
                pPreviousSpan = NULL;
            }
        }

        COperator * pOperator = m_prgOperators[uOp];
        pOperator->m_uSpanIdx = uSpanIdx;

        if (pOperator->m_vResult != 0)
        {
            // this operator generates value for variable m_vResult
            COperator* &pListHead = m_prgVarSources[pOperator->m_vResult];
            pOperator->m_pNextVarProvider = pListHead;
            pListHead = pOperator;
        }

        if (pOperator->IsControl())
        {
            pSpan->m_uLast = uOp;
            if (pOperator->m_ot != otReturn && pOperator->m_ot != otSubroutineCall && pOperator->m_ot != otSubroutineReturn)
            {
                pPreviousSpan = pSpan;
            }
            uSpanIdx++;
            pSpan = NULL;
        }
    }

    WarpAssert((uSpanIdx) == m_uSpanCount);

    for (UINT32 uSpan = 0; uSpan < m_uSpanCount; uSpan++)
    {
        OpSpan * pSpan = m_pSpanGraph + uSpan;

        UINT32 i = pSpan->m_uLast;
        COperator * pLastOperator = m_prgOperators[i];
        WarpAssert(pLastOperator->IsControl());

        if (pLastOperator->IsBranchSplit())
        {
            // setup alternative way from pSpan to the one that follows pLinkedSpan:
            //
            //  +------------------+        +------------------+        +------------------+
            //  |                  >--...--->                  >-------->                  |
            //  |     *pSpan       |        |   *pLinkedSpan   |        |   *pNextSpan     |
            //  |                  >---+    |                  |   +---->                  |
            //  +------------------+   |    +------------------+   |    +------------------+
            //                         |                           |
            //                         +---------------------------+
            //

            const COperator * pLinkedOperator = (COperator*)pLastOperator->m_pLinkedOperator;
            UINT32 uLinkedSpan = pLinkedOperator->m_uSpanIdx;
            WarpAssert(uLinkedSpan < m_uSpanCount && uLinkedSpan > uSpan);

            UINT32 uNextSpan = uLinkedSpan + 1;
            WarpAssert(uNextSpan < m_uSpanCount);
            OpSpan * pNextSpan = m_pSpanGraph + uNextSpan;

            IFC(AddSpanLink(pNextSpan, pSpan));
        }
        else if (pLastOperator->IsLoopStart())
        {
            // setup a merge point at body start
            //
            //  +------------------+        +------------------+       +------------------+
            //  |                  >-------->                  >--...-->                  >---...
            //  |     *pSpan       |        |   *pNextSpan     |       |   *pLinkedSpan   |
            //  |                  |   +---->                  |       |                  >---+
            //  +------------------+   |    +------------------+       +------------------+   |
            //                         |                                                      |
            //                         +------------------------------------------------------+
            //

            OpSpan * pNextSpan = pSpan + 1;

            const COperator * pLinkedOperator = (COperator*)pLastOperator->m_pLinkedOperator;
            UINT32 uLinkedSpan = pLinkedOperator->m_uSpanIdx;
            WarpAssert(uLinkedSpan < m_uSpanCount && uLinkedSpan > uSpan);
            OpSpan * pLinkedSpan = m_pSpanGraph + uLinkedSpan;

            IFC(AddSpanLink(pNextSpan, pLinkedSpan));
        }
        else if (pLastOperator->m_ot == otSubroutineCall)
        {
            // setup a merge point at body start
            //
            //                       +---------------------------------------------+
            //                       |                                             |
            //                       | +-----------------------------------------+ |
            //                       | |                                         | |
            //                       | | +-------------------------------------+ | |
            //                       | | |                                     | | |
            //  +------------------+ | | |          +------------------+       | | | +------------------+     +------------------+
            //  |                  | | | |          |                  |       | | +->                  |     |                  >-------+
            //  |     *pSpan       >-+ | |     +---->   *pNextSpan     >--...  | +--->   *pStartSpan    >-...->   *pReturnSpan   >-----+ |
            //  |                  |   | |     |    |                  |       +----->                  |     |                  >---+ | |
            //  +------------------+   | |     |    +------------------+             +------------------+     +------------------+   | | |
            //                         | |     |                                                                                     | | |
            //                         | |     +-------------------------------------------------------------------------------------+ | |
            //                         | |                                                                                             | |
            //  +------------------+   | |          +------------------+                                                               | |
            //  |                  |   | |          |                  |                                                               | |
            //  |     *pSpan       >---+ |     +---->   *pNextSpan     >--...                                                          | |
            //  |                  |     |     |    |                  |                                                               | |
            //  +------------------+     |     |    +------------------+                                                               | |
            //                           |     |                                                                                       | |
            //                           |     +---------------------------------------------------------------------------------------+ |
            //                           |                                                                                               |
            //  +------------------+     |          +------------------+                                                                 |
            //  |                  |     |          |                  |                                                                 |
            //  |     *pSpan       >-----+     +---->   *pNextSpan     >--...                                                            |
            //  |                  |           |    |                  |                                                                 |
            //  +------------------+           |    +------------------+                                                                 |
            //                                 |                                                                                         |
            //                                 +-----------------------------------------------------------------------------------------+
            //

            UINT32 uNextSpan = uSpan + 1;
            OpSpan * pNextSpan = m_pSpanGraph + uNextSpan;

            COperator * pStartOperator = (COperator*)pLastOperator->m_pLinkedOperator;
            WarpAssert(pStartOperator->m_ot == otSubroutineStart);

            UINT32 uStartSpan = pStartOperator->m_uSpanIdx;
            OpSpan * pStartSpan = m_pSpanGraph + uStartSpan;

            const COperator * pReturnOperator = (COperator*)pStartOperator->m_pLinkedOperator;
            WarpAssert(pReturnOperator->m_ot == otSubroutineReturn);

            UINT32 uReturnSpan = pReturnOperator->m_uSpanIdx;
            OpSpan * pReturnSpan = m_pSpanGraph + uReturnSpan;

            IFC(AddSpanLink(pStartSpan, pSpan));
            IFC(AddSpanLink(pNextSpan, pReturnSpan));
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::BuildDependencyGraph
//
//  Synopsis:
//      For each operator, compose the list of providers - i.e. operators
//      that supply data for this operator.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::BuildDependencyGraph()
{
    HRESULT hr = S_OK;

    for (UINT32 uOp = 0; uOp < m_uOperatorsCount; uOp++)
    {
        COperator * pOperator = m_prgOperators[uOp];
        UINT32 uOperand1 = pOperator->m_vOperand1;
        if (uOperand1)
        {
            IFC(GatherProviders(pOperator, uOperand1));
            UINT32 uOperand2 = pOperator->m_vOperand2;
            if (uOperand2)
            {
                if (uOperand2 != uOperand1)
                {
                    IFC(GatherProviders(pOperator, uOperand2));
                }

                UINT32 uOperand3 = pOperator->m_vOperand3;
                if (uOperand3)
                {
                    if (uOperand3 != uOperand2 && uOperand3 != uOperand1)
                    {
                        IFC(GatherProviders(pOperator, uOperand3));
                    }
                }
            }
            else
            {
                WarpAssert(pOperator->m_vOperand3 == 0);
            }
        }
        else
        {
            WarpAssert(pOperator->m_vOperand2 == 0 && pOperator->m_vOperand3 == 0);
        }
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSpanList
//
//  Synopsis:
//      Helper for GatherProviders().
//      Stores the list of spans that should be investigated
//      and a list of ones that have been already investigated.
//      Provides a way to traverse span graph that's not a DAG
//      (contains cycles).
//
//------------------------------------------------------------------------------
class CSpanList
{
public:
    CSpanList()
    {
        m_pTodo = NULL;
        m_pDone = NULL;
    }

    ~CSpanList()
    {
        while (m_pTodo)
        {
            WarpAssert(m_pTodo->m_fInTodoList);
            m_pTodo->m_fInTodoList = false;
            m_pTodo = m_pTodo->m_pNextWork;
        }
        while (m_pDone)
        {
            WarpAssert(m_pDone->m_fInDoneList);
            m_pDone->m_fInDoneList = false;
            m_pDone = m_pDone->m_pNextWork;
        }
    }

    void AddTodo(OpSpan * pSpan)
    {
        if (!pSpan->m_fInTodoList && !pSpan->m_fInDoneList)
        {
            pSpan->m_pNextWork = m_pTodo;
            m_pTodo = pSpan;
            pSpan->m_fInTodoList = true;
        }
    }

    bool IsEmpty() const
    {
        return m_pTodo == NULL;
    }

    OpSpan * GetWork()
    {
        OpSpan * pSpan = m_pTodo;
        WarpAssert(pSpan);
        WarpAssert(pSpan->m_fInTodoList);
        WarpAssert(!pSpan->m_fInDoneList);
        m_pTodo = pSpan->m_pNextWork;
        pSpan->m_fInTodoList = false;
        pSpan->m_pNextWork = m_pDone;
        m_pDone = pSpan;
        pSpan->m_fInDoneList = true;
        return pSpan;
    }


protected:
    OpSpan * m_pTodo;
    OpSpan * m_pDone;
};

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::GatherProviders
//
//  Synopsis:
//      For given operator and given variable (that's one of m_vOperand1/2/3),
//      compose the list of providers - i.e. operators that supply data for this operator.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::GatherProviders(COperator *pOperator, UINT32 varID)
{
    HRESULT hr = S_OK;

    //
    // First, look for unique provider in the same span.
    // This code relies upon descending order in variable providers list.
    //
    COperator *pProvider = m_prgVarSources[varID];
    while (pProvider != NULL && pProvider->m_uOrder >= pOperator->m_uOrder)
    {
        pProvider = pProvider->m_pNextVarProvider;
    }

    if (pProvider == NULL)
    {
        WarpError("If there are no providers ahead then at least one code path uses uninitialized data.");
        IFC(E_FAIL);
    }

    if (pOperator->m_uSpanIdx == pProvider->m_uSpanIdx)
    {
        // Found unique provider in the same span
        IFC(AddLink(pOperator, pProvider));
    }
    else
    {
        // The value of varID comes from another span,
        // possibly from several spans.

        if (pOperator->m_uSpanIdx == 0)
        {
            WarpError("Uninitialized data in the very first span.");
            IFC(E_FAIL);
        }

        CSpanList list;

        OpSpan * pThisSpan = m_pSpanGraph + pOperator->m_uSpanIdx;

        for (SpanLink * pLink = pThisSpan->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
        {
            list.AddTodo(pLink->m_pProvider);
        }

        IFC(GatherExternalProviders(list, pOperator, varID));
    }

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Member:
//      CProgram::GatherExternalProviders
//
//  Synopsis:
//      Helper for GatherProviders().
//      Handles the case when provider[s] of given operator are outside
//      of the span where this operator is.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::GatherExternalProviders(CSpanList & list, COperator *pOperator, UINT32 varID)
{
    HRESULT hr = S_OK;

    while(!list.IsEmpty())
    {
        OpSpan * pSpan = list.GetWork();


        //
        // Look for provider in the this span.
        // This code relies upon descending order in variable providers list.
        //
        COperator * pProvider = m_prgVarSources[varID];
        while (pProvider)
        {
            const OpSpan * pProviderSpan = m_pSpanGraph + pProvider->m_uSpanIdx;
            if (pProviderSpan == pSpan)
            {
                break;
            }
            if (pProviderSpan < pSpan)
            {
                pProvider = NULL;
                break;
            }
            pProvider = pProvider->m_pNextVarProvider;
        }

        if (pProvider)
        {
            // we've found the provider operator in this span
            IFC(AddLink(pOperator, pProvider));
        }
        else
        {
            // this span does not change variable varID, so
            // this variable should be initialized in previous span[s]

            if (pSpan == m_pSpanGraph)
            {
                WarpError("Uninitialized data in the very first span.");
                IFC(E_FAIL);
            }

            for (SpanLink * pLink = pSpan->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
            {
                list.AddTodo(pLink->m_pProvider);
            }
        }
    }

Cleanup:
    return hr;
}

#if DBG
void CProgram::AssertValidLink(Link *pLink)
{
    WarpAssert(pLink);

    COperator *pProvider = pLink->m_pProvider;
    Link *pLinkC = pProvider->m_pConsumers;
    while(pLinkC)
    {
        if (pLinkC == pLink)
            break;
        pLinkC = pLinkC->m_pNextConsumer;
    }
    WarpAssert(pLinkC);

    COperator *pConsumer = pLink->m_pConsumer;
    Link *pLinkP = pConsumer->m_pProviders;
    while(pLinkP)
    {
        if (pLinkP == pLink)
            break;
        pLinkP = pLinkP->m_pNextProvider;
    }
    WarpAssert(pLinkP);
}
#else //!DBG
void CProgram::AssertValidLink(Link *) {}
#endif

Link*
CProgram::AllocLink()
{
    Link *pLink = m_pRecycledLinks;

    if (pLink)
    {
        m_pRecycledLinks = pLink->m_pNextProvider;
    }
    else
    {
        pLink = (Link*)AllocMem(sizeof(Link));
    }

    return pLink;
}

void
CProgram::RecycleLink(Link *pLink)
{
    pLink->m_pNextProvider = m_pRecycledLinks;
    m_pRecycledLinks = pLink;
}

__checkReturn HRESULT
CProgram::AddLink(COperator *pConsumer, COperator *pProvider)
{
    HRESULT hr = S_OK;

    Link *pLink = AllocLink();
    IFCOOM(pLink);

    pLink->m_pProvider = pProvider;
    pLink->m_pConsumer = pConsumer;

    pLink->m_pNextProvider = pConsumer->m_pProviders;
    pConsumer->m_pProviders = pLink;

    pLink->m_pNextConsumer = pProvider->m_pConsumers;
    pProvider->m_pConsumers = pLink;

    AssertValidLink(pLink);

Cleanup:
    return hr;
}

void
CProgram::RemoveLink(Link *pLink)
{
    AssertValidLink(pLink);

    COperator *pProvider = pLink->m_pProvider;
    Link **ppLinkC = &pProvider->m_pConsumers;
    while(*ppLinkC)
    {
        if (*ppLinkC == pLink)
            break;
        ppLinkC = &(*ppLinkC)->m_pNextConsumer;
    }
    *ppLinkC = pLink->m_pNextConsumer;

    COperator *pConsumer = pLink->m_pConsumer;
    Link **ppLinkP = &pConsumer->m_pProviders;
    while(*ppLinkP)
    {
        if (*ppLinkP == pLink)
            break;
        ppLinkP = &(*ppLinkP)->m_pNextProvider;
    }
    *ppLinkP = pLink->m_pNextProvider;

    RecycleLink(pLink);
}

SpanLink*
CProgram::AllocSpanLink()
{
    C_ASSERT(sizeof(Link) == sizeof(SpanLink));
    return ((SpanLink*)AllocLink());
}

__checkReturn HRESULT
CProgram::AddSpanLink(OpSpan *pConsumer, OpSpan *pProvider)
{
    HRESULT hr = S_OK;

    SpanLink *pLink = AllocSpanLink();
    IFCOOM(pLink);

    pLink->m_pProvider = pProvider;
    pLink->m_pConsumer = pConsumer;

    pLink->m_pNextProvider = pConsumer->m_pProviders;
    pConsumer->m_pProviders = pLink;

    pLink->m_pNextConsumer = pProvider->m_pConsumers;
    pProvider->m_pConsumers = pLink;

    AssertValidSpanLink(pLink);

Cleanup:
    return hr;
}

#if DBG
void CProgram::AssertValidSpanLink(SpanLink *pLink)
{
    WarpAssert(pLink);

    OpSpan *pProvider = pLink->m_pProvider;
    SpanLink *pLinkC = pProvider->m_pConsumers;
    while(pLinkC)
    {
        if (pLinkC == pLink)
            break;
        pLinkC = pLinkC->m_pNextConsumer;
    }
    WarpAssert(pLinkC);

    OpSpan *pConsumer = pLink->m_pConsumer;
    SpanLink *pLinkP = pConsumer->m_pProviders;
    while(pLinkP)
    {
        if (pLinkP == pLink)
            break;
        pLinkP = pLinkP->m_pNextProvider;
    }
    WarpAssert(pLinkP);
}
#else //!DBG
void CProgram::AssertValidSpanLink(SpanLink *) {}
#endif

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::ConvertToSSA
//
//  Synopsis:
//      Convert each span to Single Static Assignment representation
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::ConvertToSSA()
{
    HRESULT hr = S_OK;

    //
    // This code relies upon descending order in variable providers list.
    //

    for (UINT32 uVarID = m_uVarsCount; --uVarID > 0;)
    {
        // pProvider points to the last source of uVarId in a span
        COperator * pProvider = m_prgVarSources[uVarID];
        if (pProvider == NULL)
            continue; // dead var - no providers at all

        WarpAssert(pProvider->m_vResult == uVarID);

        UINT32 uSpanIDX = pProvider->m_uSpanIdx;

        for (;;)
        {
            COperator * pNextProvider = pProvider->m_pNextVarProvider;
            if (pNextProvider == NULL)
                break;

            WarpAssert(pNextProvider->m_vResult == uVarID);

            UINT32 uNextSpanIDX = pNextProvider->m_uSpanIdx;
            if (uNextSpanIDX != uSpanIDX)
            {
                // entering next span
                pProvider = pNextProvider;
                uSpanIDX = uNextSpanIDX;
                continue;
            }
            //
            // We've met second provider of the same variable in the same span,
            // which is against SSA rules.
            // Introduce new variable as the result of pNextProvider that precedes
            // pProvider; switch pNextProvider and all its consumer to this variable
            //

            UINT32 uVarIDVersion = AllocVar(GetVarType(uVarID));
            IFCOOM(uVarIDVersion);

            // exclude pNextProvider from var source list
            pProvider->m_pNextVarProvider = pNextProvider->m_pNextVarProvider;

            // include it to new var src list
            WarpAssert(m_prgVarSources[uVarIDVersion] == NULL);
            m_prgVarSources[uVarIDVersion] = pNextProvider;
            pNextProvider->m_pNextVarProvider = NULL;

            // Change pNextProvider result value
            pNextProvider->m_vResult = uVarIDVersion;

            // adjust all the consumers

            for (Link * pLink = pNextProvider->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
            {
                COperator * pConsumer = pLink->m_pConsumer;
                WarpAssert(pConsumer->m_uSpanIdx == uSpanIDX);
#if DBG
                for (Link * pLink2 = pConsumer->m_pProviders; pLink2; pLink2 = pLink2->m_pNextProvider)
                {
                    const COperator * pDbgProvider = pLink2->m_pProvider;
                    WarpAssert(pDbgProvider == pNextProvider || pDbgProvider->m_vResult != uVarID);
                }
#endif

                if (pConsumer->m_vOperand1 == uVarID) pConsumer->m_vOperand1 = uVarIDVersion;
                if (pConsumer->m_vOperand2 == uVarID) pConsumer->m_vOperand2 = uVarIDVersion;
                if (pConsumer->m_vOperand3 == uVarID) pConsumer->m_vOperand3 = uVarIDVersion;
            }
        }
    }

Cleanup:
    return hr;
}



//------------------------------------------------------------------------
//
//  Member:
//      CProgram::BuildVarUsageTables
//
//  Synopsis:
//      Build life time data: for every span, fill bit arrays:
//          m_pVarsInUseBefore
//          m_pVarsInUseAfter
//          m_pVarsChanged
//          m_pVarsUsed
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::BuildVarUsageTables()
{
    HRESULT hr = S_OK;

    m_uBitArraySize = CBitArray::GetSizeInDWords(m_uVarsCount);

    for (UINT32 uSpan = 0; uSpan < m_uSpanCount; uSpan++)
    {
        OpSpan * pSpan = m_pSpanGraph + uSpan;

        pSpan->m_pVarsInUseBefore = (CBitArray*)AllocMem(sizeof(UINT32) * m_uBitArraySize);
        IFCOOM(pSpan->m_pVarsInUseBefore);
        pSpan->m_pVarsInUseBefore->Clear(m_uBitArraySize);

        pSpan->m_pVarsInUseAfter = (CBitArray*)AllocMem(sizeof(UINT32) * m_uBitArraySize);
        IFCOOM(pSpan->m_pVarsInUseAfter);
        pSpan->m_pVarsInUseAfter->Clear(m_uBitArraySize);

        pSpan->m_pVarsChanged = (CBitArray*)AllocMem(sizeof(UINT32) * m_uBitArraySize);
        IFCOOM(pSpan->m_pVarsChanged);
        pSpan->m_pVarsChanged->Clear(m_uBitArraySize);

        pSpan->m_pVarsUsed = (CBitArray*)AllocMem(sizeof(UINT32) * m_uBitArraySize);
        IFCOOM(pSpan->m_pVarsUsed);
        pSpan->m_pVarsUsed->Clear(m_uBitArraySize);
    }

    for (UINT32 uOp = 0; uOp < m_uOperatorsCount; uOp++)
    {
        COperator * pOperator = m_prgOperators[uOp];

        OpSpan * pSpan = m_pSpanGraph + pOperator->m_uSpanIdx;

        UINT32 uResult = pOperator->m_vResult;
        if (uResult)
        {
            pSpan->m_pVarsChanged->Set(uResult);
        }


        UINT32 uOperand1 = pOperator->m_vOperand1;
        if (uOperand1)
        {
            pSpan->m_pVarsUsed->Set(uOperand1);

            InspectProviders(pOperator, uOperand1);
            UINT32 uOperand2 = pOperator->m_vOperand2;
            if (uOperand2)
            {
                if (uOperand2 != uOperand1)
                {
                    pSpan->m_pVarsUsed->Set(uOperand2);

                    InspectProviders(pOperator, uOperand2);
                }

                UINT32 uOperand3 = pOperator->m_vOperand3;
                if (uOperand3)
                {
                    if (uOperand3 != uOperand2 && uOperand3 != uOperand1)
                    {
                        pSpan->m_pVarsUsed->Set(uOperand3);

                        InspectProviders(pOperator, uOperand3);
                    }
                }
            }
            else
            {
                WarpAssert(pOperator->m_vOperand3 == 0);
            }
        }
        else
        {
            WarpAssert(pOperator->m_vOperand2 == 0 && pOperator->m_vOperand3 == 0);
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::InspectProviders
//
//  Synopsis:
//      For given operator and given variable (that's one of m_vOperand1/2/3),
//      inspect the list of providers and fill usage tables:
//          m_pVarsInUseBefore, m_pVarsInUseAfter
//
//------------------------------------------------------------------------
void
CProgram::InspectProviders(COperator *pOperator, UINT32 varID)
{
    //
    // First, look for unique provider in the same span.
    //
    for (Link *pLink = pOperator->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
    {
        const COperator *pProvider = pLink->m_pProvider;
        WarpAssert(pProvider);

        if (pProvider->m_vResult != varID)
            continue; // this provider is for another operand

        if (pProvider->m_uSpanIdx == pOperator->m_uSpanIdx &&
            pProvider->m_uOrder < pOperator->m_uOrder)
        {
            // this provider is in-span internal so no VarsInUse marks are required
            return;
        }
    }

    CSpanList list;

    WarpAssert(pOperator->m_uSpanIdx > 0);

    OpSpan * pThisSpan = m_pSpanGraph + pOperator->m_uSpanIdx;

    pThisSpan->m_pVarsInUseBefore->Set(varID);

    for (SpanLink * pLink = pThisSpan->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
    {
        OpSpan * pPrevSpan = pLink->m_pProvider;
        pPrevSpan->m_pVarsInUseAfter->Set(varID);
        list.AddTodo(pPrevSpan);
    }

    InspectExternalProviders(list, pOperator, varID);
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::InspectExternalProviders
//
//  Synopsis:
//      Helper for GatherProviders().
//      Handles the case when provider[s] of given operator are outside
//      of the span where this operator is.
//
//------------------------------------------------------------------------
void
CProgram::InspectExternalProviders(CSpanList & list, COperator *pOperator, UINT32 varID)
{
    while(!list.IsEmpty())
    {
        OpSpan * pSpan = list.GetWork();

        //
        // Look for provider in the this span.
        //
        const COperator *pProvider = NULL;
        for (Link *pLink = pOperator->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
        {
            const COperator *pPossibleProvider = pLink->m_pProvider;
            WarpAssert(pPossibleProvider);

            if (pPossibleProvider->m_vResult != varID)
                continue; // this provider is for another operand

            const OpSpan *pProviderSpan = m_pSpanGraph + pPossibleProvider->m_uSpanIdx;

            if (pProviderSpan == pSpan)
            {
                // this span does generate requested value
                pProvider = pPossibleProvider;
                break;
            }
        }

        if (!pProvider)
        {
            // this span (pSpan) does not generate requested value;
            // so the latter should be provided by previous span[s]

            WarpAssert(pSpan->m_pProviders);
            pSpan->m_pVarsInUseBefore->Set(varID);

            for (SpanLink * pLink = pSpan->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
            {
                OpSpan * pPrevSpan = pLink->m_pProvider;
                pPrevSpan->m_pVarsInUseAfter->Set(varID);
                list.AddTodo(pPrevSpan);
            }
        }
    }
}

void
CProgram::NopifyOperator(COperator *pOperator)
{
    pOperator->m_ot = otNone;

    if (pOperator->m_vResult != 0)
    {
        // exclude this operator from variable providers list
        COperator** ppRef = &m_prgVarSources[pOperator->m_vResult];
        while (*ppRef != pOperator)
        {
            WarpAssert(*ppRef != NULL);
            ppRef = &((*ppRef)->m_pNextVarProvider);
        }
        *ppRef = pOperator->m_pNextVarProvider;
        pOperator->m_vResult = 0;
    }

    pOperator->m_vOperand1 = 0;
    pOperator->m_vOperand2 = 0;
    pOperator->m_vOperand3 = 0;

    while (pOperator->m_pProviders)
    {
        RemoveLink(pOperator->m_pProviders);
    }

    while (pOperator->m_pConsumers)
    {
        RemoveLink(pOperator->m_pConsumers);
    }
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::RedirectOperator
//
//  Synopsis:
//      Helper for optimization routines.
//      Changes operators result var, makes corresponding changes
//      in variable sources lists.
//      This routine does not make changes in dependency graph.
//
//------------------------------------------------------------------------
void
CProgram::RedirectOperator(COperator *pOperator, UINT32 uNewVResult)
{
    UINT32 uOldVResult = pOperator->m_vResult;
    if (uOldVResult != uNewVResult)
    {
        WarpAssert (pOperator->m_vResult != 0 && uNewVResult != 0);
        {
            // exclude this operator from variable providers list
            COperator **ppRef = &m_prgVarSources[pOperator->m_vResult];
            while (*ppRef != pOperator)
            {
                WarpAssert(*ppRef != NULL);
                ppRef = &((*ppRef)->m_pNextVarProvider);
            }
            *ppRef = pOperator->m_pNextVarProvider;
        }

        pOperator->m_vResult = uNewVResult;

        {
            // include this operator to new var providers list
            COperator **ppRef = &m_prgVarSources[pOperator->m_vResult];
            while (*ppRef != NULL && (*ppRef)->m_uOrder > pOperator->m_uOrder)
            {
                WarpAssert(*ppRef != pOperator);
                ppRef = &((*ppRef)->m_pNextVarProvider);
            }
            pOperator->m_pNextVarProvider = *ppRef;
            *ppRef = pOperator;
        }

        if (uOldVResult == m_uFramePointerID) m_uFramePointerID = uNewVResult;
#if defined(_AMD64_)
        if (uOldVResult == m_uArgument1ID) m_uArgument1ID = uNewVResult;
#endif
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::RemoveUnused
//
//  Synopsis:
//      For each operator, check whether it is really in use, explicitly or implicitly.
//      If not then mark operator as NOP.
//      An operator is considered "in use" explicitly when it has an outside effect
//      (i.e. writes the result to memory).
//      An operator is considered "in use" implicitly when it's result is needed for
//      the one that's explicitly in use, directly or indirectly.
//
//------------------------------------------------------------------------------
void
CProgram::RemoveUnused()
{
    for (UINT32 i = 0; i < m_uOperatorsCount; i++)
    {
        COperator * pOperator = m_prgOperators[i];
        if (pOperator->m_uFlags)
            continue; // we've already considered this operator useful


        if (pOperator->HasOutsideEffect()
            || pOperator->IsControl()
            || pOperator->CalculatesZF())
        {
            SetInUse(pOperator);
        }
    }


    UINT32 uNewCount = 0;
    for (UINT32 i = 0; i < m_uOperatorsCount; i++)
    {
        COperator * pOperator = m_prgOperators[i];
        if (pOperator->m_uFlags == 0)
        {
            NopifyOperator(pOperator);
        }
        else
        {
            // cleanup flags for future use
            pOperator->m_uFlags = 0;

            // correct span graph
            if (pOperator->IsControl())
            {
                UINT32 uSpan = pOperator->m_uSpanIdx;
                OpSpan *pSpan = m_pSpanGraph + uSpan;
                WarpAssert(pSpan->m_uLast == i);
                pSpan->m_uLast = uNewCount;
                if (uSpan < (m_uSpanCount-1))
                {
                    pSpan[1].m_uFirst = uNewCount + 1;
                }
            }

            // place the operator to new position
            pOperator->m_uOrder = uNewCount;
            m_prgOperators[uNewCount++] = pOperator;

            if (pOperator->m_refType == RefType_Static)
            {
                // count statics along the way
                switch (pOperator->GetDataType())
                {
                case ofDataR32:
                case ofDataM32:
                case ofDataI32:
                case ofDataF32:
                    m_storage4.Count();
                    break;

                case ofDataM64:
                case ofDataI64:
                    m_storage8.Count();
                    break;

                case ofDataI128:
                case ofDataF128:
                    m_storage16.Count();
                    break;

                }
            }
        }
    }

    m_uOperatorsCount = uNewCount;
}

void
CProgram::SetInUse(COperator *pOperator)
{
    pOperator->m_uFlags = 1;
    for (Link *pLink = pOperator->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
    {
        COperator * pProvider = pLink->m_pProvider;
        WarpAssert(pProvider);
        if (pProvider->m_uFlags)
            continue; // we've already considered this operator useful
            SetInUse(pProvider);
    }
}


//============================ Instruction Graph ================================

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::BuildInstructionGraph
//
//  Synopsis:
//      Build instruction-based representation of given algorithm.
//      Instruction graph node is class CInstruction.
//      We create an instance of CInstruction for every operator.
//      An instruction has explicit references to other instruction
//      that produce required data. This differs from operator-based
//      representation that involves variable indices.
//
//      It might happen that some instruction has alternative sources
//      for the same operand. This is handled by introducing special
//      instructions that serves as inputs and ouputs of every span.
//      There is no alternative sources inside a span due to ConvertToSSA().
//      Alternatives only can appear for input connectors of a span that
//      serves as a merge point (has several elements in m_pProviders chain).
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::BuildInstructionGraph()
{
    HRESULT hr = S_OK;

    WarpAssert(m_prgInstructions == NULL);

    m_prgInstructions = (CInstruction**)AllocMem(sizeof(CInstruction*) * m_uOperatorsCount);
    IFCOOM(m_prgInstructions);
    for (UINT32 u = 0; u < m_uOperatorsCount; u++)
    {
        UINT8 * p = AllocMem(sizeof(CInstruction));
        IFCOOM(p);
        CInstruction * pInstruction = new (p) CInstruction(m_prgOperators[u]);
        m_prgInstructions[u] = pInstruction;
    }

    for (UINT32 uSpan = 0; uSpan < m_uSpanCount; uSpan++)
    {
        OpSpan * pSpan = m_pSpanGraph + uSpan;
        IFC(BuildSpanInstructionGraph(pSpan));
    }

    IFC(ConnectSpans());

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Member:
//      CProgram::BuildSpanInstructionGraph
//
//  Synopsis:
//      Build inner subset of instruction graph:
//      - set up inner links for the span
//      - make a list of inputs for this span
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::BuildSpanInstructionGraph(OpSpan * pSpan)
{
    HRESULT hr = S_OK;

    for (UINT32 u = pSpan->m_uFirst; u <= pSpan->m_uLast; u++)
    {
        CInstruction * pIn = m_prgInstructions[u];
        COperator * pOp = pIn->m_pOperator;
        WarpAssert(pOp == m_prgOperators[u]);

        UINT32 uSpanIdx = pOp->m_uSpanIdx;

        for (Link * pLink = pOp->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
        {
            const COperator * pOpProvider = pLink->m_pProvider;
            UINT32 uVar = pOpProvider->m_vResult;
            WarpAssert(uVar);
            WarpAssert(uVar == pOp->m_vOperand1 || uVar == pOp->m_vOperand2 || uVar == pOp->m_vOperand3);

            UINT32 uProviderSpanIdx = pOpProvider->m_uSpanIdx;

            CInstruction * pInProvider;
            if (uProviderSpanIdx == uSpanIdx && pOpProvider->m_uOrder < u)
            {
                pInProvider = m_prgInstructions[pOpProvider->m_uOrder];
            }
            else
            {
                CConnector * pInput = EnsureConnector(&pSpan->m_pInputs, uVar);
                IFCOOM(pInput);
                pInProvider = &pInput->m_instruction;
            }

            IFC(AddInstructionHook(pIn, pInProvider));
        }
    }

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Member:
//      CProgram::EnsureConnector
//
//  Synopsis:
//      Find or create OpSpan input for given variable.
//
//------------------------------------------------------------------------
CConnector *
CProgram::EnsureConnector(CConnector **pList, UINT32 uVar)
{
    for (CConnector * pEntry = *pList; pEntry; pEntry = pEntry->m_pNext)
    {
        if (pEntry->m_uVarID == uVar)
            return pEntry;
    }

    // entry does not exist, create it
    UINT8 * p = AllocMem(sizeof(CConnector));
    if (!p) return NULL;

    CConnector * pNewEntry = new(p) CConnector(uVar);
    pNewEntry->m_pNext = *pList;
    *pList = pNewEntry;
    return pNewEntry;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::AddInstructionHook
//
//  Synopsis:
//      Check whether two CInstruction instances are linked;
//      if not then connect them with InstructionHook.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::AddInstructionHook(CInstruction * pIn, CInstruction * pInProvider)
{
    HRESULT hr = S_OK;

    for (InstructionHook * pHook = pIn->m_pProviders; pHook; pHook = pHook->m_pNext)
    {
        if (pHook->m_pProvider == pInProvider)
            return S_OK;
    }

    InstructionHook * pNewHook = AllocInstructionHook();
    IFCOOM(pNewHook);
    pNewHook->m_pProvider = pInProvider;
    pNewHook->m_pNext = pIn->m_pProviders;
    pIn->m_pProviders = pNewHook;
    pInProvider->m_uConsumersCount++;

Cleanup:
    return hr;
}

InstructionHook *
CProgram::AllocInstructionHook()
{
    C_ASSERT(sizeof(InstructionHook) == sizeof(Hook));
    return ((InstructionHook*)AllocHook());
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::ConnectSpans
//
//  Synopsis:
//      Build outer subset of instruction graph:
//      find providers for each input of each span.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::ConnectSpans()
{
    HRESULT hr = S_OK;

    const OpSpan * pLastSpan = m_pSpanGraph + m_uSpanCount;

    for (OpSpan * pSpan = m_pSpanGraph; pSpan < pLastSpan;)
    {
        OpSpan * pNextSpan = pSpan + 1;
        for (SpanLink * pLink = pSpan->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
        {
            bool fInputsUpdated = false;

            OpSpan * pProvider = pLink->m_pProvider;
            IFC(ConnectTwoSpans(pProvider, pSpan, &fInputsUpdated));
            if (fInputsUpdated)
            {
                if (pProvider < pNextSpan)
                {
                    // roll back to redo connecting
                    pNextSpan = pProvider;
                }
            }
        }

        pSpan = pNextSpan;
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::ConnectTwoSpans
//
//  Synopsis:
//      Helper for ConnectSpans:
//      For each input of consumer span: find or create
//      corresponding outputs of providing span.
//      Ensure that these outputs in turn are linked to
//      their providers.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::ConnectTwoSpans(OpSpan * pProvider, OpSpan * pConsumer, bool * pfInputsUpdated)
{
    HRESULT hr = S_OK;

    for (CConnector * pInput = pConsumer->m_pInputs; pInput; pInput = pInput->m_pNext)
    {
        UINT32 uVar = pInput->m_uVarID;
        CConnector * pOutput = EnsureConnector(&pProvider->m_pOutputs, uVar);
        IFCOOM(pOutput);

        CInstruction * pInOutput = &pOutput->m_instruction;
        CInstruction * pInInput = &pInput->m_instruction;
        IFC(AddInstructionHook(pInInput, pInOutput));

        if (pInOutput->m_uConsumersCount > 1)
        {
            // just created output instruction would have "1";
            // greater means pOutput connector is handled already
            continue;
        }

        // look for provider instruction in the span
        COperator *pOp;
        for (pOp = m_prgVarSources[uVar]; pOp; pOp = pOp->m_pNextVarProvider)
        {
            const OpSpan *pOpSpan = m_pSpanGraph + pOp->m_uSpanIdx;
            if (pOpSpan == pProvider)
            {
                // yes, this provider is inside desired span
                CInstruction * pIn = m_prgInstructions[pOp->m_uOrder];
                IFC(AddInstructionHook(pInOutput, pIn));
                break;
            }
        }

        if (pOp)
            continue; // we've found inner provider, all done

        // There is no provider for output connector in providing span.
        // This means that the value should come thru its input
        CConnector *pInput2 = EnsureConnector(&pProvider->m_pInputs, uVar);
        IFCOOM(pInput2);
        CInstruction * pInInput2 = &pInput2->m_instruction;
        IFC(AddInstructionHook(pInOutput, pInInput2));

        // see whether the input connector already has providers
        if (pInInput2->m_pProviders == NULL)
        {
            *pfInputsUpdated = true;
        }
    }

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CSpanListD
//
//  Synopsis:
//      Helper for GetDistanceToConsumer().
//      Stores the list of spans that should be investigated
//      and a list of ones that have been already investigated.
//      Provides a way to traverse span graph that's not a DAG
//      (contains cycles).
//
//      CSpanListD behaves like CSpanList but allows re-scheduling
//      job depending on distances stored in spans.
//------------------------------------------------------------------------------
class CSpanListD : public CSpanList
{
public:
    void AddTodo(OpSpan * pSpan, UINT32 uDistance)
    {
        if (pSpan->m_fInDoneList)
        {
            // this span has been handled already ...
            if (pSpan->m_uDistance <= uDistance)
            {
                return;
            }
            // ... but it was handled with too big distance
            // and needs rescheduling

            // exclude pSpan from done list
            OpSpan ** pp = &m_pDone;
            while (*pp != pSpan)
            {
                pp = &(*pp)->m_pNextWork;
                WarpAssert(*pp);
            }
            *pp = pSpan->m_pNextWork;
            pSpan->m_fInDoneList = false;

            // include pSpan in m_pTodo list
            pSpan->m_pNextWork = m_pTodo;
            m_pTodo = pSpan;
            pSpan->m_fInTodoList = true;

            pSpan->m_uDistance = uDistance;
        }
        else if (pSpan->m_fInTodoList)
        {
            // already in m_pTodo list, just correct the distance
            if (pSpan->m_uDistance > uDistance)
            {
                pSpan->m_uDistance = uDistance;
            }
        }
        else
        {
            // include pSpan in m_pTodo list
            pSpan->m_pNextWork = m_pTodo;
            m_pTodo = pSpan;
            pSpan->m_fInTodoList = true;

            pSpan->m_uDistance = uDistance;
        }
    }
};

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::GetDistanceToConsumer
//
//  Synopsis:
//      Calculate minimal distance from given operator to
//      a consumer of given variable.
//------------------------------------------------------------------------
UINT32
CProgram::GetDistanceToConsumer(const COperator *pOperator, UINT32 uVarID)
{
    UINT32 uSmallestDistance = UINT32(-1);

    UINT32 uOrder = pOperator->m_uOrder;

    // look for consumers in given operator's span
    UINT32 uSpanIdx = pOperator->m_uSpanIdx;

    for (COperator *pProvider = m_prgVarSources[uVarID];
         pProvider;
         pProvider = pProvider->m_pNextVarProvider)
    {
        for (Link *pLink = pProvider->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
        {
            const COperator *pConsumer = pLink->m_pConsumer;

            UINT32 uConsumerSpanIdx = pConsumer->m_uSpanIdx;
            if (uConsumerSpanIdx == uSpanIdx)
            {
                UINT32 uConsumerOrder = pConsumer->m_uOrder;

                if (uConsumerOrder > uOrder)
                {
                    UINT32 uDistance = uConsumerOrder - uOrder;
                    if (uDistance < uSmallestDistance)
                    {
                        uSmallestDistance = uDistance;
                    }
                }
            }
        }
    }

    if (uSmallestDistance == UINT32(-1))
    {
        // look for consumers in other spans
        // (maybe including operator's span if loops will lead there)
        OpSpan *pSpan = m_pSpanGraph + uSpanIdx;
        UINT32 uDistanceToSpanEnd = pSpan->m_uLast + 1 - uOrder;
        CSpanListD list;

        for (SpanLink * pLink = pSpan->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
        {
            OpSpan * pNext = pLink->m_pConsumer;
            list.AddTodo(pNext, uDistanceToSpanEnd);
        }

        while (!list.IsEmpty())
        {
            OpSpan *pWorkSpan = list.GetWork();
            UINT32 uSmallestDistanceFromSpanBeg = UINT32(-1);

            for (COperator *pProvider = m_prgVarSources[uVarID];
                 pProvider;
                 pProvider = pProvider->m_pNextVarProvider)
            {
                for (Link *pLink = pProvider->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
                {
                    const COperator *pConsumer = pLink->m_pConsumer;

                    UINT32 uConsumerSpanIdx = pConsumer->m_uSpanIdx;
                    const OpSpan *pConsumerSpan = m_pSpanGraph + uConsumerSpanIdx;
                    if (pConsumerSpan == pWorkSpan)
                    {
                        UINT32 uConsumerOrder = pConsumer->m_uOrder;

                        WarpAssert(uConsumerOrder >= pWorkSpan->m_uFirst && uConsumerOrder <= pWorkSpan->m_uLast);
                        UINT32 uDistanceFromSpanBeg = uConsumerOrder - pWorkSpan->m_uFirst;
                        if (uDistanceFromSpanBeg < uSmallestDistanceFromSpanBeg)
                        {
                            uSmallestDistanceFromSpanBeg = uDistanceFromSpanBeg;
                        }
                    }
                }
            }

            if (uSmallestDistanceFromSpanBeg != UINT32(-1))
            {
                // found consumers in pWorkSpan
                UINT32 uDistanceSum = pWorkSpan->m_uDistance + uSmallestDistanceFromSpanBeg;
                if (uDistanceSum < uSmallestDistance)
                {
                    uSmallestDistance = uDistanceSum;
                }
            }
            else
            {
                // pWorkSpan does not consume the var, next spans should be inspected
                UINT32 uAccumulatedDistance = pWorkSpan->m_uDistance + (pWorkSpan->m_uLast - pWorkSpan->m_uFirst + 1);
                if (uAccumulatedDistance < uSmallestDistance)
                {
                    for (SpanLink * pLink = pWorkSpan->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
                    {
                        OpSpan * pNext = pLink->m_pConsumer;
                        list.AddTodo(pNext, uAccumulatedDistance);
                    }
                }
            }
        }
    }

    return uSmallestDistance;
}

