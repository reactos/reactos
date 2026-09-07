// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of enum OpType.
//
//-----------------------------------------------------------------------------
#pragma once

#define COMMON_OPERATIONS(m)      \
    m(None                       )\
    m(LoadFramePointer           )\
    m(LoopStart                  )\
    m(LoopRepeatIfNonZero        )\
    m(BranchOnZero               )\
    m(BranchMerge                )\
    m(Call                       )\
    m(Return                     )\
    m(SubroutineStart            )\
    m(SubroutineCall             )\
    m(SubroutineReturn           )\
    m(PtrAssignArgument          )\
    m(PtrAssignMember            )\
    m(PtrAssignMemberIndexed     )\
    m(PtrAssignImm               )\
    m(PtrAssign                  )\
    m(PtrCompute                 )\
    m(UINT32Increment            )\
    m(UINT32Decrement            )\
    m(UINT32DecrementTest        )\
    m(UINT32Test                 )\
    m(UINT32Assign               )\
    m(UINT32Load                 )\
    m(UINT32LoadWord             )\
    m(UINT32LoadByte             )\
    m(UINT32Store                )\
    m(UINT32Add                  )\
    m(UINT32Or                   )\
    m(UINT32And                  )\
    m(UINT32Sub                  )\
    m(UINT32Xor                  )\
    m(UINT32Cmp                  )\
    m(UINT32Mul                  )\
    m(UINT32Div                  )\
    m(UINT32Rem                  )\
    m(UINT32ImmAssign            )\
    m(UINT32ImmAdd               )\
    m(UINT32ImmOr                )\
    m(UINT32ImmAnd               )\
    m(UINT32ImmSub               )\
    m(UINT32ImmXor               )\
    m(UINT32ImmCmp               )\
    m(UINT32ImmMul               )\
    m(UINT32ImmShiftRight        )\
    m(UINT32ImmShiftLeft         )\
    m(UINT32ShiftLeft            )\
    m(UINT32ShiftRight           )\
    m(UINT32StoreNonTemporal     )\
    m(INT32Div                   )\
    m(INT32Rem                   )\
    m(INT32ShiftRight            )\
    m(XmmAssign                  )\
    m(XmmAssignMember            )\
    m(XmmGetLowDWord             )\
    m(XmmLoadLowQWords           )\
    m(XmmLoadDWord               )\
    m(XmmSetZero                 )\
    m(XmmStoreNonTemporal        )\
    m(XmmStoreNonTemporalMasked  )\
    m(XmmBytesAdd                )\
    m(XmmBytesSub                )\
    m(XmmBytesEqual              )\
    m(XmmBytesInterleaveLow      )\
    m(XmmBytesInterleaveHigh     )\
    m(XmmBytesUnpackToWords      )\
    m(XmmBytesBlend              )\
    m(XmmWordsShiftRight         )\
    m(XmmWordsShiftLeft          )\
    m(XmmWordsAdd                )\
    m(XmmWordsAddSat             )\
    m(XmmWordsSub                )\
    m(XmmWordsEqual              )\
    m(XmmWordsSubSat             )\
    m(XmmWordsSignedMin          )\
    m(XmmWordsSignedMax          )\
    m(XmmWordsInterleaveLow      )\
    m(XmmWordsInterleaveHigh     )\
    m(XmmWordsPackSS             )\
    m(XmmWordsPackUS             )\
    m(XmmWordsMulAdd             )\
    m(XmmWordsMul                )\
    m(XmmWordsSignedShiftRight   )\
    m(XmmWordsShuffleLow         )\
    m(XmmWordsShuffleHigh        )\
    m(XmmWordsUnpackToDWords     )\
    m(XmmDWordsAssign            )\
    m(XmmDWordsAdd               )\
    m(XmmDWordsSub               )\
    m(XmmDWordsUnsignedMul       )\
    m(XmmDWordsSignedMul         )\
    m(XmmDWordsSignedMin         )\
    m(XmmDWordsSignedMax         )\
    m(XmmDWordsUnsignedMin       )\
    m(XmmDWordsUnsignedMax       )\
    m(XmmDWordsInterleaveLow     )\
    m(XmmDWordsInterleaveHigh    )\
    m(XmmDWordsPackSS            )\
    m(XmmDWordsGreater           )\
    m(XmmDWordsEqual             )\
    m(XmmDWordsShiftRight        )\
    m(XmmDWordsSignedShiftRight  )\
    m(XmmDWordsShiftLeft         )\
    m(XmmDWordsShiftRight32      )\
    m(XmmDWordsToFloat4          )\
    m(XmmDWordsShuffle           )\
    m(XmmDWordsGetElement        )\
    m(XmmDWordsExtractElement    )\
    m(XmmDWordsInsertElement     )\
    m(XmmQWordsAdd               )\
    m(XmmQWordsSub               )\
    m(XmmQWordsInterleaveLow     )\
    m(XmmQWordsInterleaveHigh    )\
    m(XmmIntLoad64               )\
    m(XmmIntStore64              )\
    m(XmmIntLoad                 )\
    m(XmmIntStore                )\
    m(XmmIntAnd                  )\
    m(XmmIntOr                   )\
    m(XmmIntXor                  )\
    m(XmmIntMul                  )\
    m(XmmIntNot                  )\
    m(XmmIntAndNot               )\
    m(XmmIntTest                 )\
    m(XmmFloat1Assign            )\
    m(XmmFloat1Load              )\
    m(XmmFloat1LoadInt           )\
    m(XmmFloat1Store             )\
    m(XmmFloat1FromInt           )\
    m(XmmFloat1Add               )\
    m(XmmFloat1Sub               )\
    m(XmmFloat1Mul               )\
    m(XmmFloat1Div               )\
    m(XmmFloat1Max               )\
    m(XmmFloat1Min               )\
    m(XmmFloat1Interleave        )\
    m(XmmFloat1Reciprocal        )\
    m(XmmFloat1Sqrt              )\
    m(XmmFloat1Rsqrt             )\
    m(XmmFloat4Assign            )\
    m(XmmFloat4Load              )\
    m(XmmFloat4Store             )\
    m(XmmFloat4Add               )\
    m(XmmFloat4Sub               )\
    m(XmmFloat4Mul               )\
    m(XmmFloat4Div               )\
    m(XmmFloat4Max               )\
    m(XmmFloat4Min               )\
    m(XmmFloat4OrderedMax        )\
    m(XmmFloat4OrderedMin        )\
    m(XmmFloat4And               )\
    m(XmmFloat4AndNot            )\
    m(XmmFloat4Or                )\
    m(XmmFloat4Xor               )\
    m(XmmFloat4Not               )\
    m(XmmFloat4UnpackHigh        )\
    m(XmmFloat4UnpackLow         )\
    m(XmmFloat4Shuffle           )\
    m(XmmFloat4Reciprocal        )\
    m(XmmFloat4Sqrt              )\
    m(XmmFloat4Rsqrt             )\
    m(XmmFloat4ToInt32x4         )\
    m(XmmFloat4Truncate          )\
    m(XmmFloat4CmpEQ             )\
    m(XmmFloat4CmpLT             )\
    m(XmmFloat4CmpLE             )\
    m(XmmFloat4CmpNEQ            )\
    m(XmmFloat4CmpNLT            )\
    m(XmmFloat4CmpNLE            )\
    m(XmmFloat4Floor             )\
    m(XmmFloat4Ceil              )\
    m(XmmFloat4LoadUnaligned     )\
    m(XmmFloat4StoreUnaligned    )\
    m(XmmFloat4StoreNonTemporal  )\
    m(XmmFloat4ExtractSignBits   )\

#if WPFGFX_FXJIT_X86
#define CPU_SPECIFIC_OPERATIONS(m)\
    m(MmAssign                   )\
    m(MmLoad                     )\
    m(MmLoadDWord                )\
    m(MmStore                    )\
    m(MmStoreNonTemporal         )\
    m(MmBytesAdd                 )\
    m(MmBytesSub                 )\
    m(MmBytesEqual               )\
    m(MmBytesInterleaveLow       )\
    m(MmBytesInterleaveHigh      )\
    m(MmWordsAdd                 )\
    m(MmWordsAddSat              )\
    m(MmWordsSub                 )\
    m(MmWordsSubSat              )\
    m(MmWordsEqual               )\
    m(MmWordsMul                 )\
    m(MmWordsMulAdd              )\
    m(MmWordsShiftRight          )\
    m(MmWordsShiftLeft           )\
    m(MmWordsInterleaveLow       )\
    m(MmWordsInterleaveHigh      )\
    m(MmWordsPackSS              )\
    m(MmWordsPackUS              )\
    m(MmDWordsAdd                )\
    m(MmDWordsSub                )\
    m(MmDWordsEqual              )\
    m(MmDWordsGreater            )\
    m(MmDWordsInterleaveLow      )\
    m(MmDWordsInterleaveHigh     )\
    m(MmDWordsPackSS             )\
    m(MmDWordsShiftRight         )\
    m(MmDWordsSignedShiftRight   )\
    m(MmDWordsShiftLeft          )\
    m(MmQWordAdd                 )\
    m(MmQWordSub                 )\
    m(MmQWordAnd                 )\
    m(MmQWordOr                  )\
    m(MmQWordXor                 )\
    m(MmQWordNot                 )\
    m(MmQWordToXmm               )\
    m(XmmConvertToMm             )\


#else //_AMD64_
#define CPU_SPECIFIC_OPERATIONS(m)\
    m(UINT64Assign               )\
    m(UINT64Assign32             )\
    m(UINT32Assign64             )\
    m(UINT64Add                  )\
    m(UINT64Or                   )\
    m(UINT64And                  )\
    m(UINT64Sub                  )\
    m(UINT64Xor                  )\
    m(UINT64Mul                  )\
    m(UINT64ImmAssign            )\
    m(UINT64ImmShiftRight        )\
    m(UINT64ImmShiftLeft         )\

#endif


#define OPERATIONS(m) COMMON_OPERATIONS(m) CPU_SPECIFIC_OPERATIONS(m)

enum OpType : UINT8
{
#define DEFINE_OPTYPE(name) ot##name,
    OPERATIONS(DEFINE_OPTYPE)
};

