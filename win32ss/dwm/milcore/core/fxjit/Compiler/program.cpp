// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      class CProgram - runtime code generator.
//
//-----------------------------------------------------------------------------
#include "precomp.h"


//------------------------------------------------------------------------
//
//  Member:
//      CProgram::Create
//
//  Synopsis:
//      Create an object for Just-In-Time code generation.
//
//  Arguments:
//      usCallParametersSize:
//          size, in bytes, of stack bytes to be released on return
//          from generated code.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::Create(
    __in UINT16 usCallParametersSize,
    __deref_out CProgram **ppProgram
    )
{
    CFlushMemory LocalHolder;

    UINT8 *pMem = LocalHolder.Alloc(sizeof(CProgram));
    if (!pMem)
        return E_OUTOFMEMORY;

    // Create CProgram object and transfer memory ownership to it,
    // so that LocalHolder destructor will do nothing.
    CProgram *pProgram = new(pMem) CProgram(LocalHolder, usCallParametersSize);

    HRESULT hr = pProgram->Init();
    if (FAILED(hr))
    {
        pProgram->Destroy();
        return hr;
    }

    *ppProgram = pProgram;

    return S_OK;
}

//------------------------------------------------------------------------------
void CProgram::Destroy()
{
    // Grab the ownership of flush memory, so that it
    // will not be flushed in the mid of destructor.
    CFlushMemory LocalHolder(m_memory);

    this->~CProgram();
}

CProgram::CProgram(
    CFlushMemory & memory,
    UINT16 usCallParametersSize
    )
    : m_memory(memory)
    , m_usCallParametersSize(usCallParametersSize)
{
    m_prgOperators = NULL;
    m_uOperatorsAllocated = 0;
    m_uOperatorsCount = 0;

    m_prgInstructions = NULL;

    m_prgVarDesc = NULL;
    m_uVarsCount = 0;
    m_uVarsAllocated = 0;

    m_pDummyOperator = NULL;

    m_uFramePointerID = 0;

    m_pRethinkList = NULL;

#if WPFGFX_FXJIT_X86
#else // _AMD64_
    m_uArgument1ID = 0;
#endif

    m_fFlowIsSplit = false;
    m_uCurrentFlow = 0;

    m_uSpanCount = 0;
    m_pSpanGraph = NULL;
    m_prgVarSources = NULL;
    m_pRecycledLinks = NULL;
    m_pRecycledHooks = NULL;

    m_pClientData = NULL;
#if DBG_DUMP
    m_hDumpFile = NULL;
#endif //DBG_DUMP

    m_fEBP_Allowed = false;
    m_fEnableShuffling = true;
    m_fEnableMemShuffling = true;
    m_fEnableTotalBubbling = true;

    // Disable the use of negative stack offsets by default.  
    // This will likely increase generated code size, but is more compatible 
    // with debugging and profiling.

    m_fUseNegativeStackOffsets = false;

    m_fUseSSE41 = false;
    m_fAvoidMOVDs = false;

    m_fReturnPresents = false;

    m_uCodeSize = 0;
}

__checkReturn HRESULT
CProgram::Init()
{
    HRESULT hr = S_OK;

    // m_pDummyOperator works on memory overflow. It serves as replacement of
    // allocated memory in CProgram::AddOperator to allow prototype program
    // to complete its pass in idle mode.
    m_pDummyOperator = reinterpret_cast<COperator*>(AllocMem(sizeof(COperator)));
    IFCOOM(m_pDummyOperator);

    // Skip zero variable index, it is reserved to mark undefined variable
    AllocVar(vtPointer);

    // allocate ID for implicit variable that points to stack frame
    m_uFramePointerID = AllocVar(vtPointer);
    IFCOOM(m_uFramePointerID);

    // Insert dummy operator that provides the pointer to stack frame.
    // We need this as an entity to keep consumers list.
    AddOperator(otLoadFramePointer, m_uFramePointerID);

#if WPFGFX_FXJIT_X86
#else // _AMD64_
    // allocate IDs for call arguments that are passed in registers
    m_uArgument1ID = AllocVar(vtPointer);
    IFCOOM(m_uArgument1ID);
    AddOperator(otLoadFramePointer, m_uArgument1ID);
#endif

Cleanup:
    return hr;
}

void
CProgram::SetMode(UINT32 uParameterIdx, INT32 nParameterValue)
{
    switch (uParameterIdx)
    {
    case CJitterAccess::sc_uidAllowEBP:
        m_fEBP_Allowed = nParameterValue != 0;
        break;

    case CJitterAccess::sc_uidEnableShuffling:
        m_fEnableShuffling = nParameterValue != 0;
        break;

    case CJitterAccess::sc_uidEnableMemShuffling:
        m_fEnableMemShuffling = nParameterValue != 0;
        break;

    case CJitterAccess::sc_uidEnableTotalBubbling:
        m_fEnableTotalBubbling = nParameterValue != 0;
        break;

    case CJitterAccess::sc_uidUseNegativeStackOffsets:
        m_fUseNegativeStackOffsets = nParameterValue != 0;
        break;

    case CJitterAccess::sc_uidUseSSE41:
        m_fUseSSE41 = nParameterValue != 0;
        break;

    case CJitterAccess::sc_uidAvoidMOVDs:
        m_fAvoidMOVDs = nParameterValue != 0;
        break;
    }
}

UINT8*
CProgram::AllocFlushMemory(UINT32 cbSize)
{
    return m_memory.Alloc(cbSize);
}

SOperator *
CProgram::AddOperator(
    __in OpType ot,
    __in UINT32 vResult,
    __in UINT32 vOperand1,
    __in UINT32 vOperand2,
    __in UINT32 vOperand3
    )
{
    HRESULT hr = S_OK;


    // Check for uninitialized variables.
    if (!WasOverflow())
    {
        WarpAssert(vOperand1 == 0 || VarIsInitialized(vOperand1));
        WarpAssert(vOperand2 == 0 || VarIsInitialized(vOperand2));
        WarpAssert(vOperand3 == 0 || VarIsInitialized(vOperand3));
        if (vResult) VarSetInitialized(vResult);
    }

    COperator *pOperator = NULL;

    UINT8 *pMem = AllocMem(sizeof(COperator));
    IFCOOM(pMem);

    pOperator = new(pMem) COperator(ot, vResult, vOperand1, vOperand2, vOperand3);

    if (m_uOperatorsCount == m_uOperatorsAllocated)
    {
        IFC(GrowOperators());
    }

    pOperator->m_uOrder = m_uOperatorsCount;
    m_prgOperators[m_uOperatorsCount++] = pOperator;
    if (pOperator->IsControl())
    {
        m_uSpanCount++;
    }

Cleanup:
    return pOperator ? pOperator : m_pDummyOperator;
}


UINT_PTR
CProgram::SnapData(uu32x1 const& src)
{
    UINT8 *pMem = AllocMem(sizeof(u32x4));
    if (pMem == NULL)
    {
        return (UINT_PTR)&src;
    }
    else
    {
        *(uu32x1*)pMem = src;
        return (UINT_PTR)pMem;
    }
}

UINT_PTR
CProgram::SnapData(uu32x2 const& src)
{
    UINT8 *pMem = AllocMem(sizeof(u32x4));
    if (pMem == NULL)
    {
        return (UINT_PTR)&src;
    }
    else
    {
        *(uu32x2*)pMem = src;
        return (UINT_PTR)pMem;
    }
}

UINT_PTR
CProgram::SnapData(uu32x4 const& src)
{
    UINT8 *pMem = AllocMem(sizeof(u32x4));
    if (pMem == NULL)
    {
        return (UINT_PTR)&src;
    }
    else
    {
        *(uu32x4*)pMem = src;
        return (UINT_PTR)pMem;
    }
}

__checkReturn HRESULT
CProgram::GrowOperators(UINT32 uDelta)
{
    HRESULT hr = S_OK;

    UINT32 uOperatorsDesired = m_uOperatorsAllocated + uDelta;

    WarpAssert(m_uOperatorsCount < uOperatorsDesired);

    if(UINT_MAX/sizeof(COperator*) < uOperatorsDesired)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    COperator **pp = reinterpret_cast<COperator **>(AllocMem(uOperatorsDesired * sizeof(COperator*)));
    IFCOOM(pp);

    for (UINT32 u = 0; u < m_uOperatorsCount; u++)
    {
        pp[u] = m_prgOperators[u];
    }

    m_prgOperators = pp;
    m_uOperatorsAllocated = uOperatorsDesired;

Cleanup:
    return hr;
}

UINT32
CProgram::AllocVar(VariableType vt)
{
    HRESULT hr = S_OK;
    UINT32 uVarIndex = 0;

    if (m_uVarsCount == m_uVarsAllocated)
    {
        IFC(GrowVars());
    }

    VarDesc &vd = m_prgVarDesc[m_uVarsCount];
    vd.varType = vt;
    vd.varInitialized = 0;

    m_prgVarSources[m_uVarsCount] = NULL;

    uVarIndex = m_uVarsCount++;

Cleanup:
    return uVarIndex;
}

__checkReturn HRESULT
CProgram::GrowVars()
{
    HRESULT hr = S_OK;

    WarpAssert(m_uVarsCount == m_uVarsAllocated);

    UINT32 uVarsDesired = m_uVarsAllocated + 100;

    if(UINT_MAX/sizeof(VarDesc) < uVarsDesired)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    VarDesc *p = reinterpret_cast<VarDesc*>(AllocMem(uVarsDesired * sizeof(VarDesc)));
    IFCOOM(p);

    if(UINT_MAX/sizeof(COperator*) < uVarsDesired)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    COperator **q = reinterpret_cast<COperator**>(AllocMem(uVarsDesired * sizeof(COperator*)));
    IFCOOM(q);

    for (UINT32 u = 0; u < m_uVarsCount; u++)
    {
        p[u] = m_prgVarDesc[u];
        q[u] = m_prgVarSources[u];
    }

    m_prgVarDesc = p;
    m_prgVarSources = q;
    m_uVarsAllocated = uVarsDesired;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------------
// Add return operator unless it is already in.
void
CProgram::AddReturnOperator()
{
    if (!m_fReturnPresents)
    {
        SOperator *pOperator = AddOperator(otReturn, 0, m_uFramePointerID);
        pOperator->m_immediateData = m_usCallParametersSize;
        m_fReturnPresents = true;
    }
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CProgram::Compile
//
//  Synopsis:
//      Generate binary code to implement an algorithm accumulated in
//      current program with AddOperator calls.
//
//-------------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::Compile(
    __deref_out UINT8 ** ppBinaryCode
    )
{
    HRESULT hr = S_OK;

    // Add return operator at the end of the program unless it is present already.

    AddReturnOperator();

    // Check for memory overflow which could happen on proto routine.
    if (m_memory.WasOverflow())
    {
        IFC(E_OUTOFMEMORY);
    }

    IFC(BuildSpanGraph());
    IFC(BuildDependencyGraph());

    // It looks like for PixelJIT scenarios it's faster to apply RemoveUnused twice,
    // this reduce time losses on optimizations.
    RemoveUnused();

    IFC(ConvertToSSA());

    IFC(Reduce());

    RemoveUnused();

    IFC(BuildVarUsageTables());

    if (m_fEnableShuffling)
    {
        IFC(Shuffle());
    }

    IFC(BuildInstructionGraph());

    IFC(CompressConstants());

    IFC(Assemble(ppBinaryCode));

#if DBG_DUMP
    if (IsDumpEnabled()) DumpSpans();
#endif

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  CProgram::Reduce
//
//  This routine applies various transformations to the
//  description of the algorithm, trying to reduce its size and get
//  better performance.
//
//-------------------------------------------------------------------
__checkReturn HRESULT
CProgram::Reduce()
{
    HRESULT hr = S_OK;

    //
    // Straightforward routine would place all the operators into
    // m_pRethinkList, then cycle with following pattern:
    //  while (rethink list is not empty)
    //  {
    //      - fetch operator from rethink list
    //      - think about this operator
    //      if (optimization is possible)
    //      {
    //          - make optimization changes
    //          - place all related operators into rethink list
    //      }
    //  }
    //
    // The routine below makes first rough pass taking operators
    // directly from array. This pass nopifies up to 70% of operators,
    // so far avoiding unnecessary Hook allocations. 
    

    for (UINT32 i = 0; i < m_uOperatorsCount; i++)
    {
        COperator * pOperator = m_prgOperators[i];
        if (pOperator->m_uFlags)
            continue;   // in rethink list

        IFC(Think(pOperator));
    }

    while(m_pRethinkList)
    {
        Hook * pHook = m_pRethinkList;
        m_pRethinkList = pHook->m_pNext;
        COperator * pOperator = pHook->m_pOperator;
        RecycleHook(pHook);
        WarpAssert(pOperator->m_uFlags);
        pOperator->m_uFlags = 0;

        IFC(Think(pOperator));
    }

Cleanup:
    return hr;
}

__checkReturn HRESULT 
CProgram::Think(COperator * pOperator)
{
    HRESULT hr = S_OK;

    switch (pOperator->m_ot)
    {
    case otPtrAssign:
    case otUINT32Assign:
#if WPFGFX_FXJIT_X86
    case otMmAssign:
#endif
    case otXmmAssign:
    case otXmmDWordsAssign:
    case otXmmFloat1Assign:
    case otXmmFloat4Assign:
        {
            IFC(RemoveAssignUp(pOperator));
            if (pOperator->m_ot != otNone)
            {
                IFC(RemoveAssignDown(pOperator));
            }
        }
        break;
#if WPFGFX_FXJIT_X86
    case otMmLoadDWord:
#endif
    case otXmmLoadDWord:
        IFC(OptimizeLoadDWord(pOperator));
        break;

    case otPtrCompute:
        {
            IFC(OptimizePtrCompute(pOperator));
            if (pOperator->m_ot != otNone)
            {
                IFC(OptimizePointersArithmetic(pOperator));
            }
        }
        break;

    case otXmmIntNot:
    case otXmmFloat4Not:
        IFC(OptimizeAndNot(pOperator));
        break;

    default:
        IFC(OptimizeIndicesUsage(pOperator));
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
// place operator into rethink list unless it's already there
__checkReturn HRESULT 
CProgram::Rethink(COperator * pOperator)
{
    HRESULT hr = S_OK;

    if (pOperator->m_uFlags == 0)
    {
        Hook * pHook = AllocHook();
        IFCOOM(pHook);
        pHook->m_pOperator = pOperator;
        pHook->m_pNext = m_pRethinkList;
        m_pRethinkList = pHook;
        pOperator->m_uFlags = 1;
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::RemoveAssignUp
//
//  Synopsis:
//      The attempt to remove assign operator with replacing
//      result value of its provider.
//
//      Given:
//          provider: A = <something>;
//          assigner: B = A;
//          consumer1: foo(B);
//          consumer2: foo(B);
//
//      Optimized:
//          provider: B = <something>;
//          assigner: NOP;
//          consumer1: foo(B);
//          consumer2: foo(B);
//
//-------------------------------------------------------------------
__checkReturn HRESULT
CProgram::RemoveAssignUp(COperator * pAssigner)
{
    HRESULT hr = S_OK;
    if (pAssigner->m_refType != RefType_Direct)
        goto Cleanup;

    UINT32 A = pAssigner->m_vOperand1;
    UINT32 B = pAssigner->m_vResult;

    if (!IsSimpleVar(A))
        goto Cleanup;

    // "A" is not mentioned anywhere, except provider and assigner.
    // So it is safe to stop using A.

    COperator * pProvider = pAssigner->m_pProviders->m_pProvider;
    if (!VarUnusedInBetween(pProvider, pAssigner, B))
        goto Cleanup;

    // do the change
    WarpAssert(pProvider->m_vResult == A);
    RedirectOperator(pProvider, B);
    IFC(Rethink(pProvider));

    // make corresponding changes in dependency graph
    for (Link * pLink2 = pAssigner->m_pConsumers; pLink2; pLink2 = pLink2->m_pNextConsumer)
    {
        COperator *pConsumer = pLink2->m_pConsumer;
        IFC(AddLink(pConsumer, pProvider));
        IFC(Rethink(pConsumer));
    }

    NopifyOperator(pAssigner);

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::RemoveAssignDown
//
//  Synopsis:
//      The attempt to remove assign operator with replacing
//      operand values of its consumers.
//
//      Given:
//          provider: A = <something>;
//          assigner: B = A;
//          consumer1: foo(B);
//          consumer2: foo(B);
//
//      Optimized:
//          provider: A = <something>;
//          assigner: NOP;
//          consumer1: foo(A);
//          consumer2: foo(A);
//
//-------------------------------------------------------------------
__checkReturn HRESULT
CProgram::RemoveAssignDown(COperator * pAssigner)
{
    HRESULT hr = S_OK;

    if (pAssigner->m_refType != RefType_Direct)
        goto Cleanup;

    UINT32 A = pAssigner->m_vOperand1;
    UINT32 B = pAssigner->m_vResult;

    if (IsUniqueProvider(pAssigner) && IsSimpleVar(A))
    {
        // simple case: "A" is nowhere used except to pass value from provider to assigner,
        // and the assigned is the only guy that generates "B" value.
    }
    else
    {
        // check whether optimization is feasible: i.e. whether A is not changed
        // after assigner and before consumers

        for (Link * pLink = pAssigner->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
        {
            const COperator *pConsumer = pLink->m_pConsumer;
            if (!VarUnchangedInBetween(pAssigner, pConsumer, A))
            {
                goto Cleanup;
            }
        }
    }

    // do the change
    while (pAssigner->m_pConsumers)
    {
        Link * pLink = pAssigner->m_pConsumers;
        COperator *pConsumer = pLink->m_pConsumer;
        IFC(Rethink(pConsumer));

        if (pConsumer->m_vOperand1 == B)
        {
            pConsumer->m_vOperand1 = A;
        }
        if (pConsumer->m_vOperand2 == B)
        {
            pConsumer->m_vOperand2 = A;
        }
        if (pConsumer->m_vOperand3 == B)
        {
            pConsumer->m_vOperand3 = A;
        }

        RemoveLink(pLink);

        for (Link *pProviderLink = pAssigner->m_pProviders; pProviderLink; pProviderLink = pProviderLink->m_pNextProvider)
        {
            COperator *pProvider = pProviderLink->m_pProvider;
            IFC(Rethink(pProvider));
            IFC(AddLink(pConsumer, pProvider));
        }
    }

    NopifyOperator(pAssigner);

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------
//
//  Member:
//      CProgram::OptimizeLoadDWord
//
//  Synopsis:
//
//      Exclude intermediate 32-bit values when they only intended to
//      compose 64-bit or 128-bit value.
//
//      Following example shows why we need this.
//          C_u32x4 xmm;
//          P_u32 p;
//          C_u32 u;
//          xmm = p[u];
//
//      The operand of "xmm = " is 32-bit expression that's directed to
//      32-bit variable (since at the moment when the expression is handled
//      it is not yet known where it will be used). So far we obtain two
//      operators:
//
//          provider: otUINT32Load(A, ...)
//          assigner: otXmmLoadDWord(B, A) or otMmLoadDWord(B, A)
//
//      Here they are replaced with single otMmLoadDWord.
//
//          provider: otXmmLoadDWord(B, ...) or otMmLoadDWord(B, ...)
//          assigner: NOP
//
//+-----------------------------------------------------------------------
__checkReturn HRESULT
CProgram::OptimizeLoadDWord(COperator * pAssigner)
{
    HRESULT hr = S_OK;

#if WPFGFX_FXJIT_X86
    WarpAssert( pAssigner->m_ot == otMmLoadDWord || pAssigner->m_ot == otXmmLoadDWord );
#else
    WarpAssert(                                     pAssigner->m_ot == otXmmLoadDWord );
#endif
    if (pAssigner->m_refType != RefType_Direct)
        goto Cleanup;

    COperator * pProvider = pAssigner->m_pProviders->m_pProvider;
    if (pProvider->m_ot != otUINT32Load) goto Cleanup;

    UINT32 A = pAssigner->m_vOperand1;
    UINT32 B = pAssigner->m_vResult;

    if (!IsSimpleVar(A))
        goto Cleanup;

    // "A" is not mentioned anywhere, except provider and assigner.
    // So it is safe to stop using A.

    if (!VarUnusedInBetween(pProvider, pAssigner, B))
        goto Cleanup;

    // do the change
    pProvider->m_ot = pAssigner->m_ot;
    WarpAssert(pProvider->m_vResult == A);
    RedirectOperator(pProvider, B);
    IFC(Rethink(pProvider));

    // make corresponding changes in dependency graph
    for (Link * pLink2 = pAssigner->m_pConsumers; pLink2; pLink2 = pLink2->m_pNextConsumer)
    {
        COperator *pConsumer = pLink2->m_pConsumer;
        IFC(Rethink(pConsumer));
        IFC(AddLink(pConsumer, pProvider));
    }

    NopifyOperator(pAssigner);

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::FindUniqueProvider
//
//  Synopsis:
//      Look for unique provider of given operand of given operator.
//      Return NULL if provider is not unique (due to loops and conditions).
//      Otherwise return pointer to the link to provider.
//
//------------------------------------------------------------------------------
Link*
CProgram::FindUniqueProvider(COperator * pOperator, UINT32 uOperand)
{
    Link* pLinkOfInterest = NULL;
    for (Link* pLink = pOperator->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
    {
        const COperator * pProvider = pLink->m_pProvider;
        if (pProvider->m_vResult == uOperand)
        {
            if (pLinkOfInterest != NULL)
            {
                // we've already found one provider and now met another one.
                // So far it is not unique provider.
                return NULL;
            }
            pLinkOfInterest = pLink;
        }
    }

    // there should be at least one provider; otherwise we have uninitialized variable.
    WarpAssert(pLinkOfInterest);
    return pLinkOfInterest;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::IsUniqueProvider
//
//  Synopsis:
//      Detect whether given operator is the unique provider of its result value.
//
//------------------------------------------------------------------------------
bool
CProgram::IsUniqueProvider(const COperator * pOperator) const
{
    if (pOperator->m_pNextVarProvider)
    {
        return false;
    }
    else
    {
        UINT32 uVarID = pOperator->m_vResult;
        const COperator * pProvider = m_prgVarSources[uVarID];
        return pProvider == pOperator;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::IsSimpleVar
//
//  Synopsis:
//      Detect whether given variable has single provider and single consumer
//
//------------------------------------------------------------------------------
bool
CProgram::IsSimpleVar(UINT32 uVar)const
{
    const COperator * pProvider = m_prgVarSources[uVar];
    WarpAssert(pProvider); // should not be called for idle variables

    if (pProvider->m_pNextVarProvider)
    {
        return false;
    }

    const Link * pConsumerLink = pProvider->m_pConsumers;
    WarpAssert(pProvider); // should not be called for unused variables

    return pConsumerLink->m_pNextConsumer == NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::OptimizePtrCompute
//
//  Synopsis:
//      Glue together otPtrCompute with another otPtrCompute or otPtrAssignImm.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::OptimizePtrCompute(COperator * pOperator)
{
    HRESULT hr = S_OK;

    WarpAssert(pOperator->m_ot == otPtrCompute);

    // UINT32 uBase = pOperator->m_vOperand1;
    UINT32 uIndex = 0;

    if (pOperator->m_refType == RefType_Base)
    {
        WarpAssert(pOperator->m_vOperand2 == 0);
        // base + offset addressing mode
    }
    else
    {
        WarpAssert(pOperator->m_refType == RefType_Index_1 ||
                   pOperator->m_refType == RefType_Index_2 ||
                   pOperator->m_refType == RefType_Index_4 ||
                   pOperator->m_refType == RefType_Index_8);

        if (pOperator->m_vOperand2)
        {
            // base + scaled index + offset addressing mode
            uIndex = pOperator->m_vOperand2;
            WarpAssert(uIndex);
        }
        else
        {
            // scaled index + offset addressing mode
            WarpAssert(pOperator->m_vOperand2 == 0);
            goto Cleanup;
        }
    }


    Link* pLink = FindUniqueProvider(pOperator, pOperator->m_vOperand1);
    if (pLink == NULL)
        goto Cleanup;

    COperator *pProvider = pLink->m_pProvider;
    if (pProvider->m_ot == otPtrAssignImm)
    {
        //
        // The operator of type "otPtrCompute" that has constant base operand.
        //
        // in 64-bit mode this optimization is not
        // always possible because displacement can only be 32 bits while pointer has 64.
        //

        if (uIndex)
        {
            pOperator->m_vOperand1 = uIndex;
            pOperator->m_vOperand2 = 0;
        }
        else
        {
            pOperator->m_ot = otPtrAssignImm;
            pOperator->m_vOperand1 = 0;
        }

        pOperator->m_uDisplacement += pProvider->m_uDisplacement;

        RemoveLink(pLink);

        if (pProvider->m_pConsumers == NULL)
        {
            NopifyOperator(pProvider);
        }

        IFC(Rethink(pOperator));
    }
    else if (pProvider->m_ot == otPtrCompute)
    {
        //
        // The operator of type "otPtrCompute" that has constant base operand
        // generated by another "otPtrCompute".
        //

        if (pProvider->m_pConsumers->m_pNextConsumer)
        {
            // unsupported case: provider serves another consumer
            goto Cleanup;
        }

        if (uIndex)
        {
            // unsupported case for now
            goto Cleanup;
        }

        if (!VarUnusedInBetween(pProvider, pOperator, pOperator->m_vResult))
        {
            goto Cleanup;
        }

        // do the change
        pProvider->m_uDisplacement += pOperator->m_uDisplacement;
        RedirectOperator(pProvider, pOperator->m_vResult);
        IFC(Rethink(pProvider));

        // make corresponding changes in dependency graph
        for (Link * pLink2 = pOperator->m_pConsumers; pLink2; pLink2 = pLink2->m_pNextConsumer)
        {
            COperator *pConsumer = pLink2->m_pConsumer;
            IFC(AddLink(pConsumer, pProvider));
            IFC(Rethink(pConsumer));
        }

        NopifyOperator(pOperator);
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::OptimizeIndicesUsage
//
//  Synopsis:
//      Look for operators that calculate pointer values by "lea"
//      instruction (otPtrCompute).
//      Detect cases where their results are consumed
//      as naked pointers (i.e. without indices and offsets).
//      Make the optimization: let consumers accept providers' arguments
//      directly, and exclude providers.
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::OptimizeIndicesUsage(COperator * pOperator)
{
    HRESULT hr = S_OK;

    if (pOperator->m_refType != RefType_Base)
        goto Cleanup;

    bool fUseOperand2;
    if (pOperator->IsStandardUnary())
    {
        WarpAssert(pOperator->m_vOperand2 == 0);
        fUseOperand2 = false;
    }
    else if (pOperator->IsStandardBinary() || pOperator->m_ot == otUINT32Add)
    {
        fUseOperand2 = true;
    }
    else if (pOperator->IsStandardMemDst())
    {
        fUseOperand2 = true;
    }
    else goto Cleanup;

    UINT32 &uVar = fUseOperand2 ? pOperator->m_vOperand2 : pOperator->m_vOperand1;
    Link* pLink = FindUniqueProvider(pOperator, uVar);
    if (pLink == NULL)
        goto Cleanup;

    COperator * pProvider = pLink->m_pProvider;

    WarpAssert(pProvider->m_pConsumers);
    if (pProvider->m_pConsumers->m_pNextConsumer == NULL)
    {
        WarpAssert(pProvider->m_pConsumers->m_pConsumer == pOperator);
    }
    else
        goto Cleanup;

    if (pProvider->m_ot != otPtrCompute)
        goto Cleanup;

    //
    // We've found an operator that has a pointer operand calculated by otPtrCompute.
    // We can change this operator to consume the provider's base/index/offset
    // directly, unless base or index change between provider and consumer
    //

    WarpAssert(pProvider->m_vOperand1);
    if (!VarUnchangedInBetween(pProvider, pOperator, pProvider->m_vOperand1))
        goto Cleanup;

    if (pProvider->m_vOperand2)
    {
        if (!VarUnchangedInBetween(pProvider, pOperator, pProvider->m_vOperand2))
            goto Cleanup;
    }

    if (fUseOperand2)
    {
        pOperator->m_vOperand2 = pProvider->m_vOperand1;
        pOperator->m_vOperand3 = pProvider->m_vOperand2;
    }
    else
    {
        pOperator->m_vOperand1 = pProvider->m_vOperand1;
        pOperator->m_vOperand2 = pProvider->m_vOperand2;
    }

    pOperator->m_refType = pProvider->m_refType;
    pOperator->m_uDisplacement += pProvider->m_uDisplacement;

    for (Link * pLink2 = pProvider->m_pProviders; pLink2; pLink2 = pLink2->m_pNextProvider)
    {
        IFC(AddLink(pOperator, pLink2->m_pProvider));
    }

    RemoveLink(pLink);

    WarpAssert(pProvider->m_pConsumers == NULL);
    NopifyOperator(pProvider);

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Member:
//      CProgram::OptimizePointersArithmetic
//
//  Synopsis:
//      Look for expressions
//          otUINT32ImmShiftLeft(delta, src1, imm_shift)
//          otPtrCompute(dst, src, delta)
//      Exclude shifts whenever possible
//
//------------------------------------------------------------------------
__checkReturn HRESULT
CProgram::OptimizePointersArithmetic(COperator * pOperator)
{
    HRESULT hr = S_OK;

    WarpAssert(pOperator->m_ot == otPtrCompute);

    for (;;)    // redo loop to catch another shift
    {
        if (!pOperator->m_vOperand2)
            goto Cleanup;

        WarpAssert(pOperator->m_refType == RefType_Index_1 ||
               pOperator->m_refType == RefType_Index_2 ||
               pOperator->m_refType == RefType_Index_4 ||
               pOperator->m_refType == RefType_Index_8);

        Link* pLink = FindUniqueProvider(pOperator, pOperator->m_vOperand2);
        if (pLink == NULL)
            goto Cleanup;

        COperator * pProvider = pLink->m_pProvider;

        if (pProvider->m_ot != otUINT32ImmShiftLeft)
            goto Cleanup;

        WarpAssert(pProvider->m_pConsumers);
        if (pProvider->m_pConsumers->m_pNextConsumer == NULL)
        {
            WarpAssert(pProvider->m_pConsumers->m_pConsumer == pOperator);
        }
        else
            goto Cleanup;

        if (!VarUnchangedInBetween(pProvider, pOperator, pProvider->m_vOperand1))
            goto Cleanup;

        WarpAssert(pOperator->m_refType == RefType_Index_1 ||
               pOperator->m_refType == RefType_Index_2 ||
               pOperator->m_refType == RefType_Index_4 ||
               pOperator->m_refType == RefType_Index_8);

        UINT32 uShift = UINT32(pOperator->m_refType) + pProvider->m_shift;
        if (uShift > 3)
            goto Cleanup;

        pOperator->m_refType = (RefType)(uShift);
        pOperator->m_vOperand2 = pProvider->m_vOperand1;

        for (Link * pLink2 = pProvider->m_pProviders; pLink2; pLink2 = pLink2->m_pNextProvider)
        {
            IFC(AddLink(pOperator, pLink2->m_pProvider));
        }

        RemoveLink(pLink);

        WarpAssert(pProvider->m_pConsumers == NULL);
        NopifyOperator(pProvider);
    }

Cleanup:
    return hr;
}

__checkReturn HRESULT
CProgram::OptimizeAndNot(COperator * pOperator)
{
    HRESULT hr = S_OK;

    WarpAssert(pOperator->m_ot == otXmmIntNot || pOperator->m_ot == otXmmFloat4Not);

    //
    // check whether all the consumers of this operator are of type "And"
    //
    bool fAllConsumersAreAnds = true;
    for (Link * pLink = pOperator->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
    {
        const COperator * pConsumer = pLink->m_pConsumer;

        //
        // The condition for equal operands below catches weird codes
        // like a = ~b then c = a&a. We don't care.
        //
        if (pConsumer->m_ot != otXmmIntAnd &&
            pConsumer->m_ot != otXmmFloat4And ||
            pConsumer->m_vOperand1 == pConsumer->m_vOperand2
            )
        {
            fAllConsumersAreAnds = false;
            break;
        }

    }

    //
    // if there is any other consumer then we can't do anything good
    //
    if (!fAllConsumersAreAnds)
        goto Cleanup;

    //
    // Change provider from otXmmIntNot to otXmmAssign and all consumers from
    // otXmmIntAnd to otXmmIntAndNot
    //

    pOperator->m_ot = otXmmAssign;

    IFC(Rethink(pOperator));

    for (Link * pLink = pOperator->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
    {
        COperator * pConsumer = pLink->m_pConsumer;

        if (pConsumer->m_vOperand2 == pOperator->m_vResult)
        {
            WarpAssert(pConsumer->m_refType == RefType_Direct);
            pConsumer->m_vOperand2 = pConsumer->m_vOperand1;
            pConsumer->m_vOperand1 = pOperator->m_vResult;
        }

        WarpAssert(pConsumer->m_vOperand1 == pOperator->m_vResult);

        if (pConsumer->m_ot == otXmmIntAnd)
        {
            pConsumer->m_ot = otXmmIntAndNot;
        }
        else
        {
            WarpAssert (pConsumer->m_ot == otXmmFloat4And);
            pConsumer->m_ot = otXmmFloat4AndNot;
        }
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::VarUnchangedInBetween
//
//  Synopsis:
//      Check whether the variable uVar can be changed in given span of operators.
//      Returns "true" if it's known for sure that the car is unchanged.
//      Returns "false" if the routine can't conclude it unchanged.
//
//------------------------------------------------------------------------------
bool
CProgram::VarUnchangedInBetween(const COperator * pFrom, const COperator * pTo, UINT32 uVar)
{
    UINT32 uFrom = pFrom->m_uOrder;
    UINT32 uTo = pTo->m_uOrder;

    UINT32 uSpanIdx = pFrom->m_uSpanIdx;
    if (pTo->m_uSpanIdx != uSpanIdx)
    {
        return false;
    }

    if (uFrom > uTo)
        return false;

    // Ensure that no operator changes "uVar" in between of "pFrom" and "pTo"
    for (COperator * pProvider = m_prgVarSources[uVar]; pProvider; pProvider = pProvider->m_pNextVarProvider)
    {
        UINT32 uProviderOrder = pProvider->m_uOrder;
        if (uProviderOrder > uFrom && uProviderOrder < uTo)
        {
            return false;
        }
    }

    return true;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CProgram::VarUnusedInBetween
//
//  Synopsis:
//      Check whether the variable uVar can be used in given span of operators.
//      Returns "true" if it's known for sure that the car is unused.
//      Returns "false" if the routine can't conclude it unused.
//
//------------------------------------------------------------------------------
bool
CProgram::VarUnusedInBetween(const COperator * pFrom, const COperator * pTo, UINT32 uVar)
{
    UINT32 uFrom = pFrom->m_uOrder;
    UINT32 uTo = pTo->m_uOrder;

    UINT32 uSpanIdx = pFrom->m_uSpanIdx;
    if (pTo->m_uSpanIdx != uSpanIdx)
    {
        return false;
    }

    if (uFrom > uTo)
        return false;

    // Ensure that no operator consumes "uVar" in between of provider and assigner
    for (COperator * pProvider = m_prgVarSources[uVar]; pProvider; pProvider = pProvider->m_pNextVarProvider)
    {
        for (Link * pLink = pProvider->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
        {
            const COperator * pConsumer = pLink->m_pConsumer;
            UINT32 uConsumerOrder = pConsumer->m_uOrder;
            if (uConsumerOrder > uFrom && uConsumerOrder < uTo)
            {
                return false;
            }
        }
    }

    return true;
}



__checkReturn HRESULT
CProgram::CompressConstants()
{
    HRESULT hr = S_OK;

    if (!m_storage4.Idle())
    {
        UINT8* pData = AllocMem(m_storage4.EstimatedSize());
        IFCOOM(pData);
        m_storage4.SetStorage(pData);
    }
    if (!m_storage8.Idle())
    {
        UINT8* pData = AllocMem(m_storage8.EstimatedSize());
        IFCOOM(pData);
        m_storage8.SetStorage(pData);
    }
    if (!m_storage16.Idle())
    {
        UINT8* pData = AllocMem(m_storage16.EstimatedSize());
        IFCOOM(pData);
        m_storage16.SetStorage(pData);
    }

    for (UINT32 i = 0; i < m_uOperatorsCount; i++)
    {
        COperator * pOperator = m_prgOperators[i];
        if (pOperator->m_refType != RefType_Static)
            continue;

        switch (pOperator->GetDataType())
        {
        case ofDataR32:
        case ofDataM32:
        case ofDataI32:
        case ofDataF32:
            m_storage4.Store(pOperator->m_uDisplacement);
            break;

        case ofDataM64:
        case ofDataI64:
            m_storage8.Store(pOperator->m_uDisplacement);
            break;

        case ofDataI128:
        case ofDataF128:
            m_storage16.Store(pOperator->m_uDisplacement);
            break;

        }
    }

Cleanup:
    return hr;
}


#if DBG
UINT32 g_uTotalCodeSize = 0;
#endif

__checkReturn HRESULT
CProgram::Assemble(
    __deref_out UINT8 ** ppBinaryCode
    )
{
    HRESULT hr = S_OK;

    UINT8 *pCode;

    CMapper mapper(this);
    IFC(mapper.MapProgram());

    if (m_fEnableTotalBubbling)
    {
        CBubbler bubbler(this);
        bubbler.BubbleMoves();
    }

    {
        CAssemblePass1 coder1(mapper, m_fUseNegativeStackOffsets);
        coder1.AssemblePrologue(mapper.GetFrameSize(), mapper.GetFrameAlignment());

#if DBG_DUMP
        coder1.AssembleProgram(this, false);
#else
        coder1.AssembleProgram(this);
#endif //DBG_DUMP

        m_uCodeSize = coder1.GetCount();
    }

#if DBG
    g_uTotalCodeSize += m_uCodeSize;
#endif
    UINT32 uSizeToAlloc =
        m_storage16.AllocateSpace(
            m_storage8.AllocateSpace(
                m_storage4.AllocateSpace(
                    m_uCodeSize
                )
            )
        );

    IFC(CJitterSupport::CodeAllocate(uSizeToAlloc, &pCode));

    m_storage4.CopyData(pCode);
    m_storage8.CopyData(pCode);
    m_storage16.CopyData(pCode);

#if DBG_DUMP
    if (IsDumpEnabled()) DumpConstants();
#endif
    {
        CAssemblePass2 coder2(
            mapper,
            m_fUseNegativeStackOffsets,
            pCode,
            m_storage4.GetAddressDelta(),
            m_storage8.GetAddressDelta(),
            m_storage16.GetAddressDelta()
            );
        coder2.AssemblePrologue(mapper.GetFrameSize(), mapper.GetFrameAlignment());

#if DBG_DUMP
        coder2.AssembleProgram(this, IsDumpEnabled());
#else
        coder2.AssembleProgram(this);
#endif //DBG_DUMP
    }

    *ppBinaryCode = pCode;

Cleanup:
    return hr;
}



