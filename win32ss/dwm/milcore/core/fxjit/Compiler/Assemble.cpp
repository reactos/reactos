// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of class CAssembleContext.
//
//-----------------------------------------------------------------------------
#include "precomp.h"


static const UINT32 uPageSize = 4096;

CAssembleContext::CAssembleContext(CMapper const & mapper, bool fUseNegativeStackOffsets)
: m_mapper(mapper)
{
    m_uOperatorFlags = 0;
    m_uEspOffset = fUseNegativeStackOffsets ? 128 : 0;
}

#if WPFGFX_FXJIT_X86

void
CAssembleContext::AssemblePrologue(
    __in UINT32 uFrameSize,
    __in UINT32 uFrameAlignment
    )
{
    push (reg_ebp);
    mov (reg_ebp, reg_esp);
    push (reg_ebx);
    push (reg_esi);
    push (reg_edi);

    //
    // Reserve memory for stack frame.
    // The system allocates stack memory dynamically, by one page at a time.
    // When stack frame is large it can occupy several pages.
    // We should sequentially touch every page to provide proper page fault handling.
    //

    while (uFrameSize > uPageSize)
    {
        subImm(reg_esp, uPageSize);
        cmd(mov_rm, reg_eax, memptr(reg_esp, 0));
        uFrameSize -= uPageSize;
    }

    UINT32 uEspOffset = uFrameSize - m_uEspOffset;

    if (uEspOffset)
    {
        subImm (reg_esp, uEspOffset);
    }

    if (uFrameAlignment)
    {
        andImm (reg_esp, ~uFrameAlignment);
    }
}
#else //_AMD64_

void
CAssembleContext::AssemblePrologue(
    __in UINT32 uFrameSize,
    __in UINT32 /*uFrameAlignment*/
    )
{
    //
    // Consider adjusting CAssembleContext::sc_uArgOffset
    // if the way to initialize ebp changes.
    //
    push (reg_rbp);
        //stack aligned to 16 bytes
    cmd(mov_64_rr, reg_rbp, reg_rsp);

    // First four call arguments are passed in registers that have
    // preallocated shadow slots in frame stack:
    //
    // 1st argument: rcx or xmm0, qword ptr [rsp + 10h] // assuminng rsp shifted by "push ebp" above
    // 2nd argument: rdx or xmm1, qword ptr [rsp + 18h]
    // 3rd argument: r8  or xmm2, qword ptr [rsp + 20h]
    // 4th argument: r9  or xmm3, qword ptr [rsp + 28h]
    //
    // Following code utilizes these slots as registry save storage.
    //

    cmd(mov_64_mr, memptr(reg_rbp, 0x10), reg_rbx);
    cmd(mov_64_mr, memptr(reg_rbp, 0x18), reg_rsi);
    cmd(mov_64_mr, memptr(reg_rbp, 0x20), reg_rdi);
    cmd(mov_64_mr, memptr(reg_rbp, 0x28), reg_r12);

        //stack aligned to 16 bytes
    push (reg_r13);         // placed at [rbp-0x08]
    push (reg_r14);         // placed at [rbp-0x10]
        //stack aligned to 16 bytes
    push (reg_r15);         // placed at [rbp-0x18]
        //stack aligned to (16*n+8)

    //
    // Reserve space to save 10 XMM registers, plus 8 bytes to align.
    //
    uFrameSize = ((uFrameSize + 0xF) & ~0xF) + 10*0x10 + 8;

    //
    // Reserve memory for stack frame.
    // The system allocates stack memory dynamically, by one page at a time.
    // When stack frame is large it can occupy several pages.
    // We should sequentially touch every page to provide proper page fault handling.
    //

    while (uFrameSize > uPageSize)
    {
        subImmWhole (reg_rsp, uPageSize);
        cmd(mov_rm, reg_rax, memptr(reg_rsp, 0));
        uFrameSize -= uPageSize;
    }

    UINT32 uEspOffset = uFrameSize - m_uEspOffset;

    if (uEspOffset)
    {
        subImmWhole (reg_rsp, uEspOffset);
    }

    cmd(movaps_mr, memptr(reg_rbp, -0x30), reg_xmm6 );
    cmd(movaps_mr, memptr(reg_rbp, -0x40), reg_xmm7 );
    cmd(movaps_mr, memptr(reg_rbp, -0x50), reg_xmm8 );
    cmd(movaps_mr, memptr(reg_rbp, -0x60), reg_xmm9 );
    cmd(movaps_mr, memptr(reg_rbp, -0x70), reg_xmm10);
    cmd(movaps_mr, memptr(reg_rbp, -0x80), reg_xmm11);
    cmd(movaps_mr, memptr(reg_rbp, -0x90), reg_xmm12);
    cmd(movaps_mr, memptr(reg_rbp, -0xA0), reg_xmm13);
    cmd(movaps_mr, memptr(reg_rbp, -0xB0), reg_xmm14);
    cmd(movaps_mr, memptr(reg_rbp, -0xC0), reg_xmm15);

}
#endif

void
#if DBG_DUMP
CAssembleContext::AssembleProgram(CProgram * pProgram, bool fDumpEnabled)
#else
CAssembleContext::AssembleProgram(CProgram * pProgram)
#endif //DBG_DUMP
{
    COperator ** ppOperators = pProgram->GetOperators();
    UINT32 uOpCount = pProgram->GetOperatorsCount();

    for (UINT32 u = 0; u < uOpCount; u++)
    {
        COperator *pOperator = ppOperators[u];
#if DBG_DUMP
        if (fDumpEnabled)
        {
            pProgram->DumpOperator(pOperator, (UINT8*)GetBase() + GetCount());
        }
#endif

        // Calculate or-combined flags so that otReturn operator will know
        // whether MMX instructions were used so that EMMS should be generated.
        m_uOperatorFlags |= pOperator->GetFlags();

        for (CShuffleRecord * psr = pOperator->m_pShuffles; psr; psr = psr->m_pNext)
        {
            psr->Assemble(*this, m_mapper);
        }

        pOperator->m_uBinaryOffset = m_uCount;

        m_pCurrentOperator = pOperator;

        if (pOperator->IsIrregular())
        {
            pOperator->AssembleIrregular(*this);
        }
        else if (pOperator->IsStandardBinary())
        {
            pOperator->AssembleBinary(*this);
        }
        else if (pOperator->IsStandardUnary())
        {
            pOperator->AssembleUnary(*this);
        }
        else if (pOperator->IsStandardMemDst())
        {
            pOperator->AssembleMemDst(*this);
        }
        else
        {
            pOperator->Assemble(*this);
        }

        m_pCurrentOperator = NULL;
    }
}

UINT32
CAssembleContext::GetOffset(UINT32 uVarID) const
{
    return m_mapper.GetVarOffset(uVarID);
}

CAssemblePass2::CAssemblePass2(
        CMapper const & mapper,
        bool fUseNegativeStackOffsets,
        UINT8 * pData,
        INT_PTR uStatic4Offset,
        INT_PTR uStatic8Offset,
        INT_PTR uStatic16Offset
        )
        : CAssembleContext(mapper, fUseNegativeStackOffsets)
        , m_pData(pData)
        , m_uStatic4Offset(uStatic4Offset)
        , m_uStatic8Offset(uStatic8Offset)
        , m_uStatic16Offset(uStatic16Offset)
{
}

void*
CAssemblePass2::Place(void* pData, UINT32 dataType)
{
    void* result = NULL;
    switch(dataType)
    {
    case ofDataR32:
    case ofDataM32:
    case ofDataI32:
    case ofDataF32:
        result= (UINT8*)pData + m_uStatic4Offset;
        break;

    case ofDataM64:
    case ofDataI64:
        result= (UINT8*)pData + m_uStatic8Offset;
        break;

    case ofDataI128:
    case ofDataF128:
        result= (UINT8*)pData + m_uStatic16Offset;
        break;

    default:
        NO_DEFAULT;
    }
    return result;
}


