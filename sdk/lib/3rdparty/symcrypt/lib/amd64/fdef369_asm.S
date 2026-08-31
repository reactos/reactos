//
//  fdef_369asm.asm   Assembler code for large integer arithmetic in the default data format
//  Expresses asm in a generic enough way to enable generation of MASM and GAS using the
//  symcryptasm_processor.py script and C preprocessor
//
// This file contains alternative routines that are used for modular computations
// where the modulus is 257-384 or 513-576 bits long.
// (Currently on ARM64 it is also used for 0-192-bit moduli but not on AMD64)
//
// The immediate advantage is that it improves EC performance on 384, and 521-bit curves.
//
// Most of this code is a direct copy of the default code.
// AMD64 digits are now 512 bits.
// We read the 'ndigit' value. If it is 1 digit, the values are 6 64-bit words, if it is 2 the values
// are 9 64-bit words. As we compute in groups of 3, our loop counters are one more than nDigit
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.

#include "symcryptasm_shared.cppasm"

// A digit consists of 4 words of 64 bits each

//UINT32
//SYMCRYPT_CALL
// SymCryptFdef369RawAddAsm(
//     _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
//     _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src2,
//     _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
//                                                             UINT32      nDigits )
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
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm)


.seh_endprologue


        inc     D4
        xor     Q0, Q0

SymCryptFdef369RawAddAsmLoop:
        // carry is in the carry flag
        mov     Q0,[Q1]
        adc     Q0,[Q2]
        mov     [Q3],Q0

        mov     Q0,[Q1 + 8]
        adc     Q0,[Q2 + 8]
        mov     [Q3 + 8], Q0

        mov     Q0,[Q1 + 16]
        adc     Q0,[Q2 + 16]
        mov     [Q3 + 16], Q0

        lea     Q1, [Q1 + 24]
        lea     Q2, [Q2 + 24]
        lea     Q3, [Q3 + 24]
        dec     D4
        jnz     SymCryptFdef369RawAddAsmLoop

        mov     Q0, 0
        adc     Q0, Q0


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
#undef Q4
#undef D4
#undef W4
#undef B4

// UINT32
// SYMCRYPT_CALL
// SymCryptFdef369RawSubAsm(
//     _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
//     _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
//     _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
//                                                             UINT32      nDigits )

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
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm)


.seh_endprologue


        inc     D4
        xor     Q0, Q0

SymCryptFdef369RawSubAsmLoop:
        // carry is in the carry flag
        mov     Q0,[Q1]
        sbb     Q0,[Q2]
        mov     [Q3],Q0

        mov     Q0,[Q1 + 8]
        sbb     Q0,[Q2 + 8]
        mov     [Q3 + 8], Q0

        mov     Q0,[Q1 + 16]
        sbb     Q0,[Q2 + 16]
        mov     [Q3 + 16], Q0

        lea     Q1, [Q1 + 24]
        lea     Q2, [Q2 + 24]
        lea     Q3, [Q3 + 24]
        dec     D4
        jnz     SymCryptFdef369RawSubAsmLoop

        mov     Q0, 0
        adc     Q0, Q0


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
#undef Q4
#undef D4
#undef W4
#undef B4

// VOID
// SYMCRYPT_CALL
// SymCryptFdef369MaskedCopyAsm(
//     _In_reads_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )        PCBYTE      pbSrc,
//     _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbDst,
//                                                                 UINT32      nDigits,
//                                                                 UINT32      mask )

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
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q5 r10
#define D5 r10d
#define W5 r10w
#define B5 r10b
PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm)


.seh_endprologue


        inc     D3
        movsxd  Q4, D4

SymCryptFdef369MaskedCopyAsmLoop:
        mov     Q0, [Q1]
        mov     Q5, [Q2]
        xor     Q0, Q5
        and     Q0, Q4
        xor     Q0, Q5
        mov     [Q2], Q0

        mov     Q0, [Q1 + 8]
        mov     Q5, [Q2 + 8]
        xor     Q0, Q5
        and     Q0, Q4
        xor     Q0, Q5
        mov     [Q2 + 8], Q0

        mov     Q0, [Q1 + 16]
        mov     Q5, [Q2 + 16]
        xor     Q0, Q5
        and     Q0, Q4
        xor     Q0, Q5
        mov     [Q2 + 16], Q0

        // Move on to the next digit

        add     Q1, 24
        add     Q2, 24
        dec     D3
        jnz     SymCryptFdef369MaskedCopyAsmLoop


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
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q5
#undef D5
#undef W5
#undef B5

// VOID
// SYMCRYPT_CALL
// SymCryptFdef369RawMulAsm(
//     _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
//                                                                     UINT32      nDigits1,
//     _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
//                                                                     UINT32      nDigits2,
//     _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst )

#define QH rdx
#define DH edx
#define WH dx
#define BH dl
#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q2 r10
#define D2 r10d
#define W2 r10w
#define B2 r10b
#define Q5 r11
#define D5 r11d
#define W5 r11w
#define B5 r11b
#define Q6 rsi
#define D6 esi
#define W6 si
#define B6 sil
#define Q7 rdi
#define D7 edi
#define W7 di
#define B7 dil
#define Q8 rbp
#define D8 ebp
#define W8 bp
#define B8 bpl
#define Q9 rbx
#define D9 ebx
#define W9 bx
#define B9 bl
#define Q10 r12
#define D10 r12d
#define W10 r12w
#define B10 r12b
PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm)

.byte 0x40; push Q6
.seh_pushreg Q6
push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10

.seh_endprologue

mov Q2, QH
mov Q5, [rsp + 80]

        // Basic structure:
        //   for each word in Src1:
        //       Dst += Src2 * word
        // Register assignments
        //
        // Q0 = tmp for mul
        // QH = tmp for mul
        // Q1 = pSrc1  (updated in outer loop)
        // D2 = # words left from Src1 to process
        // Q3 = pSrc2
        // Q4 = nDigits2
        // Q5 = pDst (incremented in outer loop)
        // Q6 = inner loop pointer into pSrc2
        // Q7 = inner loop pointer into pDst
        // Q8 = word from Src1 to multiply with
        // Q9 = carry
        // D10 = inner loop counter

        inc     D2
        inc     D4
        lea     D2, [D2 + 2*D2]     // nDigits1 * 3 = # words in Src1 to process

        // Outer loop invariant established: Q1, Q3, D4, Q5

        mov     Q6, Q3              // Q6 = pSrc2
        mov     Q7, Q5              // Q7 = pDst + outer loop ctr
        mov     Q8, [Q1]            // mulword
        xor     Q9, Q9
        mov     D10, D4

        // First inner loop overwrites Dst, which avoids adding the current Dst value

ALIGN(16)

SymCryptFdef369RawMulAsmLoop1:
        mov     Q0, [Q6]
        mul     Q8
        add     Q0, Q9
        adc     QH, 0
        mov     [Q7], Q0
        mov     Q9, QH

        mov     Q0, [Q6 + 8]
        mul     Q8
        add     Q0, Q9
        adc     QH, 0
        mov     [Q7 + 8], Q0
        mov     Q9, QH

        mov     Q0, [Q6 + 16]
        mul     Q8
        add     Q0, Q9
        adc     QH, 0
        mov     [Q7 + 16], Q0
        mov     Q9, QH

        add     Q6, 24
        add     Q7, 24
        dec     D10
        jnz     SymCryptFdef369RawMulAsmLoop1

        mov     [Q7], QH                // write last word, cannot overflow because Dst is at least 2 digits long

        dec     D2

ALIGN(16)

SymCryptFdef369RawMulAsmLoopOuter:

        add     Q1, 8                   // move to next word of pSrc1
        add     Q5, 8                   // move Dst pointer one word over
        mov     Q8, [Q1]
        mov     Q6, Q3
        mov     Q7, Q5
        xor     Q9, Q9
        mov     D10, D4

ALIGN(16)

SymCryptFdef369RawMulAsmLoop2:
        mov     Q0, [Q6]
        mul     Q8
        add     Q0, [Q7]
        adc     QH, 0
        add     Q0, Q9
        adc     QH, 0
        mov     [Q7], Q0
        mov     Q9, QH

        mov     Q0, [Q6 + 8]
        mul     Q8
        add     Q0, [Q7 + 8]
        adc     QH, 0
        add     Q0, Q9
        adc     QH, 0
        mov     [Q7 + 8], Q0
        mov     Q9, QH

        mov     Q0, [Q6 + 16]
        mul     Q8
        add     Q0, [Q7 + 16]
        adc     QH, 0
        add     Q0, Q9
        adc     QH, 0
        mov     [Q7 + 16], Q0
        mov     Q9, QH

        add     Q6, 24
        add     Q7, 24
        dec     D10
        jnz     SymCryptFdef369RawMulAsmLoop2

        mov     [Q7], QH           // write next word. (stays within Dst buffer)

        dec     D2
        jnz     SymCryptFdef369RawMulAsmLoopOuter


pop Q10
pop Q9
pop Q8
pop Q7
pop Q6
ret
.seh_endproc
#undef QH
#undef DH
#undef WH
#undef BH
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q3
#undef D3
#undef W3
#undef B3
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q5
#undef D5
#undef W5
#undef B5
#undef Q6
#undef D6
#undef W6
#undef B6
#undef Q7
#undef D7
#undef W7
#undef B7
#undef Q8
#undef D8
#undef W8
#undef B8
#undef Q9
#undef D9
#undef W9
#undef B9
#undef Q10
#undef D10
#undef W10
#undef B10

// VOID
// SYMCRYPT_CALL
// SymCryptFdef369MontgomeryReduceAsm(
//     _In_                            PCSYMCRYPT_MODULUS      pmMod,
//     _Inout_                         PUINT32                 pSrc,
//     _Out_                           PUINT32                 pDst )

#define QH rdx
#define DH edx
#define WH dx
#define BH dl
#define Q0 rax
#define D0 eax
#define W0 ax
#define B0 al
#define Q1 rcx
#define D1 ecx
#define W1 cx
#define B1 cl
#define Q3 r8
#define D3 r8d
#define W3 r8w
#define B3 r8b
#define Q4 r9
#define D4 r9d
#define W4 r9w
#define B4 r9b
#define Q2 r10
#define D2 r10d
#define W2 r10w
#define B2 r10b
#define Q5 r11
#define D5 r11d
#define W5 r11w
#define B5 r11b
#define Q6 rsi
#define D6 esi
#define W6 si
#define B6 sil
#define Q7 rdi
#define D7 edi
#define W7 di
#define B7 dil
#define Q8 rbp
#define D8 ebp
#define W8 bp
#define B8 bpl
#define Q9 rbx
#define D9 ebx
#define W9 bx
#define B9 bl
#define Q10 r12
#define D10 r12d
#define W10 r12w
#define B10 r12b
#define Q11 r13
#define D11 r13d
#define W11 r13w
#define B11 r13b
#define Q12 r14
#define D12 r14d
#define W12 r14w
#define B12 r14b
PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm)

.byte 0x40; push Q6
.seh_pushreg Q6
push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10
push Q11
.seh_pushreg Q11
push Q12
.seh_pushreg Q12

.seh_endprologue

mov Q2, QH

        mov     D4, [Q1 + SymCryptModulusNdigitsOffsetAmd64]    // nDigits
        inc     D4
        mov     Q5, [Q1 + SymCryptModulusInv64OffsetAmd64]      // inv64

        lea     Q1, [Q1 + SymCryptModulusValueOffsetAmd64]      // modulus value

        lea     D12, [D4 + 2*D4]  // outer loop counter, in words

        xor     D8, D8

        // General register allocations
        // Q0 = multiply result
        // QH = multiply result
        // Q1 = pointer to modulus value
        // Q2 = pSrc (updated in outer loop)
        // Q3 = pDst
        // D4 = nDigits
        // Q5 = pmMod->inv64
        // Q6 = multiplier in inner loop
        // Q7 = carry
        // Q8 = carry out from last word of previous loop iteration
        // Q9 = running pointer in Src
        // Q10 = running pointer in Mod
        // D11 = loop counter
        // D12 = outer loop counter (words)

ALIGN(16)

SymCryptFdef369MontgomeryReduceAsmOuterLoop:

        // start decoder with a few simple instructions, including at least one that requires
        // a uop execution and is on the critical path

        mov     Q6, [Q2]                      // fetch word of Src we want to set to zero
        mov     Q10, Q2
        mov     Q9, Q1

        imul    Q6, Q5                        // lower word is same for signed & unsigned multiply

        mov     D11, D4
        xor     D7, D7

ALIGN(16)

SymCryptFdef369MontgomeryReduceAsmInnerloop:
        // Q0 = mul scratch
        // QH = mul scratch
        // Q1 = pointer to modulus value
        // Q6 = multiplier
        // Q7 = carry (64 bits)
        // Q9  = running ptr to modulus
        // Q10 = running ptr to input/scratch
        // D11 = inner loop counter (digits)
        // D12 = outer loop counter (words)

        mov     Q0, [Q9]
        mul     Q6
        add     Q0, [Q10]
        adc     QH, 0
        add     Q0, Q7
        adc     QH, 0
        mov     [Q10], Q0
        mov     Q7, QH

        mov     Q0, [Q9 + 8]
        mul     Q6
        add     Q0, [Q10 + 8]
        adc     QH, 0
        add     Q0, Q7
        adc     QH, 0
        mov     [Q10 + 8], Q0
        mov     Q7, QH

        mov     Q0, [Q9 + 16]
        mul     Q6
        add     Q0, [Q10 + 16]
        adc     QH, 0
        add     Q0, Q7
        adc     QH, 0
        mov     [Q10 + 16], Q0
        mov     Q7, QH

        add     Q9, 24
        add     Q10, 24
        dec     D11
        jnz     SymCryptFdef369MontgomeryReduceAsmInnerloop

        add     Q7, Q8
        mov     D8, 0
        adc     Q8, 0
        add     Q7, [Q10]
        adc     Q8, 0
        mov     [Q10], Q7

        add     Q2, 8

        dec     D12
        jnz     SymCryptFdef369MontgomeryReduceAsmOuterLoop

        //
        // Most of the work is done - now all that is left is subtract the modulus if it is smaller than the result
        //

        // First we compute the pSrc result minus the modulus into the destination
        mov     D11, D4         // loop ctr
        mov     Q10, Q2         // pSrc
        mov     Q9, Q1          // pMod
        mov     Q7, Q3          // pDst

        // Cy = 0 because the last 'adc Q8,0' resulted in 0, 1, or 2

ALIGN(16)

SymCryptFdef369MontgomeryReduceAsmSubLoop:
        mov     Q0,[Q10]
        sbb     Q0,[Q9]
        mov     [Q7], Q0

        mov     Q0,[Q10 + 8]
        sbb     Q0,[Q9 + 8]
        mov     [Q7 + 8], Q0

        mov     Q0,[Q10 + 16]
        sbb     Q0,[Q9 + 16]
        mov     [Q7 + 16], Q0

        lea     Q10,[Q10 + 24]
        lea     Q9,[Q9 + 24]
        lea     Q7,[Q7 + 24]

        dec     D11
        jnz     SymCryptFdef369MontgomeryReduceAsmSubLoop

        // Finally a masked copy from pSrc to pDst
        // copy if: Q8 == 0 && Cy = 1
        sbb     Q8, 0              // mask (64 bits)

ALIGN(16)

SymCryptFdef369MontgomeryReduceAsmMaskedCopyLoop:
        mov     Q0, [Q2]
        mov     Q1, [Q3]
        xor     Q0, Q1
        and     Q0, Q8
        xor     Q0, Q1
        mov     [Q3], Q0

        mov     Q0, [Q2 + 8]
        mov     Q1, [Q3 + 8]
        xor     Q0, Q1
        and     Q0, Q8
        xor     Q0, Q1
        mov     [Q3 + 8], Q0

        mov     Q0, [Q2 + 16]
        mov     Q1, [Q3 + 16]
        xor     Q0, Q1
        and     Q0, Q8
        xor     Q0, Q1
        mov     [Q3 + 16], Q0

        // Move on to the next digit

        add     Q2, 24
        add     Q3, 24
        dec     D4
        jnz     SymCryptFdef369MontgomeryReduceAsmMaskedCopyLoop


pop Q12
pop Q11
pop Q10
pop Q9
pop Q8
pop Q7
pop Q6
ret
.seh_endproc
#undef QH
#undef DH
#undef WH
#undef BH
#undef Q0
#undef D0
#undef W0
#undef B0
#undef Q1
#undef D1
#undef W1
#undef B1
#undef Q3
#undef D3
#undef W3
#undef B3
#undef Q4
#undef D4
#undef W4
#undef B4
#undef Q2
#undef D2
#undef W2
#undef B2
#undef Q5
#undef D5
#undef W5
#undef B5
#undef Q6
#undef D6
#undef W6
#undef B6
#undef Q7
#undef D7
#undef W7
#undef B7
#undef Q8
#undef D8
#undef W8
#undef B8
#undef Q9
#undef D9
#undef W9
#undef B9
#undef Q10
#undef D10
#undef W10
#undef B10
#undef Q11
#undef D11
#undef W11
#undef B11
#undef Q12
#undef D12
#undef W12
#undef B12

FILE_END()
