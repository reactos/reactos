// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of COperator::sc_opCodes table.
//      Contains CPU instruction codes for all the operators.
//
//-----------------------------------------------------------------------------
#include "precomp.h"

#define OpcodeNone                       0
#define OpcodeLoadFramePointer           0
#define OpcodeLoopStart                  0
#define OpcodeLoopRepeatIfNonZero        0
#define OpcodeBranchOnZero               0
#define OpcodeBranchMerge                0
#define OpcodeCall                       0
#define OpcodeReturn                     0
#define OpcodeSubroutineStart            0
#define OpcodeSubroutineCall             0
#define OpcodeSubroutineReturn           0
#define OpcodePtrAssignArgument          0
#define OpcodePtrAssignMember            0
#define OpcodePtrAssignMemberIndexed     0
#define OpcodePtrAssignImm               0
#define OpcodePtrAssign                  0
#define OpcodePtrCompute                 0

#define OpcodeUINT32Increment            0
#define OpcodeUINT32Decrement            0
#define OpcodeUINT32DecrementTest        0
#define OpcodeUINT32Test                 0

#define OpcodeUINT32Assign               0
#define OpcodeUINT32Load                 mov_rm
#define OpcodeUINT32LoadWord             movzx_rm16
#define OpcodeUINT32LoadByte             movzx_rm8
#define OpcodeUINT32Store                mov_mr

#define OpcodeUINT32Add                  add
#define OpcodeUINT32Or                   or
#define OpcodeUINT32And                  and
#define OpcodeUINT32Sub                  sub
#define OpcodeUINT32Xor                  xor
#define OpcodeUINT32Cmp                  cmp
#define OpcodeUINT32Mul                  imul
#define OpcodeUINT32Div                  0
#define OpcodeUINT32Rem                  0

#define OpcodeUINT32ImmAssign            0
#define OpcodeUINT32ImmAdd               0
#define OpcodeUINT32ImmOr                0
#define OpcodeUINT32ImmAnd               0
#define OpcodeUINT32ImmSub               0
#define OpcodeUINT32ImmXor               0
#define OpcodeUINT32ImmCmp               0
#define OpcodeUINT32ImmMul               0

#define OpcodeUINT32ImmShiftRight        0
#define OpcodeUINT32ImmShiftLeft         0
#define OpcodeUINT32ShiftLeft            0
#define OpcodeUINT32ShiftRight           0
#define OpcodeUINT32StoreNonTemporal     movnti_mr

#define OpcodeINT32Div                   0
#define OpcodeINT32Rem                   0
#define OpcodeINT32ShiftRight            0

#define OpcodeXmmAssign                  0
#define OpcodeXmmAssignMember            0
#define OpcodeXmmGetLowDWord             0
#define OpcodeXmmLoadLowQWords           0
#define OpcodeXmmLoadDWord               movd_xmm_rm
#define OpcodeXmmSetZero                 0
#define OpcodeXmmStoreNonTemporal        movntdq
#define OpcodeXmmStoreNonTemporalMasked  0

#define OpcodeXmmBytesAdd                paddb
#define OpcodeXmmBytesSub                psubb
#define OpcodeXmmBytesEqual              pcmpeqb
#define OpcodeXmmBytesInterleaveLow      punpcklbw
#define OpcodeXmmBytesInterleaveHigh     punpckhbw
#define OpcodeXmmBytesUnpackToWords      pmovzxbw   // SSE4.1
#define OpcodeXmmBytesBlend              pblendvb   // SSE4.1

#define OpcodeXmmWordsAdd                paddw
#define OpcodeXmmWordsAddSat             paddusw
#define OpcodeXmmWordsSub                psubw
#define OpcodeXmmWordsSubSat             psubusw
#define OpcodeXmmWordsEqual              pcmpeqw
#define OpcodeXmmWordsInterleaveLow      punpcklwd
#define OpcodeXmmWordsInterleaveHigh     punpckhwd
#define OpcodeXmmWordsPackSS             packsswb
#define OpcodeXmmWordsPackUS             packuswb
#define OpcodeXmmWordsMulAdd             pmaddwd
#define OpcodeXmmWordsSignedMin          pminsw
#define OpcodeXmmWordsSignedMax          pmaxsw
#define OpcodeXmmWordsSignedShiftRight   0
#define OpcodeXmmWordsShiftRight         0
#define OpcodeXmmWordsSignedShiftRight   0
#define OpcodeXmmWordsShiftLeft          0
#define OpcodeXmmWordsMul                pmullw
#define OpcodeXmmWordsShuffleLow         pshuflw
#define OpcodeXmmWordsShuffleHigh        pshufhw
#define OpcodeXmmWordsUnpackToDWords     pmovzxwd   // SSE4.1

#define OpcodeXmmDWordsAssign            0
#define OpcodeXmmDWordsAdd               paddd
#define OpcodeXmmDWordsSub               psubd
#define OpcodeXmmDWordsUnsignedMul       pmuludq
#define OpcodeXmmDWordsSignedMul         pmuldq     // SSE4.1

#define OpcodeXmmDWordsSignedMin         pminsd     // SSE4.1
#define OpcodeXmmDWordsSignedMax         pmaxsd     // SSE4.1
#define OpcodeXmmDWordsUnsignedMin       pminud     // SSE4.1
#define OpcodeXmmDWordsUnsignedMax       pmaxud     // SSE4.1

#define OpcodeXmmDWordsInterleaveLow     punpckldq
#define OpcodeXmmDWordsInterleaveHigh    punpckhdq
#define OpcodeXmmDWordsPackSS            packssdw
#define OpcodeXmmDWordsGreater           pcmpgtd
#define OpcodeXmmDWordsEqual             pcmpeqd
#define OpcodeXmmDWordsSignedShiftRight  0
#define OpcodeXmmDWordsShiftRight        0
#define OpcodeXmmDWordsShiftLeft         0
#define OpcodeXmmDWordsShiftRight32      0
#define OpcodeXmmDWordsToFloat4          cvtdq2ps
#define OpcodeXmmDWordsShuffle           pshufd
#define OpcodeXmmDWordsGetElement        0
#define OpcodeXmmDWordsExtractElement    pextrd     // SSE4.1
#define OpcodeXmmDWordsInsertElement     pinsrd     // SSE4.1

#define OpcodeXmmQWordsAdd               paddq
#define OpcodeXmmQWordsSub               psubq
#define OpcodeXmmQWordsInterleaveLow     punpcklqdq
#define OpcodeXmmQWordsInterleaveHigh    punpckhqdq

#define OpcodeXmmIntLoad64               0
#define OpcodeXmmIntStore64              0
#define OpcodeXmmIntLoad                 movdqa_rm
#define OpcodeXmmIntStore                movdqa_mr
#define OpcodeXmmIntAnd                  pand
#define OpcodeXmmIntOr                   por
#define OpcodeXmmIntXor                  pxor
#define OpcodeXmmIntMul                  pmulld     // SSE4.1
#define OpcodeXmmIntNot                  pxor
#define OpcodeXmmIntAndNot               pandn
#define OpcodeXmmIntTest                 0

#define OpcodeXmmFloat1Assign            0
#define OpcodeXmmFloat1Load              movss_rm
#define OpcodeXmmFloat1LoadInt           0
#define OpcodeXmmFloat1Store             movss_mr
#define OpcodeXmmFloat1FromInt           0

#define OpcodeXmmFloat1Add               addss
#define OpcodeXmmFloat1Sub               subss
#define OpcodeXmmFloat1Mul               mulss
#define OpcodeXmmFloat1Div               divss
#define OpcodeXmmFloat1Min               maxss
#define OpcodeXmmFloat1Max               minss
#define OpcodeXmmFloat1Interleave        unpcklps
#define OpcodeXmmFloat1Reciprocal        rcpss
#define OpcodeXmmFloat1Sqrt              sqrtss
#define OpcodeXmmFloat1Rsqrt             rsqrtss

#define OpcodeXmmFloat4Assign            0
#define OpcodeXmmFloat4Load              movaps_rm
#define OpcodeXmmFloat4Store             movaps_mr
#define OpcodeXmmFloat4Add               addps
#define OpcodeXmmFloat4Sub               subps
#define OpcodeXmmFloat4Mul               mulps
#define OpcodeXmmFloat4Div               divps
#define OpcodeXmmFloat4Max               maxps
#define OpcodeXmmFloat4Min               minps
#define OpcodeXmmFloat4OrderedMax        maxps
#define OpcodeXmmFloat4OrderedMin        minps
#define OpcodeXmmFloat4And               andps
#define OpcodeXmmFloat4AndNot            andnps
#define OpcodeXmmFloat4Or                orps
#define OpcodeXmmFloat4Xor               xorps
#define OpcodeXmmFloat4Not               xorps
#define OpcodeXmmFloat4UnpackHigh        unpckhps
#define OpcodeXmmFloat4UnpackLow         unpcklps
#define OpcodeXmmFloat4Shuffle           shufps
#define OpcodeXmmFloat4Reciprocal        rcpps
#define OpcodeXmmFloat4Sqrt              sqrtps
#define OpcodeXmmFloat4Rsqrt             rsqrtps
#define OpcodeXmmFloat4ToInt32x4         cvtps2dq
#define OpcodeXmmFloat4Truncate          cvttps2dq
#define OpcodeXmmFloat4CmpEQ             cmpps
#define OpcodeXmmFloat4CmpLT             cmpps
#define OpcodeXmmFloat4CmpLE             cmpps
#define OpcodeXmmFloat4CmpNEQ            cmpps
#define OpcodeXmmFloat4CmpNLT            cmpps
#define OpcodeXmmFloat4CmpNLE            cmpps
#define OpcodeXmmFloat4Floor             roundps   // SSE4.1
#define OpcodeXmmFloat4Ceil              roundps   // SSE4.1
#define OpcodeXmmFloat4LoadUnaligned     0
#define OpcodeXmmFloat4StoreUnaligned    0
#define OpcodeXmmFloat4StoreNonTemporal  movntps
#define OpcodeXmmFloat4ExtractSignBits   0

#if WPFGFX_FXJIT_X86
#define OpcodeMmAssign                   0
#define OpcodeMmLoad                     movq_mmx_rm
#define OpcodeMmLoadDWord                movd_mmx_rm
#define OpcodeMmStore                    0
#define OpcodeMmStoreNonTemporal         0

#define OpcodeMmBytesAdd                 paddb_mmx
#define OpcodeMmBytesSub                 psubb_mmx
#define OpcodeMmBytesEqual               pcmpeqb_mmx
#define OpcodeMmBytesInterleaveLow       punpcklbw_mmx
#define OpcodeMmBytesInterleaveHigh      punpckhbw_mmx

#define OpcodeMmWordsAdd                 paddw_mmx
#define OpcodeMmWordsAddSat              paddusw_mmx
#define OpcodeMmWordsSub                 psubw_mmx
#define OpcodeMmWordsSubSat              psubusw_mmx
#define OpcodeMmWordsEqual               pcmpeqw_mmx
#define OpcodeMmWordsMul                 pmullw_mmx
#define OpcodeMmWordsInterleaveLow       punpcklwd_mmx
#define OpcodeMmWordsInterleaveHigh      punpckhwd_mmx
#define OpcodeMmWordsPackSS              packsswb_mmx
#define OpcodeMmWordsPackUS              packuswb_mmx
#define OpcodeMmWordsMulAdd              pmaddwd_mmx
#define OpcodeMmWordsShiftRight          0
#define OpcodeMmWordsShiftLeft           0

#define OpcodeMmDWordsAdd                paddd_mmx
#define OpcodeMmDWordsSub                psubd_mmx
#define OpcodeMmDWordsEqual              pcmpeqd_mmx
#define OpcodeMmDWordsGreater            pcmpgtd_mmx
#define OpcodeMmDWordsInterleaveLow      punpckldq_mmx
#define OpcodeMmDWordsInterleaveHigh     punpckhdq_mmx
#define OpcodeMmDWordsPackSS             packssdw_mmx
#define OpcodeMmDWordsShiftRight         0
#define OpcodeMmDWordsShiftLeft          0
#define OpcodeMmDWordsSignedShiftRight   0

#define OpcodeMmQWordAdd                 paddq_mmx
#define OpcodeMmQWordSub                 psubq_mmx
#define OpcodeMmQWordAnd                 pand_mmx
#define OpcodeMmQWordOr                  por_mmx
#define OpcodeMmQWordXor                 pxor_mmx
#define OpcodeMmQWordNot                 pxor_mmx
#define OpcodeMmQWordAndNot              pandn_mmx
#define OpcodeMmQWordToXmm               0

#define OpcodeXmmConvertToMm             0

#else _AMD64_

#define OpcodeUINT64Assign               0
#define OpcodeUINT64Assign32             0
#define OpcodeUINT32Assign64             0
#define OpcodeUINT64Add                  add
#define OpcodeUINT64Or                   or
#define OpcodeUINT64And                  and
#define OpcodeUINT64Sub                  sub
#define OpcodeUINT64Xor                  xor
#define OpcodeUINT64Mul                  imul

#define OpcodeUINT64ImmAssign            0
#define OpcodeUINT64ImmShiftRight        0
#define OpcodeUINT64ImmShiftLeft         0

#endif //_AMD64_

const UINT32
COperator::sc_opCodes[] =
{
#define DEFINE_OPCODE(name) Opcode##name,
    OPERATIONS(DEFINE_OPCODE)
};

