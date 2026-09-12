//
//  wipe.symcryptasm   Assembler code for wiping a buffer
//  Expresses asm in a generic enough way to enable generation of MASM and GAS using the
//  symcryptasm_processor.py script and C preprocessor
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.


#include "symcryptasm_shared.cppasm"

//VOID
//SYMCRYPT_CALL
//SymCryptWipe( _Out_writes_bytes_( cbData )    PVOID  pbData,
//                                              SIZE_T cbData )

#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q2 rdx
#define D2 edx
#define W2 dx
#define B2 dl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
PLATFORM_NAME_MANGLE(SymCryptWipeAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptWipeAsm)
.def PLATFORM_NAME_MANGLE(SymCryptWipeAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptWipeAsm)


.seh_endprologue


        // Q1 = pbData
        // Q2 = cbData

        //
        // This function will handle any alignment of pbData and any size, but it is optimized for
        // the case where the start and end of the buffer are 16-aligned.
        // 16 is the natural stack alignment on AMD64, and structures can be designed to be a multiple
        // of 16 long without adding too much slack.
        // The cost of non-alignment is relatively low, in the order of 5 cycles or so
        //

        xorps   xmm0,xmm0               // Zero register for 16-byte wipes
        cmp     Q2,16
        jb      SymCryptWipeAsmSmall    // if cbData < 16, this is a rare case

        test    Q1,15
        jnz     SymCryptWipeAsmUnaligned // if data pointer is unaligned, we jump to the code that aligns the pointer
                                        // For well-optimized callers the aligned case is the common one, and that is
                                        // the fall-through.

SymCryptWipeAsmAligned:
        //
        // Here Q1 is aligned, and Q2 contains the # bytes left to wipe, and Q2 >= 16
        //
        // Our loop wipes in 32-byte increments; we always wipe the first 16 bytes
        // and increment the pbData pointer if cbData is 16 mod 32
        // This avoids a conditional jump and is faster.
        //
        test    Q2,16
        movaps  [Q1],xmm0               // it is safe to always wipe as cbData >= 16
        lea     Q3,[Q1+16]
        cmovnz  Q1,Q3                   // only increment pbData if cbData = 16 mod 32

        sub     Q2,32                   // see if we have >= 32 bytes to wipe
        jc      SymCryptWipeAsmTailOptional // if not, wipe tail, or nothing if cbData = 0 mod 16

ALIGN(16)

SymCryptWipeAsmLoop:
        movaps  [Q1],xmm0
        movaps  [Q1+16],xmm0            // Wipe 32 bytes
        add     Q1,32
        sub     Q2,32
        jnc     SymCryptWipeAsmLoop

SymCryptWipeAsmTailOptional:
        // only the lower 4 bits of Q2 are valid, we have subtracted too much already.
        // The wipe was at least 16 bytes, so we can just wipe the tail with 2 instructions

        and     D2,15
        jnz     SymCryptWipeAsmTail
        ret

SymCryptWipeAsmTail:
        // This code appears also below at the end of the unaligned wiping routine
        // but making the jnz jump further is slower and we only duplicate 4 instructions.
        xor     D0,D0
        mov     [Q1+Q2-16],Q0
        mov     [Q1+Q2-8],Q0
        ret

ALIGN(4)

SymCryptWipeAsmUnaligned:

        //
        // At this point we know that cbData(Q2) >= 16 and pbData(Q1) is unaligned.
        // We can wipe 16 bytes and move to an aligned position
        //
        xor     D0,D0
        mov     [Q1],Q0
        mov     [Q1+8],Q0

        mov     D0,D1
        neg     D0                      // lower 4 bits of D0 = # bytes to wipe to reach alignment
        and     D0,15
        add     Q1,Q0
        sub     Q2,Q0

        //
        // If Q2 > 16, go to the aligned wiping loop
        //
        cmp     Q2,16
        jae     SymCryptWipeAsmAligned  // if cbData >= 16, do aligned wipes

        //
        // We have <= 16 bytes to wipe, and we know that the full wipe region was at least 16 bytes.
        // We just wipe the last 16 bytes completely.
        //
        xor     D0,D0
        mov     [Q1+Q2-16],Q0
        mov     [Q1+Q2-8],Q0
        ret

ALIGN(8)

SymCryptWipeAsmSmall:
        // Q1 = pbData, possibly unaligned
        // Q2 = cbData; Q2 < 16
        //
        // With speculative execution attacks, the cost of a jump table is prohibitive.
        // We use a compare ladder for 5 cases:
        //       8-15 bytes
        //       4-7 bytes
        //       2-3 bytes
        //       1 byte
        //       0 bytes

        xor     D0,D0

        cmp     D2, 8
        jb      SymCryptWipeAsmSmallLessThan8

        // wipe 8-15 bytes using two possibly overlapping writes
        mov     [Q1],Q0
        mov     [Q1+Q2-8],Q0
        ret

SymCryptWipeAsmSmallLessThan8:
        cmp     D2, 4
        jb      SymCryptWipeAsmSmallLessThan4

        // wipe 4-7 bytes
        mov     [Q1],D0
        mov     [Q1+Q2-4],D0
        ret

SymCryptWipeAsmSmallLessThan4:
        cmp     D2, 2
        jb      SymCryptWipeAsmSmallLessThan2

        // wipe 2-3 bytes
        mov     [Q1],W0
        mov     [Q1+Q2-2],W0
        ret

SymCryptWipeAsmSmallLessThan2:
        or      D2,D2
        jz      SymCryptWipeAsmSmallDone

        // wipe 1 byte
        mov     [Q1],B0

SymCryptWipeAsmSmallDone:


ret
.seh_endproc
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q3
#undef D3
#undef W3
#undef B3

FILE_END()
