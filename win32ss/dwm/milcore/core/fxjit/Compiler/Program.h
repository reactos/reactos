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
#pragma once

class COperator;
struct OpSpan;
struct SpanLink;
class CSpanList;
class CBitArray;

#define MAX_FLOWS 5

struct VarDesc
{
    UINT8
        varType : 3,
        varInitialized : 1,
        reserved : 4;
};

struct uu32x1
{
    UINT32 data[1];
    bool operator==(uu32x1 const& that) const
    {
        return data[0] == that.data[0];
    }
};

struct uu32x2
{
    UINT32 data[2];
    bool operator==(uu32x2 const& that) const
    {
        return data[0] == that.data[0]
            && data[1] == that.data[1];
    }
};

struct uu32x4
{
    UINT32 data[4];
    bool operator==(uu32x4 const& that) const
    {
        return data[0] == that.data[0]
            && data[1] == that.data[1]
            && data[2] == that.data[2]
            && data[3] == that.data[3];
    }
};

template<typename El>
class StaticStorage
{
public:
    StaticStorage()
    {
        m_uCount = 0;
        m_uStoreCount = 0;
        m_uOffset = 0;
        m_uAddressDelta = 0;
        m_pStorage = NULL;
        m_pFinalLocation = NULL;
    }
    void Count()
    {
        m_uCount++;
    }
    bool Idle() const
    {
        return m_uCount == 0;
    }
    UINT32 EstimatedSize() const
    {
        return m_uCount*sizeof(El);
    }
    void SetStorage(UINT8* pStorage)
    {
        m_pStorage = (El*)pStorage;
    }
    void Store(UINT_PTR & ptr)
    {
        El const & data = *(El*)ptr;
        UINT32 i = 0;
        for (; i < m_uStoreCount; i++)
        {
            El const &stored = m_pStorage[i];
            if (data == stored)
                break;
        }

        if (i == m_uStoreCount)
        {
            WarpAssert(m_uStoreCount < m_uCount);
            m_pStorage[m_uStoreCount++] = data;
        }

       ptr = (UINT_PTR)(m_pStorage + i);
    }
    // Reserve a piece of memory in a binary code snippet
    // to store constants; return increased size.
    UINT32 AllocateSpace(UINT32 uSize)
    {
        if (!Idle())
        {
            int mask = sizeof(El) - 1;
            m_uOffset = (uSize + mask) & ~mask;
            uSize = m_uOffset + sizeof(El)*m_uStoreCount;
        }
        return uSize;
    }
    void CopyData(UINT8 *pBase)
    {
        if (!Idle())
        {
            El* pDst = (El*)(pBase + m_uOffset);
            for (UINT32 i = 0; i < m_uStoreCount; i++)
                pDst[i] = m_pStorage[i];
            m_uAddressDelta = (UINT8*)pDst - (UINT8*)m_pStorage;
            m_pFinalLocation = pDst;
        }
    }
    UINT_PTR GetAddressDelta() const
    {
        return m_uAddressDelta;
    }
    UINT32 GetFinalCount() const
    {
        return m_uStoreCount;
    }
    UINT32* GetFinalLocation() const
    {
        return (UINT32*)m_pFinalLocation;
    }

private:
    UINT32 m_uCount;
    UINT32 m_uStoreCount;
    UINT32 m_uOffset;
    INT_PTR m_uAddressDelta;
    El* m_pStorage;
    El* m_pFinalLocation;
};

//+------------------------------------------------------------------------------
//
//  Class:
//      CProgram
//
//  Synopsis:
//      Serves as runtime code generation context that accumulates
//      formal description of generated code while executing
//      prototype programs. Contains algorithms to convert
//      it to binary code.
//
//  Usage pattern:
//
//      1. Create and initialize an instance of CProgram:
//          CProgram program;
//          IFC(program.Init());
//
//      2. Execute prototype code:
//          MyProto(MyParams);
//
//      3. Build binary code:
//          UINT8 *pBinaryCode;
//          IFC(program.Compile(&pBinaryCode));
//
//-------------------------------------------------------------------------------
class CProgram : public CFlushObject
{
public:

    static __checkReturn HRESULT Create(
        __in UINT16 usCallParametersSize,
        __deref_out CProgram **ppProgram
        );

    void Destroy();

private:
    CProgram(CFlushMemory & memory, UINT16 usCallParametersSize);

    __checkReturn HRESULT Init();
    // no destructor needed

public:
    void SetMode(UINT32 uParameterIdx, INT32 nParameterValue);

    UINT8* AllocFlushMemory(UINT32 cbSize);

    UINT32 AllocVar(VariableType vt);

    void SplitFlow();
    void SetFlow(__in_range(0, (MAX_FLOWS-1)) UINT32 uFlowID);
    void ReverseFlow(__in_range(0, (MAX_FLOWS-1)) UINT32 uFlowID);
    void MergeFlow();

    SOperator * AddOperator(
        __in OpType ot = otNone,
        __in UINT32 vResult = 0,
        __in UINT32 vOperand1 = 0,
        __in UINT32 vOperand2 = 0,
        __in UINT32 vOperand3 = 0
        );

    void AddReturnOperator();

    UINT_PTR SnapData(u64x2 const& src) { return SnapData((uu32x4&)src); }
    UINT_PTR SnapData(u32x4 const& src) { return SnapData((uu32x4&)src); }
    UINT_PTR SnapData(s32x4 const& src) { return SnapData((uu32x4&)src); }
    UINT_PTR SnapData(u16x8 const& src) { return SnapData((uu32x4&)src); }
    UINT_PTR SnapData(u8x16 const& src) { return SnapData((uu32x4&)src); }
    UINT_PTR SnapData(s16x8 const& src) { return SnapData((uu32x4&)src); }
    UINT_PTR SnapData(f32x4 const& src) { return SnapData((uu32x4&)src); }
#if WPFGFX_FXJIT_X86
    UINT_PTR SnapData(u64x1 const& src) { return SnapData((uu32x2&)src); }
    UINT_PTR SnapData(u32x2 const& src) { return SnapData((uu32x2&)src); }
    UINT_PTR SnapData(u16x4 const& src) { return SnapData((uu32x2&)src); }
    UINT_PTR SnapData(u8x8  const& src) { return SnapData((uu32x2&)src); }
#endif
    UINT_PTR SnapData(float const& src) { return SnapData((uu32x1&)src); }
    UINT_PTR SnapData(UINT32 const& src) { return SnapData((uu32x1&)src); }

    __checkReturn HRESULT Compile(__deref_out UINT8 ** pBinaryCode );

    UINT32 GetCodeSize() const { return m_uCodeSize; }

    SOperator * GetOperator(__in UINT32 uIndex)
    {
        WarpAssert(uIndex < m_uOperatorsCount);
        return m_prgOperators[uIndex];
    }

    //
    // Methods to serve assemble procedure
    //
    COperator * * GetOperators()
    {
        return m_prgOperators;
    }

    UINT32 GetOperatorsCount() const { return m_uOperatorsCount; }

    CInstruction * * GetInstructions()
    {
        return m_prgInstructions;
    }

    VariableType GetVarType(UINT32 uVarIndex) const
    {
        WarpAssert(uVarIndex < m_uVarsCount);
        return (VariableType)(m_prgVarDesc[uVarIndex].varType);
    }

    RegisterType GetRegType(UINT32 uVarIndex) const
    {
        static const RegisterType rt[] =
        {
#if WPFGFX_FXJIT_X86
            rtGPR,    //vtPointer
            rtGPR,    //vtUINT32
            rtMMX,    //vtMm
            rtXMM,    //vtXmm
            rtXMM,    //vtXmmF1
            rtXMM,    //vtXmmF4
#else //_AMD64_
            rtGPR,    //vtPointer
            rtGPR,    //vtUINT32
            rtGPR,    //vtUINT64
            rtXMM,    //vtXmm
            rtXMM,    //vtXmmF1
            rtXMM,    //vtXmmF4
#endif
        };

        return rt[GetVarType(uVarIndex)];
    }

    bool VarIsInitialized(UINT32 uVarIndex) const
    {
        WarpAssert(uVarIndex < m_uVarsCount);
        return m_prgVarDesc[uVarIndex].varInitialized != 0;
    }

    void VarSetInitialized(UINT32 uVarIndex)
    {
        WarpAssert(uVarIndex < m_uVarsCount);
        m_prgVarDesc[uVarIndex].varInitialized = 1;
    }

    VarDesc const * GetVarDesc() const
    {
        return m_prgVarDesc;
    }

    UINT32 GetVarsCount() const { return m_uVarsCount; }

    UINT8* AllocMem(UINT32 cbSize)
    {
        return m_memory.Alloc(cbSize);
    }

    BOOL WasOverflow() const
    {
        return m_memory.WasOverflow();
    }

    void SetClientData(void *pClientData) { m_pClientData = pClientData; }
    void* GetClientData() const { return m_pClientData; }

#if DBG_DUMP
    void SetDumpFile(WarpPlatform::FileHandle hDumpFile) { m_hDumpFile = hDumpFile; }
    bool IsDumpEnabled() const { return m_hDumpFile != NULL; }
    void DumpOperator(COperator const* pOperator, void *pInstruction);
#endif //DBG_DUMP

    UINT32 GetSpanCount() const { return m_uSpanCount; }
    OpSpan *GetSpanGraph() { return m_pSpanGraph; }

    bool IsEBP_Allowed() const { return m_fEBP_Allowed; }

    UINT32 GetDistanceToConsumer(const COperator *pOperator, UINT32 uVarID);

    UINT32 GetFramePointerID() const { return m_uFramePointerID; }

#if WPFGFX_FXJIT_X86
#else // _AMD64_
    UINT32 GetArgument1ID() const { return m_uArgument1ID; }
#endif

private:
    __checkReturn HRESULT GrowOperators(UINT32 uDelta = 100);
    __checkReturn HRESULT GrowVars();

    UINT_PTR SnapData(uu32x1 const& src);
    UINT_PTR SnapData(uu32x2 const& src);
    UINT_PTR SnapData(uu32x4 const& src);

    // program analysis
    __checkReturn HRESULT BuildSpanGraph();
    __checkReturn HRESULT BuildDependencyGraph();
    __checkReturn HRESULT GatherProviders(COperator *pOperator, UINT32 varID);
    __checkReturn HRESULT GatherExternalProviders(CSpanList & list, COperator *pOperator, UINT32 varID);
    __checkReturn HRESULT BuildVarUsageTables();
    void InspectProviders(COperator *pOperator, UINT32 varID);
    void InspectExternalProviders(CSpanList & list, COperator *pOperator, UINT32 varID);

    __checkReturn HRESULT AddLink(COperator *pConsumer, COperator *pProvider);
    Link * AllocLink();
    void AssertValidLink(Link *pLink);
    void RecycleLink(Link *pLink);
    void RemoveLink(Link *pLink);

    SpanLink * AllocSpanLink();
    __checkReturn HRESULT AddSpanLink(OpSpan * pConsumer, OpSpan * pProvider);
    void AssertValidSpanLink(SpanLink *pLink);

    __checkReturn HRESULT AddHook(COperator *pBlocker, COperator *pDependent);

    void NopifyOperator(COperator *Operator);
    void RedirectOperator(COperator *Operator, UINT32 uNewVResult);

    Hook * AllocHook();
    void RecycleHook(Hook *pHook);

    InstructionHook * AllocInstructionHook();
    void RecycleInstructionHook(InstructionHook *pHook);

    __checkReturn HRESULT ConvertToSSA();

    __checkReturn HRESULT BuildInstructionGraph();
    __checkReturn HRESULT BuildSpanInstructionGraph(OpSpan * pSpan);
    CConnector * EnsureConnector(CConnector **pList, UINT32 uVar);
    __checkReturn HRESULT AddInstructionHook(CInstruction * pIn, CInstruction * pInProvider);
    __checkReturn HRESULT ConnectSpans();
    __checkReturn HRESULT ConnectTwoSpans(OpSpan * pProvider, OpSpan * pConsumer, bool * pfInputsUpdated);

    // optimization
    __checkReturn HRESULT Reduce();
    __checkReturn HRESULT Think(COperator * pOperator);
    __checkReturn HRESULT Rethink(COperator * pOperator);
    __checkReturn HRESULT RemoveAssignUp(COperator * pOperator);
    __checkReturn HRESULT RemoveAssignDown(COperator * pOperator);
    __checkReturn HRESULT OptimizeLoadDWord(COperator * pOperator);
    __checkReturn HRESULT OptimizeAndNot(COperator * pOperator);

    __checkReturn HRESULT OptimizePtrCompute(COperator * pOperator);
    __checkReturn HRESULT OptimizeIndicesUsage(COperator * pOperator);
    __checkReturn HRESULT OptimizePointersArithmetic(COperator * pOperator);

    bool VarUnchangedInBetween(const COperator * pFrom, const COperator * pTo, UINT32 uVar);
    bool VarUnusedInBetween(const COperator * pFrom, const COperator * pTo, UINT32 uVar);
    Link* FindUniqueProvider(COperator * pOperator, UINT32 uOperand);
    bool IsUniqueProvider(const COperator * pOperator) const;
    bool IsSimpleVar(UINT32 uVar) const;
    void RemoveUnused();
    void SetInUse(COperator *pOperator);
    __checkReturn HRESULT Shuffle();
    __checkReturn HRESULT CheckImplicitDependencies(COperator *pOperator, UINT32 varID);
    __checkReturn HRESULT CheckFlagsDependencies(const COperator *pOperator);
    __checkReturn HRESULT ShuffleSpan(OpSpan * pSpan);
    COperator* ChooseNextOperator(struct ShuffleCtx &ctx);

    __checkReturn HRESULT CompressConstants();

    __checkReturn HRESULT Assemble(__deref_out UINT8 ** ppBinaryCode);

    struct Flow
    {
        Flow();
        COperator **m_prgOperators;
        UINT32 m_uOperatorsCount;
        UINT32 m_uOperatorsAllocated;
        bool m_fReversed;
    };

    void SwapFlow(Flow &currentFlow, Flow &newFlow);
    void AppendFlow(Flow &flow);

#if DBG_DUMP
    void Dump();
    void DumpConstants();
    void DumpSpans();
public:
    void DbgDump();

#endif

private:
    // Operators storage.
    COperator **m_prgOperators;
    UINT32 m_uOperatorsCount;
    UINT32 m_uOperatorsAllocated;

    CInstruction **m_prgInstructions;

    // Operators storage for split flow.
    Flow m_flowMain;
    Flow m_flowSplit[MAX_FLOWS];

    bool m_fFlowIsSplit;
    UINT32 m_uCurrentFlow;

    // Variables storage.
    VarDesc *m_prgVarDesc;
    UINT32 m_uVarsCount;
    UINT32 m_uVarsAllocated;

    UINT16 const m_usCallParametersSize;

    UINT32 m_uCodeSize;

    // static variable control
    StaticStorage<uu32x1> m_storage4;
    StaticStorage<uu32x2> m_storage8;
    StaticStorage<uu32x4> m_storage16;

    // Dependency graph storage
    UINT32 m_uSpanCount;
    OpSpan *m_pSpanGraph;
    COperator **m_prgVarSources;
    UINT32 m_uBitArraySize;
    Link * m_pRecycledLinks;
    Hook * m_pRecycledHooks;

    // Memory allocation.
    CFlushMemory m_memory;
    COperator *m_pDummyOperator;

    UINT32 m_uFramePointerID;

    // optimization pass storage
    Hook * m_pRethinkList;

#if WPFGFX_FXJIT_X86
#else // _AMD64_
    UINT32 m_uArgument1ID;
#endif

    void *m_pClientData;
#if DBG_DUMP
    WarpPlatform::FileHandle m_hDumpFile;
#endif //DBG_DUMP

    bool m_fEBP_Allowed;
    bool m_fEnableShuffling;
    bool m_fEnableMemShuffling;
    bool m_fEnableTotalBubbling;
    bool m_fUseNegativeStackOffsets;

public:
    bool m_fUseSSE41;
    bool m_fAvoidMOVDs;
    bool m_fReturnPresents;
};

struct SpanLink
{
    SpanLink *m_pNextProvider;
    OpSpan *m_pProvider;

    SpanLink *m_pNextConsumer;
    OpSpan *m_pConsumer;
};


//+-----------------------------------------------------------------------------
//
//  Struct:
//      OpSpan
//
//  Synopsis:
//      Represents a linear sequence of operators that does not
//      contain control transfers, except maybe last operator of the span.
//
//      All spans of a program constitute a span graph stored as
//      linear array pointed by CProgram::m_pSpanGraph.
//      After executing the last operator in the span, the control
//      is conveyed either to the first operator of the next span in the array,
//      or to the first operator of alternative span pointed by m_pAltNext
//      member of this span.
//------------------------------------------------------------------------------
struct OpSpan
{
    // The index of the first operator in the span.
    UINT32 m_uFirst;

    // The index of the last operator in the span.
    UINT32 m_uLast;

    // The pointer to the chain of spans that enumerates spans that can get control after this span.
    SpanLink *m_pConsumers;

    // The pointer to the chain of spans that enumerates spans that can precede this span.
    SpanLink *m_pProviders;

    // Variables that are in use before first operator in the span
    CBitArray * m_pVarsInUseBefore;

    // Variables that are in use before last operator in the span
    CBitArray * m_pVarsInUseAfter;

    // Variables that change in this span
    CBitArray * m_pVarsChanged;

    // Variables that are used in this span
    CBitArray * m_pVarsUsed;

    // CSpanList storage
    OpSpan *m_pNextWork;

    // The mark that tells that this span is included in CSpanList::m_pTodo list.
    bool m_fInTodoList;

    // The mark that tells that this span is included in CSpanList::m_pDone list.
    bool m_fInDoneList;

    // CProgram::GetDistanceToConsumer() data
    UINT32 m_uDistance;

    // CProgram::ShuffleSpan data
    UINT32 m_uLongestChainSize;
    UINT32 m_uVariety;

    // Instruction graph data
    CConnector * m_pInputs;
    CConnector * m_pOutputs;
};


