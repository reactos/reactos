// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Implementation of COperator::sc_opFlags table.
//      Contains feature flags for all the operators.
//
//-----------------------------------------------------------------------------
#include "precomp.h"


#define FlagsNone                       ofDataNone
#define FlagsLoadFramePointer           ofDataR32
#define FlagsLoopStart                  ofDataNone | ofIsControl | ofNoBubble
#define FlagsLoopRepeatIfNonZero        ofDataNone | ofIsControl | ofNoBubble | ofIsLoopRepeat | ofConsumesZF
#define FlagsBranchOnZero               ofDataNone | ofIsControl | ofNoBubble | ofIsBranchSplit | ofConsumesZF
#define FlagsBranchMerge                ofDataNone | ofIsControl | ofNoBubble
#define FlagsCall                       ofDataNone | ofChangesZF | ofHasOutsideEffect | ofHasOutsideDependency | ofNoBubble | ofIrregular
#define FlagsReturn                     ofDataR32  | ofIsControl | ofNoBubble
#define FlagsSubroutineStart            ofDataNone | ofNoBubble | ofHasOutsideEffect // ofHasOutsideEffect to avoid deleting on optimization
#define FlagsSubroutineCall             ofDataNone | ofIsControl | ofNoBubble
#define FlagsSubroutineReturn           ofDataNone | ofIsControl | ofNoBubble
#define FlagsPtrAssignArgument          ofDataR32
#define FlagsPtrAssignMember            ofDataR32 | ofHasOutsideDependency
#define FlagsPtrAssignMemberIndexed     ofDataR32 | ofHasOutsideDependency
#define FlagsPtrAssignImm               ofDataR32
#define FlagsPtrAssign                  ofDataR32
#define FlagsPtrCompute                 ofDataR32
#define FlagsUINT32Load                 ofDataR32  | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsUINT32LoadWord             ofDataR32  | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsUINT32LoadByte             ofDataR32  | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsUINT32Increment            ofDataR32
#define FlagsUINT32Decrement            ofDataR32
#define FlagsUINT32DecrementTest        ofDataR32  | ofChangesZF | ofCalculatesZF
#define FlagsUINT32Test                 ofDataR32  | ofChangesZF | ofCalculatesZF

#define FlagsUINT32Assign               ofDataR32

#define FlagsUINT32Add                  ofDataR32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF
#define FlagsUINT32Or                   ofDataR32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT32And                  ofDataR32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT32Sub                  ofDataR32  | ofCanTakeOperand2FromMemory                     | ofChangesZF | ofStandardBinary
#define FlagsUINT32Xor                  ofDataR32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT32Cmp                  ofDataR32  | ofCanTakeOperand2FromMemory                     | ofChangesZF | ofStandardBinary
#define FlagsUINT32Mul                  ofDataR32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT32Div                  ofDataR32  | ofCanTakeOperand2FromMemory                     | ofChangesZF | ofStandardBinary | ofIrregular | ofNoBubble
#define FlagsUINT32Rem                  ofDataR32  | ofCanTakeOperand2FromMemory                     | ofChangesZF | ofStandardBinary | ofIrregular | ofNoBubble

#define FlagsUINT32ImmAssign            ofDataR32  | ofChangesZF
#define FlagsUINT32ImmAdd               ofDataR32
#define FlagsUINT32ImmOr                ofDataR32  | ofChangesZF
#define FlagsUINT32ImmAnd               ofDataR32  | ofChangesZF
#define FlagsUINT32ImmSub               ofDataR32
#define FlagsUINT32ImmXor               ofDataR32  | ofChangesZF
#define FlagsUINT32ImmCmp               ofDataR32  | ofChangesZF | ofCalculatesZF
#define FlagsUINT32ImmMul               ofDataR32  | ofChangesZF

#define FlagsUINT32ImmShiftRight        ofDataR32  | ofChangesZF
#define FlagsUINT32ImmShiftLeft         ofDataR32  | ofChangesZF
#define FlagsUINT32ShiftLeft            ofDataR32  | ofChangesZF | ofIrregular
#define FlagsUINT32ShiftRight           ofDataR32  | ofChangesZF | ofIrregular
#define FlagsUINT32Store                ofDataR32  | ofHasOutsideEffect | ofStandardMemDst
#define FlagsUINT32StoreNonTemporal     ofDataR32  | ofHasOutsideEffect | ofStandardMemDst

#define FlagsINT32Div                   ofDataR32  | ofCanTakeOperand2FromMemory | ofChangesZF | ofStandardBinary | ofIrregular | ofNoBubble
#define FlagsINT32Rem                   ofDataR32  | ofCanTakeOperand2FromMemory | ofChangesZF | ofStandardBinary | ofIrregular | ofNoBubble
#define FlagsINT32ShiftRight            ofDataR32  | ofChangesZF | ofIrregular

#define FlagsXmmAssign                  ofDataI128
#define FlagsXmmAssignMember            ofDataI128
#define FlagsXmmGetLowDWord             ofDataNone // I128->I32
#define FlagsXmmLoadLowQWords           ofDataI128 | ofCanTakeOperand2FromMemory
#define FlagsXmmLoadDWord               ofDataI32  | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmSetZero                 ofDataI128
#define FlagsXmmStoreNonTemporal        ofDataI128 | ofHasOutsideEffect | ofNonTemporalStore | ofStandardMemDst
#define FlagsXmmStoreNonTemporalMasked  ofDataI128 | ofHasOutsideEffect | ofIrregular | ofNonTemporalStore

#define FlagsXmmBytesAdd                ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmBytesSub                ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmBytesEqual              ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmBytesInterleaveLow      ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmBytesInterleaveHigh     ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmBytesUnpackToWords      ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardUnary
#define FlagsXmmBytesBlend              ofDataI128                                                   | ofIrregular

#define FlagsXmmWordsAdd                ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmWordsAddSat             ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmWordsSub                ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmWordsSubSat             ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmWordsEqual              ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmWordsInterleaveLow      ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmWordsInterleaveHigh     ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmWordsPackSS             ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmWordsPackUS             ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmWordsShiftRight         ofDataI128
#define FlagsXmmWordsSignedShiftRight   ofDataI128
#define FlagsXmmWordsShiftLeft          ofDataI128
#define FlagsXmmWordsMul                ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmWordsMulAdd             ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmWordsSignedMin          ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmWordsSignedMax          ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmWordsShuffleLow         ofDataI128 | ofCanTakeOperand1FromMemory | ofHasImmediateByte | ofStandardUnary
#define FlagsXmmWordsShuffleHigh        ofDataI128 | ofCanTakeOperand1FromMemory | ofHasImmediateByte | ofStandardUnary
#define FlagsXmmWordsUnpackToDWords     ofDataI128 | ofCanTakeOperand1FromMemory                     | ofStandardUnary

#define FlagsXmmDWordsAssign            ofDataI128
#define FlagsXmmDWordsAdd               ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmDWordsSub               ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmDWordsUnsignedMul       ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmDWordsSignedMul         ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary

#define FlagsXmmDWordsSignedMin         ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmDWordsSignedMax         ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmDWordsUnsignedMin       ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmDWordsUnsignedMax       ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary

#define FlagsXmmDWordsInterleaveLow     ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmDWordsInterleaveHigh    ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmDWordsPackSS            ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmDWordsGreater           ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmDWordsEqual             ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmDWordsShiftRight        ofDataI128
#define FlagsXmmDWordsSignedShiftRight  ofDataI128
#define FlagsXmmDWordsShiftLeft         ofDataI128
#define FlagsXmmDWordsShiftRight32      ofDataI128
#define FlagsXmmDWordsToFloat4          ofDataI128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmDWordsShuffle           ofDataI128 | ofCanTakeOperand1FromMemory | ofHasImmediateByte | ofStandardUnary
#define FlagsXmmDWordsGetElement        ofDataNone                               | ofHasImmediateByte
#define FlagsXmmDWordsExtractElement    ofDataNone                               | ofHasImmediateByte | ofStandardUnary
#define FlagsXmmDWordsInsertElement     ofDataI128 | ofCanTakeOperand2FromMemory | ofHasImmediateByte | ofStandardBinary

#define FlagsXmmQWordsAdd               ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmQWordsSub               ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmQWordsInterleaveLow     ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmQWordsInterleaveHigh    ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary

#define FlagsXmmIntLoad64               ofDataI64  | ofHasOutsideEffect
#define FlagsXmmIntStore64              ofDataI64  | ofHasOutsideEffect
#define FlagsXmmIntLoad                 ofDataI128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmIntStore                ofDataI128 | ofHasOutsideEffect | ofStandardMemDst
#define FlagsXmmIntAnd                  ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmIntOr                   ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmIntXor                  ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmIntMul                  ofDataI128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmIntNot                  ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmIntAndNot               ofDataI128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmIntTest                 ofDataI128 | ofChangesZF | ofCalculatesZF //SSE4.1

#define FlagsXmmFloat1Assign            ofDataF32
#define FlagsXmmFloat1Load              ofDataF32  | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmFloat1LoadInt           ofDataI32
#define FlagsXmmFloat1Store             ofDataF32  | ofHasOutsideEffect | ofStandardMemDst
#define FlagsXmmFloat1FromInt           ofDataF32
#define FlagsXmmFloat1Add               ofDataF32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat1Sub               ofDataF32  | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsXmmFloat1Mul               ofDataF32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat1Div               ofDataF32  | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsXmmFloat1Min               ofDataF32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat1Max               ofDataF32  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat1Interleave        ofDataF32  | ofCanTakeOperand2FromMemory |                     ofStandardBinary
#define FlagsXmmFloat1Reciprocal        ofDataF32  | ofCanTakeOperand2FromMemory
#define FlagsXmmFloat1Sqrt              ofDataF32  | ofCanTakeOperand2FromMemory
#define FlagsXmmFloat1Rsqrt             ofDataF32  | ofCanTakeOperand2FromMemory

#define FlagsXmmFloat4Assign            ofDataF128
#define FlagsXmmFloat4Load              ofDataF128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmFloat4Store             ofDataF128 | ofHasOutsideEffect | ofStandardMemDst
#define FlagsXmmFloat4Add               ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat4Sub               ofDataF128 | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsXmmFloat4Mul               ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat4Div               ofDataF128 | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsXmmFloat4Max               ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat4Min               ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat4OrderedMax        ofDataF128 | ofCanTakeOperand2FromMemory |                     ofStandardBinary
#define FlagsXmmFloat4OrderedMin        ofDataF128 | ofCanTakeOperand2FromMemory |                     ofStandardBinary
#define FlagsXmmFloat4And               ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat4AndNot            ofDataF128 | ofCanTakeOperand2FromMemory |                     ofStandardBinary
#define FlagsXmmFloat4Or                ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat4Xor               ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsXmmFloat4Not               ofDataF128 | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsXmmFloat4UnpackHigh        ofDataF128 | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsXmmFloat4UnpackLow         ofDataF128 | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsXmmFloat4Shuffle           ofDataF128 | ofCanTakeOperand2FromMemory | ofHasImmediateByte | ofStandardBinary
#define FlagsXmmFloat4Reciprocal        ofDataF128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmFloat4Sqrt              ofDataF128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmFloat4Rsqrt             ofDataF128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmFloat4ToInt32x4         ofDataF128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmFloat4Truncate          ofDataF128 | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsXmmFloat4CmpEQ             ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofHasImmediateByte | ofHasOpcodeSuffix  | ofStandardBinary
#define FlagsXmmFloat4CmpLT             ofDataF128 | ofCanTakeOperand2FromMemory                     | ofHasImmediateByte | ofHasOpcodeSuffix  | ofStandardBinary
#define FlagsXmmFloat4CmpLE             ofDataF128 | ofCanTakeOperand2FromMemory                     | ofHasImmediateByte | ofHasOpcodeSuffix  | ofStandardBinary
#define FlagsXmmFloat4CmpNEQ            ofDataF128 | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofHasImmediateByte | ofHasOpcodeSuffix  | ofStandardBinary
#define FlagsXmmFloat4CmpNLT            ofDataF128 | ofCanTakeOperand2FromMemory                     | ofHasImmediateByte | ofHasOpcodeSuffix  | ofStandardBinary
#define FlagsXmmFloat4CmpNLE            ofDataF128 | ofCanTakeOperand2FromMemory                     | ofHasImmediateByte | ofHasOpcodeSuffix | ofStandardBinary
#define FlagsXmmFloat4Floor             ofDataF128 | ofCanTakeOperand1FromMemory                     | ofHasImmediateByte | ofHasOpcodeSuffix | ofStandardUnary
#define FlagsXmmFloat4Ceil              ofDataF128 | ofCanTakeOperand2FromMemory                     | ofHasImmediateByte | ofHasOpcodeSuffix | ofStandardUnary
#define FlagsXmmFloat4LoadUnaligned     ofDataF128
#define FlagsXmmFloat4StoreUnaligned    ofDataF128 | ofHasOutsideEffect
#define FlagsXmmFloat4StoreNonTemporal  ofDataF128 | ofHasOutsideEffect | ofNonTemporalStore | ofStandardMemDst
#define FlagsXmmFloat4ExtractSignBits   ofDataF128

#if WPFGFX_FXJIT_X86
#define FlagsMmAssign                   ofDataM64  | ofUsesMMX
#define FlagsMmLoad                     ofDataM64  | ofUsesMMX | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsMmLoadDWord                ofDataI32  | ofUsesMMX | ofCanTakeOperand1FromMemory | ofStandardUnary
#define FlagsMmStore                    ofDataM64  | ofUsesMMX | ofHasOutsideEffect
#define FlagsMmStoreNonTemporal         ofDataM64  | ofUsesMMX | ofHasOutsideEffect | ofNonTemporalStore

#define FlagsMmBytesAdd                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmBytesSub                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmBytesEqual               ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmBytesInterleaveLow       ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmBytesInterleaveHigh      ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary

#define FlagsMmWordsAdd                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmWordsAddSat              ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmWordsSub                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsMmWordsSubSat              ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory                     | ofStandardBinary
#define FlagsMmWordsEqual               ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmWordsMul                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmWordsInterleaveLow       ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmWordsInterleaveHigh      ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmWordsPackSS              ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmWordsPackUS              ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmWordsMulAdd              ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmWordsShiftRight          ofDataM64 | ofUsesMMX
#define FlagsMmWordsShiftLeft           ofDataM64 | ofUsesMMX

#define FlagsMmDWordsAdd                ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmDWordsSub                ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmDWordsEqual              ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmDWordsGreater            ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmDWordsInterleaveLow      ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmDWordsInterleaveHigh     ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmDWordsPackSS             ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmDWordsShiftRight         ofDataM64 | ofUsesMMX
#define FlagsMmDWordsSignedShiftRight   ofDataM64 | ofUsesMMX
#define FlagsMmDWordsShiftLeft          ofDataM64 | ofUsesMMX

#define FlagsMmQWordAdd                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmQWordSub                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmQWordAnd                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmQWordOr                  ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmQWordXor                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofStandardBinary
#define FlagsMmQWordNot                 ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory | ofStandardBinary
#define FlagsMmQWordAndNot              ofDataM64 | ofUsesMMX | ofCanTakeOperand2FromMemory
#define FlagsMmQWordToXmm               ofDataM64 | ofUsesMMX

#define FlagsXmmConvertToMm             ofDataI64

#else // _AMD64_

#define FlagsUINT64Assign               ofDataR64
#define FlagsUINT64Assign32             ofDataNone
#define FlagsUINT32Assign64             ofDataNone

#define FlagsUINT64Add                  ofDataR64  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT64Or                   ofDataR64  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT64And                  ofDataR64  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT64Sub                  ofDataR64  | ofCanTakeOperand2FromMemory                     | ofChangesZF | ofStandardBinary
#define FlagsUINT64Xor                  ofDataR64  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary
#define FlagsUINT64Mul                  ofDataR64  | ofCanTakeOperand2FromMemory | ofCanSwapOperands | ofChangesZF | ofStandardBinary

#define FlagsUINT64ImmAssign            ofDataR64  | ofChangesZF
#define FlagsUINT64ImmShiftRight        ofDataR64  | ofChangesZF
#define FlagsUINT64ImmShiftLeft         ofDataR64  | ofChangesZF

#endif //_AMD64_

const UINT32
COperator::sc_opFlags[] =
{
#define DEFINE_OPFLAGS(name) Flags##name,
    OPERATIONS(DEFINE_OPFLAGS)
};

