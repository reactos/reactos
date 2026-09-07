// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of class COperator.
//
//-----------------------------------------------------------------------------
#include "precomp.h"

COperator::COperator(OpType ot, UINT32 vResult, UINT32 vOperand1, UINT32 vOperand2, UINT32 vOperand3)
{
    m_ot = ot;
    m_uFlags = 0;

    m_vResult   = vResult  ;
    m_vOperand1 = vOperand1;
    m_vOperand2 = vOperand2;
    m_vOperand3 = vOperand3;

    m_refType = RefType_Direct;
    m_uDisplacement = 0;

    m_pLinkedOperator = NULL;

    m_pShuffles = NULL;

    m_uBinaryOffset = UINT32(-1);

    m_uSpanIdx = UINT32(-1);
    m_pNextVarProvider = NULL;
    m_pProviders = NULL;
    m_pConsumers = NULL;
    m_pDependents = NULL;
    m_uBlockersCount = 0;
    m_uChainSize = 0;
}

// immediate byte for cmpps
enum CompareType : UINT8
{
    CompareType_EQ    = 0,
    CompareType_LT    = 1,
    CompareType_LE    = 2,
    CompareType_UNORD = 3,
    CompareType_NEQ   = 4,
    CompareType_NLT   = 5,
    CompareType_NLE   = 6,
    CompareType_ORD   = 7
};

// immediate byte for roundps (SSE4.1)
enum RoundingType : UINT8
{
    RoundingType_NearestEven = 0,
    RoundingType_Down        = 1,
    RoundingType_Up          = 2,
    RoundingType_Truncate    = 3,
};

// Assemble the operation that requires special register mapping
void
COperator::AssembleIrregular(CAssembleContext & actx)
{
    switch (m_ot)
    {
    case otUINT32Div:
    case otUINT32Rem:
    case otINT32Div:
    case otINT32Rem:
        {
            WarpAssert(RegGPROperand1() == gax);
            WarpAssert(RegGPRResult() == gax && (m_ot == otUINT32Div || m_ot == otINT32Div)
                    || RegGPRResult() == gdx && (m_ot == otUINT32Rem || m_ot == otINT32Rem));

            bool fSigned = m_ot == otINT32Div || m_ot == otINT32Rem;

            if (fSigned)
            {
                actx.cdq();
            }
            else
            {
                actx.cmd(xor, CRegID(gdx), CRegID(gdx), 0, 0);
            }

            switch (m_refType)
            {
            case RefType_Direct:
                {
                    //
                    // Direct addressing: second operand is a variable
                    // that can appear either in register or in stack frame.
                    //
                    if (m_rOperand2.IsDefined())
                    {
                        RegGPR src2 = RegGPROperand2();
                        actx.div(src2, fSigned);
                    }
                    else
                    {
                        UINT32 offset = actx.GetOffset(m_vOperand2);
                        actx.div(actx.FramePtr(offset), fSigned);
                    }
                }
                break;
            case RefType_Static:
                {
                    //
                    // Indirect addressing with static immediate pointer: the operand is undefined,
                    // data resides in temporary memory pointed by m_pData.
                    //
                    WarpAssert(m_vOperand2 == 0);

                    actx.div(memptr(actx.Place(m_pData, GetDataType())), fSigned);
                }
                break;
            case RefType_Base:
                {
                    //
                    // Indirect addressing with pointer and offset: second operand is a pointer
                    // to array (or structure); data resides in memory pointed to this pointer
                    // with offset stored in m_uDisplacement.
                    //
                    WarpAssert(m_vOperand2 != 0);

                    RegGPR pBase = RegGPROperand2();
                    actx.div(memptr(pBase, m_uDisplacement), fSigned);
                }
                break;
            default:
                {
                    //
                    // Indirect addressing with indexing: second operand is an index;
                    // data reside in memory pointed to by m_uDisplacement plus
                    // index with given scale.
                    //

                    WarpAssert(m_vOperand2 != 0);
                    RegGPR index = RegGPROperand2();
                    actx.div(memptr(m_pData, index, (Scale32)m_refType), fSigned);
                }
                break;
            }
        }
        break;

    case otUINT32ShiftLeft:
        {
            RegGPR dst  = RegGPRResult();
            RegGPR src1 = RegGPROperand1();
            WarpAssert(RegGPROperand2() == gcx);

            if (dst != src1) actx.mov(dst, src1);
            actx.shl(dst);
        }
        break;


    case otUINT32ShiftRight:
        {
            RegGPR dst  = RegGPRResult();
            RegGPR src1 = RegGPROperand1();
            WarpAssert(RegGPROperand2() == gcx);

            if (dst != src1) actx.mov(dst, src1);
            actx.shr(dst);
        }
        break;


    case otINT32ShiftRight:
        {
            RegGPR dst  = RegGPRResult();
            RegGPR src1 = RegGPROperand1();
            WarpAssert(RegGPROperand2() == gcx);

            if (dst != src1) actx.mov(dst, src1);
            actx.sar(dst);
        }
        break;


    case otXmmStoreNonTemporalMasked:
        {
            RegXMM src  = RegXMMOperand1();
            RegXMM mask = RegXMMOperand2();
            WarpAssert(RegGPROperand3() == gdi);

            actx.maskmovdqu(src, mask);
        }
        break;

    case otXmmBytesBlend:
        {
            if (m_rResult != m_rOperand1)
            {
                actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
            }

            WarpAssert(m_rOperand3 == CRegID(reg_xmm0));

            actx.cmd(pblendvb, m_rResult, m_rOperand2);
        }
        break;

    case otCall:
        {
            UINT32 uEspOffset = actx.GetEspOffset();

#if WPFGFX_FXJIT_X86
            if (uEspOffset)
            {
                actx.subImmWhole (gsp, uEspOffset);
            }
            actx.push(gcx);
            actx.callImm(m_uDisplacement);
            if (uEspOffset)
            {
                actx.addImmWhole (gsp, uEspOffset);
            }
#else // _AMD64_
            actx.subImmWhole (gsp, uEspOffset + 4*sizeof(void*));
            actx.movImmWhole(gax, m_uDisplacement);
            actx.call(CRegID(gax));
            actx.addImmWhole (gsp, uEspOffset + 4*sizeof(void*));
#endif
        }
        break;

        default:
            NO_DEFAULT;
    }
}

// Assemble binary operation that respects RefType for second and third operands
void
COperator::AssembleBinary(CAssembleContext & actx)
{
    UINT32 opCode = sc_opCodes[m_ot];
    WarpAssert(opCode);

    UINT32 dataType = GetDataType();
    UINT32 movCode = sc_movCodesRM[dataType];
    WarpAssert(movCode);

#if WPFGFX_FXJIT_X86
#else // _AMD64_
    opCode |= (movCode & REX_W);
#endif

    UINT32 immSize = 0, immData = 0;
    if (HasImmediateByte())
    {
        immSize = 1;
        if (HasOpcodeSuffix())
        {
            switch (m_ot)
            {
            case otXmmFloat4CmpEQ : immData = CompareType_EQ ; break;
            case otXmmFloat4CmpLT : immData = CompareType_LT ; break;
            case otXmmFloat4CmpLE : immData = CompareType_LE ; break;
            case otXmmFloat4CmpNEQ: immData = CompareType_NEQ; break;
            case otXmmFloat4CmpNLT: immData = CompareType_NLT; break;
            case otXmmFloat4CmpNLE: immData = CompareType_NLE; break;

            default: NO_DEFAULT;
            }
        }
        else
        {
            immData = m_bImmediateByte;
        }
    }

    switch (m_refType)
    {
    case RefType_Direct:
        {
            //
            // Direct addressing: second operand is a variable
            // that can appear either in register or in stack frame.
            //
            if (m_rOperand2.IsDefined())
            {
                CRegID src2 = m_rOperand2;
                if (m_rResult == m_rOperand1)
                {
                    actx.cmd(opCode, m_rResult, src2, immSize, immData);
                }
                else if (CanSwapOperands() && m_rResult == src2)
                {
                    actx.cmd(opCode, m_rResult, m_rOperand1, immSize, immData);
                }
                else
                {
                    actx.cmd(movCode, m_rResult, m_rOperand1);
                    actx.cmd(opCode, m_rResult, src2, immSize, immData);
                }
            }
            else
            {
                if (m_rResult != m_rOperand1)
                {
                    actx.cmd(movCode, m_rResult, m_rOperand1);
                }

                UINT32 offset = actx.GetOffset(m_vOperand2);
                actx.cmd(opCode, m_rResult, actx.FramePtr(offset), immSize, immData);
            }
        }
        break;
    case RefType_Static:
        {
            //
            // Indirect addressing with static immediate pointer: the operand is undefined,
            // data resides in temporary memory pointed by m_pData.
            //
            WarpAssert(m_vOperand2 == 0);

            if (m_rResult != m_rOperand1)
            {
                actx.cmd(movCode, m_rResult, m_rOperand1);
            }
            actx.cmd(opCode, m_rResult, memptr(actx.Place(m_pData, dataType)), immSize, immData);
        }
        break;
    case RefType_Base:
        {
            //
            // Indirect addressing with pointer and offset: second operand is a pointer
            // to array (or structure); data resides in memory pointed to this pointer
            // with offset stored in m_uDisplacement.
            //
            WarpAssert(m_vOperand2 != 0);

            if (m_rResult != m_rOperand1)
            {
                actx.cmd(movCode, m_rResult, m_rOperand1, 0 ,0);
            }
            RegGPR pBase = RegGPROperand2();
            actx.cmd(opCode, m_rResult, memptr(pBase, m_uDisplacement), immSize, immData);
        }
        break;
    default:
        {
            //
            // Indirect addressing with indexing: second operand is an index;
            // data reside in memory pointed to by m_uDisplacement plus
            // index with given scale.
            //
            if (m_rResult != m_rOperand1)
            {
                actx.cmd(movCode, m_rResult, m_rOperand1);
            }

            WarpAssert(m_vOperand2 != 0);
            if (m_rOperand3 == 0)
            {
                RegGPR index = RegGPROperand2();
                actx.cmd(opCode, m_rResult, memptr(m_pData, index, (Scale32)m_refType), immSize, immData);
            }
            else
            {
                RegGPR base = RegGPROperand2();
                RegGPR index = RegGPROperand3();
                actx.cmd(opCode, m_rResult, memptr(base, index, (Scale32)m_refType, m_uDisplacement), immSize, immData);
            }
        }
        break;
    }
}


// Assemble unary operation that respects RefType for an operand
void
COperator::AssembleUnary(CAssembleContext & actx)
{
    UINT32 opcode = sc_opCodes[m_ot];
    WarpAssert(opcode);

    UINT32 immSize = 0, immData = 0;
    if (HasImmediateByte())
    {
        immSize = 1;
        if (HasOpcodeSuffix())
        {
            switch (m_ot)
            {
            case otXmmFloat4Floor : immData = RoundingType_Down; break;
            case otXmmFloat4Ceil  : immData = RoundingType_Up  ; break;

            default: NO_DEFAULT;
            }
        }
        else
        {
            immData = m_bImmediateByte;
        }
    }

    switch (m_refType)
    {
    case RefType_Direct:
        {
            //
            // Direct addressing: the operand is a variable
            // that can appear either in register or in stack frame.
            //
            if (m_rOperand1.IsDefined())
            {
                actx.cmd(opcode, m_rResult, m_rOperand1, immSize, immData);
            }
            else
            {
                UINT32 offset = actx.GetOffset(m_vOperand1);
                actx.cmd(opcode, m_rResult, actx.FramePtr(offset), immSize, immData);
            }
        }
        break;
    case RefType_Static:
        {
            //
            // Indirect addressing with static immediate pointer: the operand is undefined,
            // data resides in temporary memory pointed by m_pData.
            //
            WarpAssert(m_vOperand1 == 0);

            actx.cmd(opcode, m_rResult, memptr(actx.Place(m_pData, GetDataType())), immSize, immData);
        }
        break;
    case RefType_Base:
        {
            //
            // Indirect addressing with pointer and offset: the operand is a pointer
            // to array (or structure); data resides in memory pointed to this pointer
            // with offset stored in m_uDisplacement.
            //
            WarpAssert(m_vOperand1 != 0);

            RegGPR pBase = RegGPROperand1();
            actx.cmd(opcode, m_rResult, memptr(pBase, m_uDisplacement), immSize, immData);
        }
        break;
    default:
        {
            //
            // Indirect addressing with indexing: second operand is an index;
            // data reside in memory pointed to by m_uDisplacement plus
            // index with given scale.
            //

            WarpAssert(m_vOperand1 != 0);
            if (m_vOperand2 == 0)
            {
                RegGPR index = RegGPROperand1();
                actx.cmd(opcode, m_rResult, memptr(m_pData, index, (Scale32)m_refType), immSize, immData);
            }
            else
            {
                RegGPR pBase = RegGPROperand1();
                RegGPR index = RegGPROperand2();
                actx.cmd(opcode, m_rResult, memptr(pBase, index, (Scale32)m_refType, m_uDisplacement), immSize, immData);
            }
        }
        break;
    }
}

// Assemble operation with memory destination that respects RefType notation.
// These operations 
// for second and third operands
void
COperator::AssembleMemDst(CAssembleContext & actx)
{
    UINT32 opCode = sc_opCodes[m_ot];
    WarpAssert(opCode);

    WarpAssert(m_vOperand2 != 0);

    switch (m_refType)
    {
    case RefType_Base:
        {
            //
            // Indirect addressing with pointer and offset.
            //
            RegGPR pBase = RegGPROperand2();
            actx.cmd(opCode, memptr(pBase, m_uDisplacement), m_rOperand1, 0, 0);
        }
        break;

    case RefType_Index_1:
    case RefType_Index_2:
    case RefType_Index_4:
    case RefType_Index_8:
        {
            //
            // Indirect addressing with pointer, scaled index and offset.
            //
            WarpAssert(m_vOperand3 != 0);

            RegGPR base = RegGPROperand2();
            RegGPR index = RegGPROperand3();
            actx.cmd(opCode, memptr(base, index, (Scale32)m_refType, m_uDisplacement), m_rOperand1, 0, 0);
        }
        break;

    case RefType_Direct:
    case RefType_Static:
    default:
        NO_DEFAULT;
    }
}

void
COperator::Assemble(CAssembleContext & actx)
{
switch (m_ot)
{

case otReturn:
    {
        if (actx.GetOperatorFlags() & ofNonTemporalStore)
        {
            actx.mfence();
        }

#if WPFGFX_FXJIT_X86
        if (actx.GetOperatorFlags() & ofUsesMMX)
        {
            actx.emms();
        }

        RegGPR src1 = RegGPROperand1();
        actx.cmd(lea_ptr, reg_esp, dword(src1, -12));

        actx.pop(reg_edi);
        actx.pop(reg_esi);
        actx.pop(reg_ebx);
        actx.pop(reg_ebp);
        actx.ret(m_immediateData);
#else //_AMD64_
        RegGPR src1 = RegGPROperand1();

        actx.cmd(movaps_rm, reg_xmm6 , memptr(src1, -0x30));
        actx.cmd(movaps_rm, reg_xmm7 , memptr(src1, -0x40));
        actx.cmd(movaps_rm, reg_xmm8 , memptr(src1, -0x50));
        actx.cmd(movaps_rm, reg_xmm9 , memptr(src1, -0x60));
        actx.cmd(movaps_rm, reg_xmm10, memptr(src1, -0x70));
        actx.cmd(movaps_rm, reg_xmm11, memptr(src1, -0x80));
        actx.cmd(movaps_rm, reg_xmm12, memptr(src1, -0x90));
        actx.cmd(movaps_rm, reg_xmm13, memptr(src1, -0xA0));
        actx.cmd(movaps_rm, reg_xmm14, memptr(src1, -0xB0));
        actx.cmd(movaps_rm, reg_xmm15, memptr(src1, -0xC0));

        actx.cmd(lea_64, reg_rsp, memptr(src1, -0x18));

        actx.pop(reg_r15);
        actx.pop(reg_r14);
        actx.pop(reg_r13);

        actx.cmd(mov_64_rm, reg_rbx, memptr(reg_rbp, 0x10));
        actx.cmd(mov_64_rm, reg_rsi, memptr(reg_rbp, 0x18));
        actx.cmd(mov_64_rm, reg_rdi, memptr(reg_rbp, 0x20));
        actx.cmd(mov_64_rm, reg_r12, memptr(reg_rbp, 0x28));

        actx.pop(reg_rbp);
        actx.ret(0);
#endif
    }
    break;

case otLoopRepeatIfNonZero:
    {
        // branch taken prefix (actually I have not seen a profit of it)
        //actx.Emit(0x2E); //branch not taken
        //actx.Emit(0x3E); //branch taken
        actx.jne(((COperator*)m_pLinkedOperator)->GetBinaryOffset());
    }
    break;

case otBranchOnZero:
    {
        actx.je(((COperator*)m_pLinkedOperator)->GetBinaryOffset());
    }
    break;

case otSubroutineCall:
    {
        RegGPR ptr = RegGPROperand1();

        // Tricky way to get return address, that's the address of next instruction after "jmp".
        // Troubles appear in 64-bit system where REX prefixes of "mov"s vary instructions sizes.
        // Following code executes generating twice; first pass to get return address
        // and second for actual code generating.

        UINT_PTR retAddr = 0;

        UINT32 uSavedCount = actx.GetCount();

        for (;;)
        {

            actx.movImm(dword(ptr, 0), (UINT32)retAddr);
#if WPFGFX_FXJIT_X86
#else // _AMD64_
            actx.movImm(dword(ptr, 4), (UINT32)(retAddr >> 32));
#endif
            actx.jmp(((COperator*)m_pLinkedOperator)->GetBinaryOffset());
            if (retAddr)
                break;

            retAddr = actx.GetBase() + actx.GetCount();
            actx.SetCount(uSavedCount); // force redo
        }
    }
    break;

case otSubroutineReturn:
    {
        actx.jmp(m_rOperand1);
    }
    break;

case otPtrAssignArgument:
    {
        RegGPR src1 = RegGPROperand1();
        actx.cmd(mov_ptr_rm, m_rResult, memptr(src1, CAssembleContext::sc_uArgOffset + m_uDisplacement));
    }
    break;

case otPtrAssignMember:
    {
        RegGPR src1 = RegGPROperand1();
        actx.cmd(mov_ptr_rm, m_rResult, memptr(src1, m_uDisplacement));
    }
    break;

case otPtrAssignMemberIndexed:
    {
        RegGPR src1 = RegGPROperand1();
        RegGPR src2 = RegGPROperand2();
        actx.cmd(mov_ptr_rm, m_rResult, memptr(src1, src2, scale_4, m_uDisplacement));
    }
    break;

#if WPFGFX_FXJIT_X86
#else // _AMD64_
case otUINT64ImmAssign:
#endif

case otPtrAssignImm:
    {
        RegGPR dst  = RegGPRResult();
        actx.movImmWhole(dst, m_uDisplacement);
    }
    break;

case otPtrAssign:
    {
        if (m_rResult != m_rOperand1)
        {
            actx.cmd(mov_ptr_rr, m_rResult, m_rOperand1);
        }
    }
    break;

case otPtrCompute:
    {
        RegGPR src1 = RegGPROperand1();

        if (m_refType == RefType_Base)
        {
            WarpAssert(m_vOperand2 == 0);
            actx.cmd(lea_ptr, m_rResult, memptr(src1, m_uDisplacement));
        }
        else
        {
            WarpAssert(m_refType == RefType_Index_1 ||
                       m_refType == RefType_Index_2 ||
                       m_refType == RefType_Index_4 ||
                       m_refType == RefType_Index_8);

            if (m_vOperand2)
            {
                RegGPR src2 = RegGPROperand2();
                actx.cmd(lea_ptr, m_rResult, memptr(src1, src2, (Scale32)m_refType, m_uDisplacement));
            }
            else
            {
                actx.cmd(lea_ptr, m_rResult, memptr(m_pData, src1, (Scale32)m_refType));
            }
        }
    }
    break;

#if WPFGFX_FXJIT_X86
#else //_AMD64_
case otUINT64Assign:
case otUINT64Assign32:
case otUINT32Assign64:
#endif

case otUINT32Assign:
    {
        if (m_rResult != m_rOperand1)
        {
            actx.cmd(mov_rr, m_rResult, m_rOperand1);
        }
    }
    break;

case otUINT32ImmAssign:
    {
        RegGPR dst  = RegGPRResult();
        if (m_immediateData == 0)
        {
            actx.cmd(xor, m_rResult, m_rResult);
        }
        else
        {
            actx.movImm(dst, m_immediateData);
        }
    }
    break;


case otUINT32Increment:
    {
        RegGPR src1 = RegGPROperand1();
        actx.cmd(lea, m_rResult, memptr(src1, 1));
    }
    break;

case otUINT32Decrement:
    {
        RegGPR src1 = RegGPROperand1();
        actx.cmd(lea, m_rResult, memptr(src1, -1));
    }
    break;

case otUINT32DecrementTest:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        if (dst != src1) actx.mov(dst, src1);

        // Note: "dec" instruction is smaller, but "sub" is faster on P4
        actx.subImm(dst, 1);
    }
    break;

case otUINT32Test:
    {
        RegGPR src1 = RegGPROperand1();
        RegGPR src2 = RegGPROperand2();
        actx.test(src1, src2);
    }
    break;


// Binary operations
case otUINT32Add:
    if (m_refType == RefType_Direct && m_rOperand2.IsDefined())
    {
        RegGPR src1 = RegGPROperand1();
        RegGPR src2 = RegGPROperand2();
        actx.cmd(lea, m_rResult, memptr(src1, src2, scale_1));
    }
    else
    {
        AssembleBinary(actx);
    }
    break;

case otUINT32ImmAdd:
    {
        RegGPR src1 = RegGPROperand1();
        actx.cmd(lea, m_rResult, memptr(src1, m_immediateData));
    }
    break;

case otUINT32ImmOr:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        if (dst != src1) actx.mov(dst, src1);
        actx.orImm(dst, m_immediateData);
    }
    break;

case otUINT32ImmAnd:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        if (dst != src1) actx.mov(dst, src1);
        actx.andImm(dst, m_immediateData);
    }
    break;

case otUINT32ImmSub:
    {
        RegGPR src1 = RegGPROperand1();
        actx.cmd(lea, m_rResult, memptr(src1, -(INT32)m_immediateData));
    }
    break;

case otUINT32ImmXor:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        if (dst != src1) actx.mov(dst, src1);
        actx.xorImm(dst, m_immediateData);
    }
    break;

case otUINT32ImmCmp:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        if (dst != src1) actx.mov(dst, src1);
        actx.cmpImm(dst, m_immediateData);
    }
    break;

case otUINT32ImmMul:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        actx.imulImm(dst, src1, m_immediateData);
    }
    break;

case otUINT32ImmShiftRight:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        if (dst != src1) actx.mov(dst, src1);
        actx.shr(dst, m_shift);
    }
    break;


case otUINT32ImmShiftLeft:
    {
        RegGPR dst  = RegGPRResult();
        RegGPR src1 = RegGPROperand1();

        if (dst != src1) actx.mov(dst, src1);
        actx.shl(dst, m_shift);
    }
    break;

case otXmmAssign:
case otXmmDWordsAssign:
    {
        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }
    }
    break;

case otXmmAssignMember:
    {
        RegGPR src1 = RegGPROperand1();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rm, m_rResult, memptr(src1, m_uDisplacement));
        }
    }
    break;

case otXmmGetLowDWord:
    {
        actx.cmd(movd_xmm_rx, m_rResult, m_rOperand1);
    }
    break;

case otXmmDWordsGetElement:
    {
        UINT32 uOffset = actx.GetOffset(m_vOperand1);

        actx.cmd(mov_rm, m_rResult, actx.FramePtr(uOffset + m_bImmediateByte*4));
    }
    break;

case otXmmIntLoad64:
    {
        RegGPR ptr = RegGPROperand1();

        actx.cmd(movq_xmm_rm, m_rResult, memptr(ptr, 0));
    }
    break;

case otXmmIntStore64:
    {
        RegGPR ptr = RegGPROperand1();

        actx.cmd(movq_xmm_mr, memptr(ptr, 0), m_rOperand2);
    }
    break;

case otXmmIntTest:
    {
        RegXMM src1 = RegXMMOperand1();
        RegXMM src2 = RegXMMOperand2();
        actx.ptest(src1, src2);
    }
    break;

case otXmmLoadLowQWords:
    {
        if (m_rResult != m_rOperand1)
            actx.cmd(movdqa_rm, m_rResult, m_rOperand1);

        if (m_rOperand2.IsDefined())
        {
            actx.cmd(punpcklqdq, m_rResult, m_rOperand2);
        }
        else
        {
            UINT32 offset = actx.GetOffset(m_vOperand2);
            actx.cmd(punpcklqdq, m_rResult, actx.FramePtr(offset));
        }
    }
    break;

case otXmmSetZero:
    {
        RegXMM dst = RegXMMResult();
        actx.cmd(pxor, dst, dst);
    }
    break;

case otXmmWordsShiftRight:
    {
        RegXMM dst = RegXMMResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }

        actx.psrlw(dst, m_shift);
    }
    break;

case otXmmWordsSignedShiftRight:
    {
        RegXMM dst = RegXMMResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }

        actx.psraw(dst, m_shift);
    }
    break;

case otXmmWordsShiftLeft:
    {
        RegXMM dst = RegXMMResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }

        actx.psllw(dst, m_shift);
    }
    break;

case otXmmDWordsShiftRight:
    {
        RegXMM dst  = RegXMMResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }

        actx.psrld(dst, m_shift);
    }
    break;

case otXmmDWordsSignedShiftRight:
    {
        RegXMM dst  = RegXMMResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }

        actx.psrad(dst, m_shift);
    }
    break;

case otXmmDWordsShiftLeft:
    {
        RegXMM dst  = RegXMMResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }

        if (m_vOperand2)
        {
            RegXMM src2 = RegXMMOperand2();
            actx.pslld(dst, src2);
        }
        else
        {
            actx.pslld(dst, m_shift);
        }
    }
    break;

case otXmmDWordsShiftRight32:
    {
        RegXMM dst  = RegXMMResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movdqa_rr, m_rResult, m_rOperand1);
        }

        actx.psrldq(dst, 4);
    }
    break;

case otXmmFloat1Assign:
    {
        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movss_rr, m_rResult, m_rOperand1);
        }
    }
    break;

case otXmmFloat1LoadInt:
    {
        if (m_vOperand1)
        {
            if (m_refType == RefType_Base)
            {
                RegGPR pBase = RegGPROperand1();
                actx.cmd(cvtsi2ss, m_rResult, memptr(pBase, m_uDisplacement));
            }
            else
            {
                RegGPR index = RegGPROperand1();
                actx.cmd(cvtsi2ss, m_rResult, memptr(m_pData, index, (Scale32)m_refType));
            }
        }
        else
        {
            actx.cmd(cvtsi2ss, m_rResult, memptr(m_pData));
        }
    }
    break;

case otXmmFloat1FromInt:
    {
        if (m_rOperand1.IsDefined())
        {
            actx.cmd(cvtsi2ss, m_rResult, m_rOperand1);
        }
        else
        {
            UINT32 offset = actx.GetOffset(m_vOperand1);
            actx.cmd(cvtsi2ss, m_rResult, actx.FramePtr(offset));
        }
    }
    break;

case otXmmFloat4Assign:
    {
        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movaps_rr, m_rResult, m_rOperand1);
        }
    }
    break;

case otXmmFloat4LoadUnaligned:
    {
        RegGPR  ptr = RegGPROperand1();
        RegXMM dst = RegXMMResult();

        actx.movups(dst, xmmword(ptr, m_nOffset));
    }
    break;

case otXmmFloat4StoreUnaligned:
    {
        RegGPR  ptr = RegGPROperand1();
        RegXMM src = RegXMMOperand2();

        actx.movups(xmmword(ptr, m_nOffset), src);
    }
    break;

case otXmmFloat4ExtractSignBits:
    {
        RegGPR  dst = RegGPRResult();
        RegXMM src = RegXMMOperand1();

        actx.movmskps(dst, src);
    }
    break;

#if WPFGFX_FXJIT_X86

case otMmAssign:
    {
        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movq_mmx_rr, m_rResult, m_rOperand1);
        }
    }
    break;

case otMmStore:
    {
        RegGPR ptr = RegGPROperand1();

        actx.cmd(movq_mmx_mr, mmxword(ptr, m_nOffset), m_rOperand2);
    }
    break;

case otMmStoreNonTemporal:
    {
        RegGPR ptr = RegGPROperand1();
        RegMMX src = RegMMXOperand2();

        actx.movntq(mmxword(ptr, m_nOffset), src);
    }
    break;

case otMmQWordToXmm:
    {
        RegXMM dst  = RegXMMResult();
        RegMMX src1 = RegMMXOperand1();
        actx.movq2dq(dst, src1);
    }
    break;

case otMmDWordsShiftRight:
    {
        RegMMX dst  = RegMMXResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movq_mmx_rr, m_rResult, m_rOperand1);
        }

        actx.psrld(dst, m_shift);
    }
    break;

case otMmDWordsSignedShiftRight:
    {
        RegMMX dst  = RegMMXResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movq_mmx_rr, m_rResult, m_rOperand1);
        }

        actx.psrad(dst, m_shift);
    }
    break;

case otMmDWordsShiftLeft:
    {
        RegMMX dst = RegMMXResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movq_mmx_rr, m_rResult, m_rOperand1);
        }

        if (m_vOperand2)
        {
            RegMMX src2 = RegMMXOperand2();
            actx.pslld(dst, src2);
        }
        else
        {
            actx.pslld(dst, m_shift);
        }
    }
    break;

case otMmWordsShiftRight:
    {
        RegMMX dst = RegMMXResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movq_mmx_rr, m_rResult, m_rOperand1);
        }

        actx.psrlw(dst, m_shift);
    }
    break;

case otMmWordsShiftLeft:
    {
        RegMMX dst  = RegMMXResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(movq_mmx_rr, m_rResult, m_rOperand1);
        }

        if (m_vOperand2)
        {
            RegMMX src2 = RegMMXOperand2();
            actx.psllw(dst, src2);
        }
        else
        {
            actx.psllw(dst, m_shift);
        }
    }
    break;

case otXmmConvertToMm:
    {
        RegMMX dst = RegMMXResult();
        RegXMM src = RegXMMOperand1();
        actx.movdq2q(dst, src);
    }
    break;

#endif //WPFGFX_FXJIT_X86

#if WPFGFX_FXJIT_X86
#else // _AMD64_
case otUINT64ImmShiftRight:
    {
        RegGPR dst = RegGPRResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(mov_64_rr, m_rResult, m_rOperand1);
        }

        actx.shrWhole(dst, m_shift);
    }
    break;


case otUINT64ImmShiftLeft:
    {
        RegGPR dst  = RegGPRResult();

        if (m_rResult != m_rOperand1)
        {
            actx.cmd(mov_64_rr, m_rResult, m_rOperand1);
        }

        actx.shlWhole(dst, m_shift);
    }
    break;
#endif //_AMD64_


// NOPs
case otLoadFramePointer:
case otLoopStart:
case otBranchMerge:
case otSubroutineStart:

    break;

default:
    NO_DEFAULT;

}
}

// Instruction codes to move data from memory to register
const UINT32
COperator::sc_movCodesRM[] =
{
    0,              // ofDataNone
    mov_rm,         // ofDataR32
#if WPFGFX_FXJIT_X86
    movd_mmx_rm,    // ofDataM32
    movq_mmx_rm,    // ofDataM64
#else
    0,              // ofDataM32 not supported
    0,              // ofDataM64 not supported
#endif
    movd_xmm_rm,    // ofDataI32
    movq_xmm_rm,    // ofDataI64
    movdqa_rm,      // ofDataI128
    movss_rm,       // ofDataF32
    movaps_rm,      // ofDataF128
#if WPFGFX_FXJIT_X86
    0               // ofDataR64 not supported
#else // _AMD64_
    mov_64_rm,      // ofDataR64
#endif
};

