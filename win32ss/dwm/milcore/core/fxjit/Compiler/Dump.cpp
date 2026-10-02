// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Class CProgram debugging routines .
//
//-----------------------------------------------------------------------------

#include "precomp.h"

#if DBG_DUMP

static const char* sc_opNames[] =
{
#define DEFINE_OPNAME2(name)  #name
#define DEFINE_OPNAME(name) DEFINE_OPNAME2(ot##name),
    OPERATIONS(DEFINE_OPNAME)
};

void
CProgram::Dump()
{
    if(IsDumpEnabled())
    {
        for (UINT32 u = 0; u < m_uOperatorsCount; u++)
        {
            DumpOperator(m_prgOperators[u], NULL);
        }
    }
}

//------------------------------------------------------------------------
// CProgram::DbgDump() intended for ad hoc debugging.
// It is handy when the troubles apprear earlier than CProgram::Dump()
// is called. Usually it is not called from anywhere, by design.
void
CProgram::DbgDump()
{
    WarpPlatform::FileHandle hSaved = m_hDumpFile;
    m_hDumpFile = WarpPlatform::FileOpen("DbgDump.txt", WarpPlatform::Write);
    if (m_hDumpFile)
    {
        for (UINT32 u = 0; u < m_uOperatorsCount; u++)
        {
            DumpOperator(m_prgOperators[u], (void*)m_prgOperators[u]->GetBinaryOffset());
        }

        DumpSpans();

        WarpPlatform::FileClose(m_hDumpFile);
    }
    m_hDumpFile = hSaved;
}

void
CProgram::DumpOperator(COperator const* pOperator, void *pInstruction)
{
    WarpPlatform::FilePrintf(m_hDumpFile,
        "%3do: 0x%08x: %30s %4dv %4dv %4dv %4dv %c",
        pOperator->m_uOrder,
        pInstruction,
        sc_opNames[pOperator->m_ot],
        pOperator->m_vResult,
        pOperator->m_vOperand1,
        pOperator->m_vOperand2,
        pOperator->m_vOperand3,
        "1248BRSD"[pOperator->m_refType%8]
        );

    if (pOperator->m_pProviders)
    {
        WarpPlatform::FilePrintf(m_hDumpFile, " Providers:");
        for (Link *pLink = pOperator->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
        {
            WarpPlatform::FilePrintf(m_hDumpFile, " %do", pLink->m_pProvider->m_uOrder);
        }
    }

    if (pOperator->m_pConsumers)
    {
        WarpPlatform::FilePrintf(m_hDumpFile, " Consumers:");
        for (Link *pLink = pOperator->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
        {
            WarpPlatform::FilePrintf(m_hDumpFile, " %do", pLink->m_pConsumer->m_uOrder);
        }
    }

    WarpPlatform::FilePrintf(m_hDumpFile, "\n");
};

void
CProgram::DumpConstants()
{
    UINT32 uConstID = 0;
    {
        UINT32 *pData = m_storage16.GetFinalLocation();
        UINT32 uDataSize = m_storage16.GetFinalCount();

        for (UINT32 u = 0; u < uDataSize; u++, pData += 4)
        {
            UINT32 combineOR = pData[0]|pData[1]|pData[2]|pData[3];
            UINT32 combineAND = pData[0]&pData[1]&pData[2]&pData[3];
            if(combineOR>256 &&
             ((combineOR&0x00ffffff)!=0) &&
             ((combineAND&0x00ffffff)!=0xffffff) &&
             ((combineAND&0xffffff00)!=0xffffff00) &&
               combineOR!=0x00400000 &&
               combineAND!=0x4eff0000)
            {
                WarpPlatform::FilePrintf(m_hDumpFile,
                    "Constant_%03d 0x%08X: 0x%08x 0x%08x 0x%08x 0x%08x (%g %g %g %g)\n",
                    uConstID++,
                    pData,
                    pData[0],
                    pData[1],
                    pData[2],
                    pData[3],
                    (double)*(float*)&pData[0],
                    (double)*(float*)&pData[1],
                    (double)*(float*)&pData[2],
                    (double)*(float*)&pData[3]
                );
            }
            else
            {
                WarpPlatform::FilePrintf(m_hDumpFile,
                    "Constant_%03d 0x%08X: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                    uConstID++,
                    pData,
                    pData[0],
                    pData[1],
                    pData[2],
                    pData[3]
                );
            }
        }
    }

    {
        UINT32 *pData = m_storage8.GetFinalLocation();
        UINT32 uDataSize = m_storage8.GetFinalCount();

        for (UINT32 u = 0; u < uDataSize; u++, pData += 2)
        {
            UINT32 combineOR = pData[0]|pData[1];
            UINT32 combineAND = pData[0]&pData[1];
            if(combineOR>256 &&
             ((combineOR&0x00ffffff)!=0) &&
             ((combineAND&0x00ffffff)!=0xffffff) &&
             ((combineAND&0xffffff00)!=0xffffff00) &&
               combineOR!=0x00400000 &&
               combineAND!=0x4eff0000)
            {
                WarpPlatform::FilePrintf(m_hDumpFile,
                    "Constant_%03d 0x%08X: 0x%08x 0x%08x (%g %g)\n",
                    uConstID++,
                    pData,
                    pData[0],
                    pData[1],
                    (double)*(float*)&pData[0],
                    (double)*(float*)&pData[1]
                );
            }
            else
            {
                WarpPlatform::FilePrintf(m_hDumpFile,
                    "Constant_%03d 0x%08X: 0x%08x 0x%08x\n",
                    uConstID++,
                    pData,
                    pData[0],
                    pData[1]
                );
            }
        }
    }

    {
        UINT32 *pData = m_storage4.GetFinalLocation();
        UINT32 uDataSize = m_storage4.GetFinalCount();

        for (UINT32 u = 0; u < uDataSize; u++, pData += 1)
        {
            if(pData[0]>256 &&
             ((pData[0]&0x00ffffff)!=0) &&
             ((pData[0]&0x00ffffff)!=0xffffff) &&
             ((pData[0]&0x00ffffff)!=0xffffff00) &&
               pData[0]!=0x00400000 &&
               pData[0]!=0x4eff0000)
            {
                WarpPlatform::FilePrintf(m_hDumpFile,
                    "Constant_%03d 0x%08X: 0x%08x (%g)\n",
                    uConstID++,
                    pData,
                    pData[0],
                    (double)*(float*)&pData[0]
                );
            }
            else
            {
                WarpPlatform::FilePrintf(m_hDumpFile,
                    "Constant_%03d 0x%08X: 0x%08x\n",
                    uConstID++,
                    pData,
                    pData[0]
                );
            }
        }
    }
    if (uConstID)
    {
        WarpPlatform::FilePrintf(m_hDumpFile, "\n\n");
    }
}

void
CProgram::DumpSpans()
{
    WarpPlatform::FilePrintf(m_hDumpFile, "\n\n");

    for (UINT32 uSpan = 0; uSpan < m_uSpanCount; uSpan++)
    {
        OpSpan * pSpan = m_pSpanGraph + uSpan;
        char *type;

        COperator * pLastOperator = m_prgOperators[pSpan->m_uLast];

             if (pLastOperator->IsLoopStart  ()) type = "LoopStart";
        else if (pLastOperator->IsLoopRepeat ()) type = "LoopRepeat";
        else if (pLastOperator->IsBranchSplit()) type = "BranchSplit";
        else if (pLastOperator->IsBranchMerge()) type = "BranchMerge";
        else if (pLastOperator->m_ot == otReturn) type = "Return";
        else if (pLastOperator->m_ot == otSubroutineCall) type = "SubroutineCall";
        else if (pLastOperator->m_ot == otSubroutineReturn) type = "SubroutineReturn";
        else type = "Unknown !!! UPDATE DUMP ROUTINES !!!";

        WarpPlatform::FilePrintf(m_hDumpFile,
            "Span%02d: type = %s\n",
            uSpan,
            type
            );

        WarpPlatform::FilePrintf(m_hDumpFile,
            "    start = %d; end = %d; size = %d; max chain = %d; variety = %d;\n",
            pSpan->m_uFirst,
            pSpan->m_uLast,
            pSpan->m_uLast - pSpan->m_uFirst + 1,
            pSpan->m_uLongestChainSize,
            pSpan->m_uVariety
            );

        UINT32 uInputCount = 0;
        UINT32 uOutputCount = 0;
        UINT32 uPassedThruCount = 0;
        UINT32 uUsedAndKept = 0;
        {
            for (CConnector * pc = pSpan->m_pInputs; pc; pc= pc->m_pNext)
            {
                uInputCount++;
            }
            for (CConnector * pc = pSpan->m_pOutputs; pc; pc= pc->m_pNext)
            {
                uOutputCount++;
                InstructionHook * pProviderHook = pc->m_instruction.m_pProviders;
                WarpAssert(pProviderHook);
                CInstruction * pProvider = pProviderHook->m_pProvider;
                WarpAssert(pProvider);
                if (pProvider->m_pOperator == NULL)
                {
                    if (pProvider->m_uConsumersCount == 1)
                    {
                        uPassedThruCount++;
                    }
                    else
                    {
                        uUsedAndKept++;
                    }
                }
            }
        }

        WarpPlatform::FilePrintf(m_hDumpFile,
            "    number of inputs = %d, including: passed thru = %d; used and kept = %d; used and not kept = %d\n",
            uInputCount,
            uPassedThruCount,
            uUsedAndKept,
            uInputCount - uPassedThruCount - uUsedAndKept
            );

        WarpPlatform::FilePrintf(m_hDumpFile,
            "    number of outputs = %d, including: given = %d; computed = %d\n",
            uOutputCount,
            uPassedThruCount + uUsedAndKept,
            uOutputCount - uPassedThruCount - uUsedAndKept
            );

        WarpPlatform::FilePrintf(m_hDumpFile, "    Preceding spans:");
        if (pSpan->m_pProviders == NULL)
        {
            WarpPlatform::FilePrintf(m_hDumpFile, " none\n");
        }
        else
        {
            for (SpanLink * pLink = pSpan->m_pProviders; pLink; pLink = pLink->m_pNextProvider)
            {
                OpSpan * pProvider = pLink->m_pProvider;
                WarpPlatform::FilePrintf(m_hDumpFile, " %d", pProvider - m_pSpanGraph);
            }
            WarpPlatform::FilePrintf(m_hDumpFile, "\n");
        }

        WarpPlatform::FilePrintf(m_hDumpFile, "    Following spans:");
        if (pSpan->m_pConsumers == NULL)
        {
            WarpPlatform::FilePrintf(m_hDumpFile, " none\n");
        }
        else
        {
            for (SpanLink * pLink = pSpan->m_pConsumers; pLink; pLink = pLink->m_pNextConsumer)
            {
                OpSpan * pConsumer = pLink->m_pConsumer;
                WarpPlatform::FilePrintf(m_hDumpFile, " %d", pConsumer - m_pSpanGraph);
            }
            WarpPlatform::FilePrintf(m_hDumpFile, "\n");
        }

        WarpPlatform::FilePrintf(m_hDumpFile, "\n");
    }
}

#endif

