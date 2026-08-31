//
//  fdef_asm.symcryptasm   Assembler code for large integer arithmetic in the default data format
//  Expresses asm in a generic enough way to enable generation of MASM and GAS using the
//  symcryptasm_processor.py script and C preprocessor
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "symcryptasm_shared.cppasm"


.macro MULT_SINGLEADD_128  index, src_reg, dst_reg, Q0, QH, mul_word, even_carry, odd_carry
        // \Q0 = mul scratch
        // \QH = mul scratch
        // \mul_word = multiplier
        // \src_reg = running ptr to input
        // \dst_reg = running ptr to output/scratch
        // \even_carry = carry for even words (64 bits)
        // \odd_carry = carry for odd words (64 bits)

        mov     \Q0, [\src_reg + 8*\index]
        mul     \mul_word
        mov     \odd_carry, \QH
        add     \Q0, \even_carry
        mov     [\dst_reg + 8*\index], \Q0
        adc     \odd_carry, 0

        mov     \Q0, [\src_reg + 8*(\index+1)]
        mul     \mul_word
        mov     \even_carry, \QH
        add     \Q0, \odd_carry
        mov     [\dst_reg + 8*(\index+1)], \Q0
        adc     \even_carry, 0
.endm

.macro MULT_DOUBLEADD_128  index, src_reg, dst_reg, Q0, QH, mul_word, even_carry, odd_carry
        // \Q0 = mul scratch
        // \QH = mul scratch
        // \mul_word = multiplier
        // \src_reg = running ptr to input
        // \dst_reg = running ptr to output/scratch
        // \even_carry = carry for even words (64 bits)
        // \odd_carry = carry for odd words (64 bits)

        mov     \Q0, [\src_reg + 8*\index]
        mul     \mul_word
        mov     \odd_carry, \QH
        add     \Q0, [\dst_reg + 8*\index]
        adc     \odd_carry, 0
        add     \Q0, \even_carry
        mov     [\dst_reg + 8*\index], \Q0
        adc     \odd_carry, 0

        mov     \Q0, [\src_reg + 8*(\index+1)]
        mul     \mul_word
        mov     \even_carry, \QH
        add     \Q0, [\dst_reg + 8*(\index+1)]
        adc     \even_carry, 0
        add     \Q0, \odd_carry
        mov     [\dst_reg + 8*(\index+1)], \Q0
        adc     \even_carry, 0
.endm

// Squaring

.macro SQR_SINGLEADD_64  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
        // \Q0 = mul scratch
        // \QH = mul scratch
        // \mul_word = multiplier
        // \src_reg = running ptr to input
        // \dst_reg = running ptr to output/scratch
        // \src_carry = input carry
        // \dst_carry = output carry

        mov     \Q0, [\src_reg + 8*\index]
        mul     \mul_word
        mov     \dst_carry, \QH
        add     \Q0, \src_carry
        mov     [\dst_reg + 8*\index], \Q0
        adc     \dst_carry, 0
.endm

.macro SQR_DOUBLEADD_64  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
        // \Q0 = mul scratch
        // \QH = mul scratch
        // \mul_word = multiplier
        // \src_reg = running ptr to input
        // \dst_reg = running ptr to output/scratch
        // \src_carry = input carry
        // \dst_carry = output carry

        mov     \Q0, [\src_reg + 8*\index]
        mul     \mul_word
        mov     \dst_carry, \QH
        add     \Q0, [\dst_reg + 8*\index]
        adc     \dst_carry, 0
        add     \Q0, \src_carry
        mov     [\dst_reg + 8*\index], \Q0
        adc     \dst_carry, 0
.endm

.macro SQR_SHIFT_LEFT  index, Q0, src_reg
    mov     \Q0, [\src_reg + 8*\index]
    adc     \Q0, \Q0                 // Shift left and add the carry
    mov     [\src_reg + 8*\index], \Q0
.endm

.macro SQR_DIAGONAL_PROP  index, src_reg, dst_reg, Q0, QH, carry
    // Calculating the square
    mov     \Q0, [\src_reg + 8*\index]     // mulword
    mul     \Q0                     // m^2

    // Adding the square to the even column
    add     \Q0, [\dst_reg + 16*\index]
    adc     \QH, 0
    add     \Q0, \carry
    adc     \QH, 0
    mov     [\dst_reg + 16*\index], \Q0

    // Propagating the sum to the next column
    mov     \Q0, \QH
    xor     \QH, \QH

    add     \Q0, [\dst_reg + 16*\index + 8]
    adc     \QH, 0
    mov     [\dst_reg + 16*\index + 8], \Q0
    mov     \carry, \QH
.endm

.macro MONTGOMERY14  Q0, QH, mul_word, pA, R0, R1, R2, R3, Cy
    // (xx, \R1, \R2, \R3, \QH) = \mul_word * (A0..3) + (\R0, \R1, \R2, \R3)
    // Used when it is statically known that \R0 will get set to 0, so we don't bother computing it
    // \Cy, \Q0 = scratch

    mov     \Q0, [\pA]
    mul     \mul_word
    add     \R0, -1  // set carry flag only when \R0 is non-zero
    adc     \QH, 0
    mov     \Cy, \QH

    mov     \Q0, [\pA + 8]
    mul     \mul_word
    add     \R1, \Q0
    adc     \QH, 0
    add     \R1, \Cy
    adc     \QH, 0
    mov     \Cy, \QH

    mov     \Q0, [\pA + 16]
    mul     \mul_word
    add     \R2, \Q0
    adc     \QH, 0
    add     \R2, \Cy
    adc     \QH, 0
    mov     \Cy, \QH

    mov     \Q0, [\pA + 24]
    mul     \mul_word
    add     \R3, \Q0
    adc     \QH, 0
    add     \R3, \Cy
    adc     \QH, 0
.endm

.macro MUL14  Q0, QH, mul_word, pA, R0, R1, R2, R3, Cy
    // (\R0, \R1, \R2, \R3, \QH) = \mul_word * (A0..3) + (\R0, \R1, \R2, \R3)
    // \Cy, \Q0 = scratch

    mov     \Q0, [\pA]
    mul     \mul_word
    add     \R0, \Q0
    adc     \QH, 0
    mov     \Cy, \QH

    mov     \Q0, [\pA + 8]
    mul     \mul_word
    add     \R1, \Q0
    adc     \QH, 0
    add     \R1, \Cy
    adc     \QH, 0
    mov     \Cy, \QH

    mov     \Q0, [\pA + 16]
    mul     \mul_word
    add     \R2, \Q0
    adc     \QH, 0
    add     \R2, \Cy
    adc     \QH, 0
    mov     \Cy, \QH

    mov     \Q0, [\pA + 24]
    mul     \mul_word
    add     \R3, \Q0
    adc     \QH, 0
    add     \R3, \Cy
    adc     \QH, 0
.endm

// Macros for size-specific squaring
.macro SQR_DOUBLEADD_64_2  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64    (\index),     \src_reg, \dst_reg, \Q0, \QH, \mul_word, \src_carry, \dst_carry
    SQR_DOUBLEADD_64    (\index + 1), \src_reg, \dst_reg, \Q0, \QH, \mul_word, \dst_carry, \src_carry
.endm

.macro SQR_DOUBLEADD_64_4  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64_2  (\index),     \src_reg, \dst_reg, \Q0, \QH, \mul_word, \src_carry, \dst_carry
    SQR_DOUBLEADD_64_2  (\index + 2), \src_reg, \dst_reg, \Q0, \QH, \mul_word, \src_carry, \dst_carry
.endm

.macro SQR_DOUBLEADD_64_8  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64_4  (\index),     \src_reg, \dst_reg, \Q0, \QH, \mul_word, \src_carry, \dst_carry
    SQR_DOUBLEADD_64_4  (\index + 4), \src_reg, \dst_reg, \Q0, \QH, \mul_word, \src_carry, \dst_carry
.endm

.macro SQR_SIZE_SPECIFIC_INIT  outer_src_reg, outer_dst_reg, inner_src_reg, inner_dst_reg, mul_word
    lea     \outer_src_reg, [\outer_src_reg + 8]  // move \outer_src_reg pointer 1 word over
    lea     \outer_dst_reg, [\outer_dst_reg + 16] // move \outer_dst_reg pointer 2 words over

    mov     \inner_src_reg, \outer_src_reg        // \inner_src_reg = \outer_src_reg
    mov     \inner_dst_reg, \outer_dst_reg        // \inner_dst_reg = \outer_dst_reg

    mov     \mul_word, [\outer_src_reg]           // Get the next mulword
    lea     \inner_src_reg, [\inner_src_reg + 8]  // move \inner_src_reg pointer 1 word over
.endm

//UINT32
//SYMCRYPT_CALL
//SymCryptFdefRawAdd(
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
//    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
//                                                            UINT32      nDigits )

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
PLATFORM_NAME_MANGLE(SymCryptFdefRawAddAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawAddAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawAddAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawAddAsm)


.seh_endprologue


        // loop over each half digit
        add     D4, D4
        xor     Q0, Q0

SymCryptFdefRawAddAsmLoop:
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

        mov     Q0,[Q1 + 24]
        adc     Q0,[Q2 + 24]
        mov     [Q3 + 24], Q0

        lea     Q1, [Q1 + 32]
        lea     Q2, [Q2 + 32]
        lea     Q3, [Q3 + 32]
        dec     D4
        jnz     SymCryptFdefRawAddAsmLoop

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

//UINT32
//SYMCRYPT_CALL
//SymCryptFdefRawSub(
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src2,
//    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
//                                                            UINT32      nDigits )

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
PLATFORM_NAME_MANGLE(SymCryptFdefRawSubAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawSubAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawSubAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawSubAsm)


.seh_endprologue


        // loop over each half digit
        add     D4, D4
        xor     Q0, Q0

SymCryptFdefRawSubAsmLoop:
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

        mov     Q0,[Q1 + 24]
        sbb     Q0,[Q2 + 24]
        mov     [Q3 + 24], Q0

        lea     Q1,[Q1 + 32]
        lea     Q2,[Q2 + 32]
        lea     Q3,[Q3 + 32]
        dec     D4
        jnz     SymCryptFdefRawSubAsmLoop

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

//VOID
//SYMCRYPT_CALL
//SymCryptFdefMaskedCopy(
//    _In_reads_bytes_( nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )      PCBYTE      pbSrc,
//    _InOut_writes_bytes_( nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )  PBYTE       pbDst,
//                                                                UINT32      nDigits,
//                                                                UINT32      mask )

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
PLATFORM_NAME_MANGLE(SymCryptFdefMaskedCopyAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefMaskedCopyAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefMaskedCopyAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefMaskedCopyAsm)


.seh_endprologue


        add     D3, D3              // loop over half digits

        movd    xmm0, D4            // xmm0[0] = mask
        pcmpeqd xmm1, xmm1          // xmm1 = ff...ff
        pshufd  xmm0, xmm0, 0       // xmm0[0..3] = mask
        pxor    xmm1, xmm0          // xmm1 = not Mask

SymCryptFdefMaskedCopyAsmLoop:
        movdqa  xmm2, [Q1]          // xmm2 = pSrc[i]
        movdqa  xmm3, [Q2]          // xmm3 = pDst[i]
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [Q2], xmm2

        movdqa  xmm2, [Q1 + 16]     // xmm2 = pSrc[i + 16]
        movdqa  xmm3, [Q2 + 16]     // xmm3 = pDst[i + 16]
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [Q2 + 16], xmm2

        // Move on to the next digit

        add     Q1, 32
        add     Q2, 32
        dec     D3
        jnz     SymCryptFdefMaskedCopyAsmLoop


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


//VOID
//SYMCRYPT_CALL
//SymCryptFdefRawMul(
//    _In_reads_(nDigits1 * SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
//                                                                      UINT32      nDigits1,
//    _In_reads_(nDigits2 * SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
//                                                                      UINT32      nDigits2,
//    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)     PUINT32     pDst )

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
PLATFORM_NAME_MANGLE(SymCryptFdefRawMulAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawMulAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawMulAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawMulAsm)

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

.seh_endprologue

mov Q2, QH
mov Q5, [rsp + 88]

        shl     Q2, 3           // nDigits1 * 8 = # words in Src1 to process

        // Basic structure:
        //   for each word in Src1:
        //       Dst += Src2 * word
        // Register assignments
        //
        // Q0 = tmp for mul
        // QH = tmp for mul
        // Q1 = pSrc1  (updated in outer loop)
        // Q2 = # words left from Src1 to process
        // Q3 = pSrc2
        // Q4 = nDigits2
        // Q5 = pDst (incremented in outer loop)
        // Q6 = inner loop pointer into pSrc2
        // Q7 = inner loop pointer into pDst
        // Q8 = word from Src1 to multiply with
        // Q9 = carry for even words (64 bits)
        // Q10 = inner loop counter
        // Q11 = carry for odd words (64 bits)


        // Outer loop invariant established: Q1, Q3, Q4, Q5

        mov     Q6, Q3          // Q6 = pSrc2
        mov     Q7, Q5          // Q7 = pDst + outer loop ctr
        mov     Q8, [Q1]        // mulword
        xor     Q9, Q9
        mov     Q10, Q4

        // First inner loop overwrites Dst, which avoids adding the current Dst value

ALIGN(16)

SymCryptFdefRawMulAsmLoop1:
        MULT_SINGLEADD_128 0, Q6, Q7, Q0, QH, Q8, Q9, Q11
        MULT_SINGLEADD_128 2, Q6, Q7, Q0, QH, Q8, Q9, Q11
        MULT_SINGLEADD_128 4, Q6, Q7, Q0, QH, Q8, Q9, Q11
        MULT_SINGLEADD_128 6, Q6, Q7, Q0, QH, Q8, Q9, Q11

        lea     Q6,[Q6 + 64]
        lea     Q7,[Q7 + 64]

        dec     Q10
        jnz     SymCryptFdefRawMulAsmLoop1

        mov     [Q7], Q9        // write last word, cannot overflow because Dst is at least 2 digits long

        dec     Q2

ALIGN(16)

SymCryptFdefRawMulAsmLoopOuter:

        add     Q1, 8           // move to next word of pSrc1
        add     Q5, 8           // move Dst pointer one word over
        mov     Q8, [Q1]
        mov     Q6, Q3
        mov     Q7, Q5
        xor     Q9, Q9
        mov     Q10, Q4

ALIGN(16)

SymCryptFdefRawMulAsmLoop2:
        MULT_DOUBLEADD_128 0, Q6, Q7, Q0, QH, Q8, Q9, Q11
        MULT_DOUBLEADD_128 2, Q6, Q7, Q0, QH, Q8, Q9, Q11
        MULT_DOUBLEADD_128 4, Q6, Q7, Q0, QH, Q8, Q9, Q11
        MULT_DOUBLEADD_128 6, Q6, Q7, Q0, QH, Q8, Q9, Q11

        lea     Q6,[Q6 + 64]
        lea     Q7,[Q7 + 64]

        dec     Q10
        jnz     SymCryptFdefRawMulAsmLoop2

        mov     [Q7], Q9        // write next word. (stays within Dst buffer)

        dec     Q2
        jnz     SymCryptFdefRawMulAsmLoopOuter


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

// VOID
// SYMCRYPT_CALL
// SymCryptFdefRawSquareAsm(
//   _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)        PCUINT32    pSrc,
//                                                          UINT32      nDigits,
//   _Out_writes_(2*nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)    PUINT32     pDst )

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
PLATFORM_NAME_MANGLE(SymCryptFdefRawSquareAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawSquareAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawSquareAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawSquareAsm)

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

        // Register assignments
        //
        // Q0 = tmp for mul
        // QH = tmp for mul
        // Q1 = outer loop pointer into pSrc
        // Q2 = nDigits (constant)
        // Q3 = pDst (constant)
        // Q4 = inner loop pointer into pSrc
        // Q5 = inner loop pointer into pDst
        // Q6 = word from Src to multiply with
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q9 = outer loop pointer into pDst
        // Q10 = outer loop counter of #words left
        // Q11 = inner loop counter of #words left
        // Q12 = cyclic counter that specifies on which branch we jump into

        ////////////////////////////////////////////////////////////////
        // First Pass - Addition of the cross products x_i*x_j with i!=j
        ////////////////////////////////////////////////////////////////
        //
        // At the beginning of each inner loop we will jump over the
        // words that don't need processing. The decision of the jump
        // will be based on the cyclic counter Q12.
        //
        // For the first pass we loop over **half** digits since having a smaller
        // number of jumps (i.e. 4) is actually faster than having 8 jumps.
        //
        ////////////////////////////////////////////////////////////////

        mov     [rsp + 64 /*MEMSLOT0*/], Q1   // save pSrc

        mov     Q10, Q2             // nDigits
        shl     Q10, 3              // Q10 = outer #words
        mov     Q9, Q3              // Q9 = outer pDst

        mov     Q4, Q1              // Q4 = inner pSrc
        mov     Q5, Q3              // Q5 = inner pDst

        // Initial inner loop overwrites Dst, which avoids adding the current Dst value

        mov     Q6, [Q1]            // mulword

        xor     Q7, Q7              // carry = 0
        xor     Q8, Q8              // carry = 0

        mov     Q11, Q10            // Q11 = inner #words
        mov     [Q5], Q7            // Write 0 in the first word

        // Skip over the first word
        jmp     SymCryptFdefRawSquareAsmInnerLoopInit_Word1

ALIGN(16)
SymCryptFdefRawSquareAsmInnerLoopInit_Word0:
        SQR_SINGLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q7, Q8

ALIGN(16)
SymCryptFdefRawSquareAsmInnerLoopInit_Word1:
        SQR_SINGLEADD_64 1, Q4, Q5, Q0, QH, Q6, Q8, Q7

        SQR_SINGLEADD_64 2, Q4, Q5, Q0, QH, Q6, Q7, Q8

        SQR_SINGLEADD_64 3, Q4, Q5, Q0, QH, Q6, Q8, Q7

        lea     Q4, [Q4 + 32]
        lea     Q5, [Q5 + 32]
        sub     Q11, 4
        jnz     SymCryptFdefRawSquareAsmInnerLoopInit_Word0

        mov     [Q5], Q7                // write last word, cannot overflow because Dst is at least 2 digits long

        dec     Q10                     // Counter for the outer loop
        mov     Q12, 1                  // Cyclic counter Q12 = 1

ALIGN(16)
SymCryptFdefRawSquareAsmLoopOuter:

        add     Q9, 8                   // move Dst pointer 1 word over

        mov     Q4, Q1                  // Q4 = inner pSrc
        mov     Q5, Q9                  // Q5 = inner pDst

        mov     Q6, [Q1 + 8*Q12]        // Get the next mulword

        inc     B12                     // Increment the cyclic counter by 1

        mov     Q11, Q10                // # of words for the inner loop
        add     Q11, 2
        and     Q11, -4                 // Zero out the 2 lower bits

        xor     Q7, Q7                  // carry = 0
        xor     Q8, Q8                  // carry = 0

        // Logic to find the correct jump
        cmp     B12, 3
        je      SymCryptFdefRawSquareAsmInnerLoop_Word3
        cmp     B12, 2
        je      SymCryptFdefRawSquareAsmInnerLoop_Word2
        cmp     B12, 1
        je      SymCryptFdefRawSquareAsmInnerLoop_Word1

        // The following instructions are only executed when B12 == 4
        xor     B12, B12                // Set it to 0 for the next iteration

        add     Q1, 32                  // move pSrc 4 words over
        add     Q9, 32                  // move destination 4 words over

        mov     Q4, Q1                  // Q4 = inner pSrc
        mov     Q5, Q9                  // Q5 = inner pDst

ALIGN(16)
SymCryptFdefRawSquareAsmInnerLoop_Word0:
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q7, Q8

ALIGN(16)
SymCryptFdefRawSquareAsmInnerLoop_Word1:
        SQR_DOUBLEADD_64 1, Q4, Q5, Q0, QH, Q6, Q8, Q7

ALIGN(16)
SymCryptFdefRawSquareAsmInnerLoop_Word2:
        SQR_DOUBLEADD_64 2, Q4, Q5, Q0, QH, Q6, Q7, Q8

ALIGN(16)
SymCryptFdefRawSquareAsmInnerLoop_Word3:
        SQR_DOUBLEADD_64 3, Q4, Q5, Q0, QH, Q6, Q8, Q7

        lea     Q4, [Q4 + 32]
        lea     Q5, [Q5 + 32]
        sub     Q11, 4
        jnz     SymCryptFdefRawSquareAsmInnerLoop_Word0

        mov     [Q5], Q7            // write next word. (stays within Dst buffer)

        dec     Q10
        cmp     Q10, 1
        jne     SymCryptFdefRawSquareAsmLoopOuter

        xor     QH, QH
        mov     [Q9 + 40], QH       // Final word = 0


        ////////////////////////////////////////////////////////////////
        // Second Pass - Shifting all results 1 bit left
        ////////////////////////////////////////////////////////////////

        mov     Q10, Q2             // nDigits
        mov     Q5, Q3              // pDst pointer
        shl     Q10, 1              // 2*nDigits

ALIGN(16)
SymCryptFdefRawSquareAsmSecondPass:
        SQR_SHIFT_LEFT 0, Q0, Q5
        SQR_SHIFT_LEFT 1, Q0, Q5
        SQR_SHIFT_LEFT 2, Q0, Q5
        SQR_SHIFT_LEFT 3, Q0, Q5

        SQR_SHIFT_LEFT 4, Q0, Q5
        SQR_SHIFT_LEFT 5, Q0, Q5
        SQR_SHIFT_LEFT 6, Q0, Q5
        SQR_SHIFT_LEFT 7, Q0, Q5

        lea     Q5, [Q5 + 64]
        dec     Q10
        jnz     SymCryptFdefRawSquareAsmSecondPass

        //////////////////////////////////////////////////////////////////////////////
        // Third Pass - Adding the squares on the even columns and propagating the sum
        //////////////////////////////////////////////////////////////////////////////

        mov     Q1, [rsp + 64 /*MEMSLOT0*/]  // Q1 = pSrc

SymCryptFdefRawSquareAsmThirdPass:
        SQR_DIAGONAL_PROP 0, Q1, Q3, Q0, QH, Q10
        SQR_DIAGONAL_PROP 1, Q1, Q3, Q0, QH, Q10
        SQR_DIAGONAL_PROP 2, Q1, Q3, Q0, QH, Q10
        SQR_DIAGONAL_PROP 3, Q1, Q3, Q0, QH, Q10
        SQR_DIAGONAL_PROP 4, Q1, Q3, Q0, QH, Q10
        SQR_DIAGONAL_PROP 5, Q1, Q3, Q0, QH, Q10
        SQR_DIAGONAL_PROP 6, Q1, Q3, Q0, QH, Q10
        SQR_DIAGONAL_PROP 7, Q1, Q3, Q0, QH, Q10

        add     Q1, 64              // One digit up
        add     Q3, 128             // Two digits up
        dec     Q2
        jnz     SymCryptFdefRawSquareAsmThirdPass


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

//VOID
//SymCryptFdefMontgomeryReduceAsm(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _Inout_                         PUINT32                 pSrc,
//    _Out_                           PUINT32                 pDst )

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
#define Q13 r15
#define D13 r15d
#define W13 r15w
#define B13 r15b
PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduceAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduceAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduceAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduceAsm)

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
push Q13
.seh_pushreg Q13

.seh_endprologue

mov Q2, QH

        mov     D4, [Q1 + SymCryptModulusNdigitsOffsetAmd64]    // nDigits
        mov     Q5, [Q1 + SymCryptModulusInv64OffsetAmd64]      // inv64

        lea     Q1, [Q1 + SymCryptModulusValueOffsetAmd64]      // modulus value

        mov     D13, D4         // outer loop counter
        shl     D13, 3          // D13 is in words

        xor     D9, D9

        // General register allocations
        // Q0 = multiply result
        // QH = multiply result
        // Q1 = pointer to modulus value
        // Q2 = pSrc (updated in outer loop)
        // Q3 = pDst
        // D4 = nDigits
        // Q5 = pmMod->inv64
        // Q6 = multiplier in inner loop
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q9 = carry out from last word of previous loop iteration
        // Q10 = running pointer in Src
        // Q11 = running pointer in Mod
        // Q12 = loop counter
        // Q13 = loop counter

ALIGN(16)

SymCryptFdefMontgomeryReduceAsmOuterLoop:

        // start decoder with a few simple instructions, including at least one that requires
        // a uop execution and is on the critical path

        mov     Q6, [Q2]                        // fetch word of Src we want to set to zero
        mov     Q11, Q2
        mov     Q10, Q1

        imul    Q6, Q5                          // lower word is same for signed & unsigned multiply

        mov     D12, D4
        xor     D7, D7

ALIGN(16)

SymCryptFdefMontgomeryReduceAsmInnerloop:
        // Q0 = mul scratch
        // QH = mul scratch
        // Q6 = multiplier
        // Q1 = pointer to modulus value
        // D13 = outer loop counter (words)
        // D12 = inner loop counter (digits)
        // Q10  = running ptr to modulus
        // Q11 = running ptr to input/scratch
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)

        MULT_DOUBLEADD_128 0, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 2, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 4, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 6, Q10, Q11, Q0, QH, Q6, Q7, Q8

        lea     Q10,[Q10 + 64]
        lea     Q11,[Q11 + 64]

        dec     D12
        jnz     SymCryptFdefMontgomeryReduceAsmInnerloop

        add     Q7, Q9
        mov     D9, 0
        adc     Q9, 0
        add     Q7, [Q11]
        adc     Q9, 0
        mov     [Q11], Q7

        lea     Q2,[Q2 + 8]

        dec     D13
        jnz     SymCryptFdefMontgomeryReduceAsmOuterLoop

        //
        // Most of the work is done - now all that is left is subtract the modulus if it is smaller than the result
        //

        // First we compute the pSrc result minus the modulus into the destination
        mov     D12, D4         // loop ctr
        mov     Q11, Q2         // pSrc
        mov     Q10, Q1         // pMod
        mov     Q7, Q3          // pDst

        // Cy = 0 because the last 'adc Q9,0' resulted in 0, 1, or 2

ALIGN(16)

SymCryptFdefMontgomeryReduceAsmSubLoop:
        mov     Q0,[Q11]
        sbb     Q0,[Q10]
        mov     [Q7], Q0

        mov     Q0,[Q11 + 8]
        sbb     Q0,[Q10 + 8]
        mov     [Q7 + 8], Q0

        mov     Q0,[Q11 + 16]
        sbb     Q0,[Q10 + 16]
        mov     [Q7 + 16], Q0

        mov     Q0,[Q11 + 24]
        sbb     Q0,[Q10 + 24]
        mov     [Q7 + 24], Q0

        mov     Q0,[Q11 + 32]
        sbb     Q0,[Q10 + 32]
        mov     [Q7 + 32], Q0

        mov     Q0,[Q11 + 40]
        sbb     Q0,[Q10 + 40]
        mov     [Q7 + 40], Q0

        mov     Q0,[Q11 + 48]
        sbb     Q0,[Q10 + 48]
        mov     [Q7 + 48], Q0

        mov     Q0,[Q11 + 56]
        sbb     Q0,[Q10 + 56]
        mov     [Q7 + 56], Q0

        lea     Q11,[Q11 + 64]
        lea     Q10,[Q10 + 64]
        lea     Q7,[Q7 + 64]

        dec     D12
        jnz     SymCryptFdefMontgomeryReduceAsmSubLoop

        // Finally a masked copy from pSrc to pDst
        // copy if: Q9 == 0 && Cy = 1
        sbb     D9, 0

        movd    xmm0, D9            // xmm0[0] = mask
        pcmpeqd xmm1, xmm1          // xmm1 = ff...ff
        pshufd  xmm0, xmm0, 0       // xmm0[0..3] = mask
        pxor    xmm1, xmm0          // xmm1 = not Mask

ALIGN(16)

SymCryptFdefMontgomeryReduceAsmMaskedCopyLoop:
        movdqa  xmm2, [Q2]          // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3]          // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3], xmm2

        movdqa  xmm2, [Q2 + 16]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 16]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 16], xmm2

        movdqa  xmm2, [Q2 + 32]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 32]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 32], xmm2

        movdqa  xmm2, [Q2 + 48]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 48]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 48], xmm2

        // Move on to the next digit
        lea     Q2,[Q2 + 64]
        lea     Q3,[Q3 + 64]

        dec     D4
        jnz     SymCryptFdefMontgomeryReduceAsmMaskedCopyLoop


pop Q13
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
#undef Q13
#undef D13
#undef W13
#undef B13


// --------------------------------
// 256 and 384-bit size specific functions
// --------------------------------

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
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
PLATFORM_NAME_MANGLE(SymCryptFdefModSub256Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefModSub256Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefModSub256Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefModSub256Asm)

.byte 0x40; push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8

.seh_endprologue


        // Q1 = pmMod
        // Q2 = peSrc1
        // Q3 = peSrc2
        // Q4 = peDst

        add     Q1, SymCryptModulusValueOffsetAmd64

        // compute Src1 - Src2 into (Q0, Q5, Q6, Q7), Cy = inverted subtraction borrow

        mov     Q0, [Q2 + 0*8]
        sub     Q0, [Q3 + 0*8]
        mov     Q5, [Q2 + 1*8]
        sbb     Q5, [Q3 + 1*8]
        mov     Q6, [Q2 + 2*8]
        sbb     Q6, [Q3 + 2*8]
        mov     Q7, [Q2 + 3*8]
        sbb     Q7, [Q3 + 3*8]

        // Q2, Q3: free
        // Load Mod into (Q2, Q3, Q8, Q1)

        mov     Q2, 0
        cmovb   Q2, [Q1 + 0*8]
        mov     Q3, 0
        cmovb   Q3, [Q1 + 1*8]
        mov     Q8, 0
        cmovb   Q8, [Q1 + 2*8]
        mov     Q1, [Q1 + 3*8]
        cmovnb  Q1, Q2

        // Add the (masked) modulus
        add     Q0, Q2
        adc     Q5, Q3
        adc     Q6, Q8
        adc     Q7, Q1

        mov     [Q4 + 0*8], Q0
        mov     [Q4 + 1*8], Q5
        mov     [Q4 + 2*8], Q6
        mov     [Q4 + 3*8], Q7


pop Q8
pop Q7
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
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
#define Q9 rbp
#define D9 ebp
#define W9 bp
#define B9 bpl
#define Q10 rbx
#define D10 ebx
#define W10 bx
#define B10 bl
#define Q11 r12
#define D11 r12d
#define W11 r12w
#define B11 r12b
#define Q12 r13
#define D12 r13d
#define W12 r13w
#define B12 r13b
PLATFORM_NAME_MANGLE(SymCryptFdefModSub384Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefModSub384Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefModSub384Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefModSub384Asm)

.byte 0x40; push Q7
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


        // Q1 = pmMod
        // Q2 = peSrc1
        // Q3 = peSrc2
        // Q4 = peDst

        add     Q1, SymCryptModulusValueOffsetAmd64

        // compute Src1 - Src2 into (Q0, Q5, Q6, Q7, Q8, Q9), Cy = inverted subtraction borrow

        mov     Q0, [Q2 + 0*8]
        sub     Q0, [Q3 + 0*8]
        mov     Q5, [Q2 + 1*8]
        sbb     Q5, [Q3 + 1*8]
        mov     Q6, [Q2 + 2*8]
        sbb     Q6, [Q3 + 2*8]
        mov     Q7, [Q2 + 3*8]
        sbb     Q7, [Q3 + 3*8]
        mov     Q8, [Q2 + 4*8]
        sbb     Q8, [Q3 + 4*8]
        mov     Q9, [Q2 + 5*8]
        sbb     Q9, [Q3 + 5*8]

        // Q2, Q3: free
        // Load Mod into (Q2, Q3, Q10, Q11, Q12, Q1)

        mov     Q2,  0
        cmovb   Q2,  [Q1 + 0*8]
        mov     Q3,  0
        cmovb   Q3,  [Q1 + 1*8]
        mov     Q10, 0
        cmovb   Q10, [Q1 + 2*8]
        mov     Q11, 0
        cmovb   Q11, [Q1 + 3*8]
        mov     Q12, 0
        cmovb   Q12, [Q1 + 4*8]
        mov     Q1,  [Q1 + 5*8]
        cmovnb  Q1,  Q2

        // Add the (masked) modulus
        add     Q0, Q2
        adc     Q5, Q3
        adc     Q6, Q10
        adc     Q7, Q11
        adc     Q8, Q12
        adc     Q9, Q1

        mov     [Q4 + 0*8], Q0
        mov     [Q4 + 1*8], Q5
        mov     [Q4 + 2*8], Q6
        mov     [Q4 + 3*8], Q7
        mov     [Q4 + 4*8], Q8
        mov     [Q4 + 5*8], Q9


pop Q12
pop Q11
pop Q10
pop Q9
pop Q8
pop Q7
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

//=================================================
// Multiplication
//

#if defined(SYMCRYPT_MASM)
altentry SymCryptFdefMontgomeryReduce256AsmInternal
#endif

//VOID
//SYMCRYPT_CALL
//SymCryptFdefModMulMontgomery256Asm(
//    _In_                            PCSYMCRYPT_MODULUS      pMod,
//    _In_                            PCSYMCRYPT_MODELEMENT   pSrc1,
//    _In_                            PCSYMCRYPT_MODELEMENT   pSrc2,
//    _Out_                           PSYMCRYPT_MODELEMENT    pDst,
//    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
//                                    SIZE_T                  cbScratch )

// Note we specify only 4 arguments as we never use arguments 5 and 6 (saves some prolog code in MSFT calling convention)
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
#define Q13 r15
#define D13 r15d
#define W13 r15w
#define B13 r15b
PLATFORM_NAME_MANGLE(SymCryptFdefModMulMontgomery256Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefModMulMontgomery256Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefModMulMontgomery256Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefModMulMontgomery256Asm)

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
push Q13
.seh_pushreg Q13

.seh_endprologue

mov Q2, QH

        // Q1 = pMod
        // Q2 = pSrc1
        // Q3 = pSrc2
        // Q4 = pDst

        // First we compute the product. The result will be in 8 registers
        //       Q6, Q7, Q8, Q9, Q10, Q11, Q12, Q13

        mov     Q5, [Q2]
        xor     Q8, Q8
        xor     Q9, Q9
        xor     Q10, Q10

        mov     Q0, [Q3]
        mul     Q5
        mov     Q6, Q0
        mov     Q7, QH

        mov     Q0, [Q3 + 8]
        mul     Q5
        add     Q7, Q0
        adc     Q8, QH

        mov     Q0, [Q3 + 16]
        mul     Q5
        add     Q8, Q0
        adc     Q9, QH

        mov     Q0, [Q3 + 24]
        mul     Q5
        add     Q9, Q0
        adc     Q10, QH

        // Second row
        mov     Q5, [Q2 + 8]
        MUL14   Q0, QH, Q5, Q3, Q7, Q8, Q9, Q10, Q13
        mov     Q11, QH

        // third row
        mov     Q5, [Q2 + 16]
        MUL14   Q0, QH, Q5, Q3, Q8, Q9, Q10, Q11, Q13
        mov     Q12, QH

        // fourth row
        mov     Q5, [Q2 + 24]
        MUL14   Q0, QH, Q5, Q3, Q9, Q10, Q11, Q12, Q13
        mov     Q13, QH

SymCryptFdefMontgomeryReduce256AsmInternal: .global SymCryptFdefMontgomeryReduce256AsmInternal
        // Invariant:
        //   common prologue used
        //   512-bit result in (Q6, Q7, Q8, Q9, Q10, Q11, Q12, Q13)
        //   Q1 = pmMod
        //   Q4 = pDst

        mov     Q3, [Q1 + SymCryptModulusInv64OffsetAmd64]      // inv64
        add     Q1, SymCryptModulusValueOffsetAmd64

        mov     Q5, Q6
        imul    Q5, Q3             // lower word is the same for signed & unsigned multiply - Q5 = multiplicand for first row
        MONTGOMERY14 Q0, QH, Q5, Q1, Q6, Q7, Q8, Q9, Q6
        mov     Q6, QH            // Save the out carries in (eventually) (Q6, Q7, Q8, Q9)

        mov     Q5, Q7
        imul    Q5, Q3
        MONTGOMERY14 Q0, QH, Q5, Q1, Q7, Q8, Q9, Q10, Q7
        mov     Q7, QH            // Save the out carries in (eventually) (Q6, Q7, Q8, Q9)

        mov     Q5, Q8
        imul    Q5, Q3
        MONTGOMERY14 Q0, QH, Q5, Q1, Q8, Q9, Q10, Q11, Q8
        mov     Q8, QH

        mov     Q5, Q9
        imul    Q5, Q3
        MONTGOMERY14 Q0, QH, Q5, Q1, Q9, Q10, Q11, Q12, Q9
        // mov   Q9, QH

        add     Q10, Q6
        adc     Q11, Q7
        adc     Q12, Q8
        adc     Q13, QH

        sbb     Q5, Q5        // Carry out from final addition in mask form

        // reduced value in (Q10, Q11, Q12, Q13, -Q5), and it is less than 2*Modulus

        mov     Q6, Q10
        sub     Q6, [Q1]
        mov     Q7, Q11
        sbb     Q7, [Q1 + 8]
        mov     Q8, Q12
        sbb     Q8, [Q1 + 16]
        mov     Q9, Q13
        sbb     Q9, [Q1 + 24]

        sbb     Q1, Q1        // Q1 = carry out mask

        // Choose between the two
        // addition carry = 1, then subtraction carry = 1 and we pick the 2nd result.
        // addition carry = 0 and subtraction carry = 0: pick 2nd result
        // addition carry = 0 and subtraction carry = 1: pick first result

        xor     Q1, Q5        // 0 = 2nd result, 1 = first result

        cmovz   Q10, Q6
        cmovz   Q11, Q7
        cmovz   Q12, Q8
        cmovz   Q13, Q9

        mov     [Q4 +  0], Q10
        mov     [Q4 +  8], Q11
        mov     [Q4 + 16], Q12
        mov     [Q4 + 24], Q13


pop Q13
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
#undef Q13
#undef D13
#undef W13
#undef B13


//VOID
//SYMCRYPT_CALL
//SymCryptFdefMontgomeryReduce256Asm(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _Inout_                         PUINT32                 pSrc,
//    _Out_                           PUINT32                 pDst )

// Note we specify 4 arguments so that our prolog matches SymCryptFdefModMulMontgomery256Asm
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
#define Q13 r15
#define D13 r15d
#define W13 r15w
#define B13 r15b
PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce256Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce256Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce256Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce256Asm)

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
push Q13
.seh_pushreg Q13

.seh_endprologue

mov Q2, QH

        mov     Q4, Q3
        mov     Q6,  [Q2 +  0]
        mov     Q7,  [Q2 +  8]
        mov     Q8,  [Q2 + 16]
        mov     Q9,  [Q2 + 24]
        mov     Q10, [Q2 + 32]
        mov     Q11, [Q2 + 40]
        mov     Q12, [Q2 + 48]
        mov     Q13, [Q2 + 56]

        // Normal code doesn't jump from the body of one function to the body of another function.
        // Here we have ensured that our stack frames are identical, so it is safe.
        // We just have to convince the other system components that this works...

        // Use conditional jump so that stack unwinder doesn't think it is an epilogue
        test    rsp,rsp
        jne     SymCryptFdefMontgomeryReduce256AsmInternal       // jumps always

        int     3       // Dummy instruction because the debugger seems to have an off-by-one
                        // error and still see the (wrong) epilogue when on the JNE instruction
                        // Best guess: the debugger starts the stack trace *after* the current instruction


pop Q13
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
#undef Q13
#undef D13
#undef W13
#undef B13

#if 0
//////////////////////////////////////////////////////////////
//
// UNUSED ASSEMBLY FROM HERE
// Kept in case we need it later, but removed at compile time
// to avoid inclusion in produced binaries (the linker struggles
// to remove unused assembly functions)
//
//////////////////////////////////////////////////////////////

//VOID
//SYMCRYPT_CALL
//SymCryptFdefModAdd256(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _In_                            PCSYMCRYPT_MODELEMENT   peSrc1,
//    _In_                            PCSYMCRYPT_MODELEMENT   peSrc2,
//    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
//    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
//                                    SIZE_T                  cbScratch )

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
#define Q6 r11
#define D6 r11d
#define W6 r11w
#define B6 r11b
#define Q7 rsi
#define D7 esi
#define W7 si
#define B7 sil
#define Q8 rdi
#define D8 edi
#define W8 di
#define B8 dil
#define Q9 rbp
#define D9 ebp
#define W9 bp
#define B9 bpl
#define Q10 rbx
#define D10 ebx
#define W10 bx
#define B10 bl
PLATFORM_NAME_MANGLE(SymCryptFdefModAdd256Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefModAdd256Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefModAdd256Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefModAdd256Asm)

.byte 0x40; push Q7
.seh_pushreg Q7
push Q8
.seh_pushreg Q8
push Q9
.seh_pushreg Q9
push Q10
.seh_pushreg Q10

.seh_endprologue


        // Q1 = pmMod
        // Q2 = peSrc1
        // Q3 = peSrc2
        // Q4 = peDst

        // compute Src1 + Src2 into (Q0, Q5, Q6, Q7) with carry out mask in Q8

        mov     Q0, [Q2 + 0*8]
        add     Q0, [Q3 + 0*8]
        mov     Q5, [Q2 + 1*8]
        adc     Q5, [Q3 + 1*8]
        mov     Q6, [Q2 + 2*8]
        adc     Q6, [Q3 + 2*8]
        mov     Q7, [Q2 + 3*8]
        adc     Q7, [Q3 + 3*8]
        sbb     Q8, Q8                  // Q8 = carry out mask

        // Q2, Q3: free
        // Compute sum - Mod into (Q2, Q3, Q9, Q10) = sum - modulus, Q1 = carry out mask

        add     Q1, SymCryptModulusValueOffsetAmd64

        mov     Q2,  Q0
        sub     Q2,  [Q1 + 0*8]
        mov     Q3,  Q5
        sbb     Q3,  [Q1 + 1*8]
        mov     Q9,  Q6
        sbb     Q9,  [Q1 + 2*8]
        mov     Q10, Q7
        sbb     Q10, [Q1 + 3*8]

        sbb     Q1, Q1                 // Q1 = carry out mask

        // Choose between the two
        // addition carry = 1, then subtraction carry = 1 and we pick the 2nd result.
        // addition carry = 0 and subtraction carry = 0: pick 2nd result
        // addition carry = 0 and subtraction carry = 1: pick first result

        xor     Q1, Q8            // 0 = 2nd result, 1 = first result

        cmovz   Q0, Q2
        cmovz   Q5, Q3
        cmovz   Q6, Q9
        cmovz   Q7, Q10

        mov     [Q4 + 0*8], Q0
        mov     [Q4 + 1*8], Q5
        mov     [Q4 + 2*8], Q6
        mov     [Q4 + 3*8], Q7


pop Q10
pop Q9
pop Q8
pop Q7
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

//VOID
//SYMCRYPT_CALL
//SymCryptFdefModSquareMontgomery256(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _In_                            PCSYMCRYPT_MODELEMENT   peSrc,
//    _Out_                           PSYMCRYPT_MODELEMENT    peDst,
//    _Out_writes_bytes_( cbScratch ) PBYTE                   pbScratch,
//                                    SIZE_T                  cbScratch )

// Note we specify 4 arguments so that our prolog matches SymCryptFdefModMulMontgomery256Asm
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
#define Q13 r15
#define D13 r15d
#define W13 r15w
#define B13 r15b
PLATFORM_NAME_MANGLE(SymCryptFdefModSquareMontgomery256Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefModSquareMontgomery256Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefModSquareMontgomery256Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefModSquareMontgomery256Asm)

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
push Q13
.seh_pushreg Q13

.seh_endprologue

mov Q2, QH

        //  Result in   Q6, Q7, Q8, Q9, Q10, Q11, Q12, Q13

        // Q1 = pmMod
        // Q2 = peSrc
        // Q3 = peDst

        mov     Q4, Q3
        mov     Q5, [Q2]
        xor     Q9, Q9
        xor     Q10, Q10
        xor     Q11, Q11
        xor     Q12, Q12

        // First we compute all the terms that need doubling

        mov     Q0, [Q2 + 8]
        mul     Q5
        mov     Q7, Q0
        mov     Q8, QH

        mov     Q0, [Q2 + 16]
        mul     Q5
        add     Q8, Q0
        adc     Q9, QH

        mov     Q0, [Q2 + 24]
        mul     Q5
        add     Q9, Q0
        adc     Q10, QH

        mov     Q5, [Q2 + 8]
        mov     Q0, [Q2 + 16]
        mul     Q5
        add     Q9, Q0
        adc     QH, 0
        mov     Q13, QH

        mov     Q0, [Q2 + 24]
        mul     Q5
        add     Q10, Q0
        adc     QH, 0
        add     Q10, Q13
        adc     Q11, QH

        mov     Q5, [Q2 + 16]
        mov     Q0, [Q2 + 24]
        mul     Q5
        add     Q11, Q0
        adc     Q12, QH        // no overflow from this

        // double these terms
        xor     Q13, Q13

        add     Q7, Q7
        adc     Q8, Q8
        adc     Q9, Q9
        adc     Q10, Q10
        adc     Q11, Q11
        adc     Q12, Q12
        adc     Q13, 0

        mov     Q0, [Q2]
        mul     Q0
        mov     Q6, Q0
        mov     Q5, QH

        mov     Q0, [Q2 + 8]
        mul     Q0

        add     Q7, Q5
        adc     Q8, Q0
        adc     Q9, QH
        sbb     Q3, Q3          // -carry

        mov     Q0, [Q2 + 16]
        mul     Q0

        add     Q3, Q3
        adc     Q10, Q0
        adc     Q11, QH
        sbb     Q3, Q3

        mov     Q0, [Q2 + 24]
        mul     Q0
        add     Q3, Q3
        adc     Q12, Q0
        adc     Q13, QH

        // See SymCryptFdefMontgomeryReduce256Asm for a discussion of this strange epilogue sequence
        test    rsp,rsp
        jne     SymCryptFdefMontgomeryReduce256AsmInternal       // jumps always

        int     3


pop Q13
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
#undef Q13
#undef D13
#undef W13
#undef B13

// --------------------------------
// 512-bit size specific functions
// --------------------------------

//VOID
//SYMCRYPT_CALL
//SymCryptFdefRawMul512Asm(
//    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc1,
//    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc2,
//                                                        UINT32      nDigits,
//    _Out_writes_(2*nWords)                              PUINT32     pDst )
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
PLATFORM_NAME_MANGLE(SymCryptFdefRawMul512Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawMul512Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawMul512Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawMul512Asm)

.byte 0x40; push Q6
.seh_pushreg Q6
push Q7
.seh_pushreg Q7

.seh_endprologue

mov Q2, QH

        // Basic structure:
        //   for each word in Src1:
        //       Dst += Src2 * word
        // Register assignments
        //
        // Q0 = tmp for mul
        // QH = tmp for mul
        // Q1 = pSrc1  (updated in outer loop)
        // Q2 = pSrc2 (constant)
        // Q3 = # words left from Src1 to process
        // Q4 = pDst (incremented in outer loop)
        // Q5 = word from Src1 to multiply with
        // Q6 = carry for even words (64 bits)
        // Q7 = carry for odd words (64 bits)

        shl     Q3, 3               // nDigits * 8 = # words in Src1 to process

        mov     Q5, [Q1]            // mulword
        xor     Q6, Q6              // carry

        // First inner loop overwrites Dst, which avoids adding the current Dst value
        MULT_SINGLEADD_128 0, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 2, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 4, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 6, Q2, Q4, Q0, QH, Q5, Q6, Q7

        mov     [Q4 + 64], Q6       // write last word, cannot overflow because Dst is at least 2 digits long

        dec     Q3

ALIGN(16)

SymCryptFdefRawMul512AsmLoopOuter:

        lea     Q1, [Q1 + 8]        // move to next word of pSrc1
        lea     Q4, [Q4 + 8]        // move Dst pointer one word over

        mov     Q5, [Q1]            // mulword
        xor     Q6, Q6              // carry

        MULT_DOUBLEADD_128 0, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 2, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 4, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 6, Q2, Q4, Q0, QH, Q5, Q6, Q7

        mov     [Q4 + 64], Q6       // write last word, cannot overflow because Dst is at least 2 digits long

        dec     Q3
        jnz     SymCryptFdefRawMul512AsmLoopOuter


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

// VOID
// SYMCRYPT_CALL
// SymCryptFdefRawSquareAsm(
//   _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc,
//                                                       UINT32      nDigits,
//   _Out_writes_(2*nWords)                              PUINT32     pDst )
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
PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare512Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare512Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare512Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare512Asm)

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

        // Register assignments
        //
        // Q0 = tmp for mul
        // QH = tmp for mul
        // Q1 = outer loop pointer into pSrc
        // Q2 = nDigits (constant)
        // Q3 = pDst (constant)
        // Q4 = inner loop pointer into pSrc
        // Q5 = inner loop pointer into pDst
        // Q6 = word from Src to multiply with
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q9 = outer loop pointer into pDst
        // Q10 = outer loop counter of #words left

        mov     [rsp + 48 /*MEMSLOT0*/], Q1   // save pSrc

        ////////////////////////////////////////////////////////////////
        // First Pass - Addition of the cross products x_i*x_j with i!=j
        ////////////////////////////////////////////////////////////////

        mov     Q9, Q3              // Q9 = outer pDst

        mov     Q4, Q1              // Q4 = inner pSrc
        mov     Q5, Q3              // Q5 = inner pDst

        // Initial inner loop overwrites Dst, which avoids adding the current Dst value
        // 7 iterations
        xor     Q8, Q8              // carry = 0 (for "odd" iterations set only the Q8 carry)
        mov     Q6, [Q1]            // mulword
        mov     [Q5], Q8            // Write 0 in the first word

        SQR_SINGLEADD_64 1, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_SINGLEADD_64 2, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 3, Q4, Q5, Q0, QH, Q6, Q8, Q7

        SQR_SINGLEADD_64 4, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 5, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_SINGLEADD_64 6, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 7, Q4, Q5, Q0, QH, Q6, Q8, Q7

        mov     [Q5 + 8*8], Q7      // write last word, cannot overflow because Dst is at least 2 digits long
        add     Q9, 8               // Skip over the first word

        // 6 iterations
        xor     Q7, Q7              // carry = 0 (for "even" iterations set only the Q7 carry)
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_2 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_4 2, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 6*8], Q7

        // 5 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7  // Notice the dst_carry is Q7 since all the "double" macros have Q7 as src_carry
        SQR_DOUBLEADD_64_4 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 5*8], Q7

        // 4 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_4 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 4*8], Q7

        // 3 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_DOUBLEADD_64_2 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 3*8], Q7

        // 2 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_2 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 2*8], Q7

        // 1 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        mov     [Q5 + 8], Q7

        xor     QH, QH
        mov     [Q5 + 16], QH      // Final word = 0


        ////////////////////////////////////////////////////////////////
        // Second Pass - Shifting all results 1 bit left
        ////////////////////////////////////////////////////////////////

        xor     Q0, Q0              // carry flag = 0
        mov     Q5, Q3              // pDst pointer

        SQR_SHIFT_LEFT 0, Q0, Q5
        SQR_SHIFT_LEFT 1, Q0, Q5
        SQR_SHIFT_LEFT 2, Q0, Q5
        SQR_SHIFT_LEFT 3, Q0, Q5

        SQR_SHIFT_LEFT 4, Q0, Q5
        SQR_SHIFT_LEFT 5, Q0, Q5
        SQR_SHIFT_LEFT 6, Q0, Q5
        SQR_SHIFT_LEFT 7, Q0, Q5

        SQR_SHIFT_LEFT 8, Q0, Q5
        SQR_SHIFT_LEFT 9, Q0, Q5
        SQR_SHIFT_LEFT 10, Q0, Q5
        SQR_SHIFT_LEFT 11, Q0, Q5

        SQR_SHIFT_LEFT 12, Q0, Q5
        SQR_SHIFT_LEFT 13, Q0, Q5
        SQR_SHIFT_LEFT 14, Q0, Q5
        SQR_SHIFT_LEFT 15, Q0, Q5

        //////////////////////////////////////////////////////////////////////////////
        // Third Pass - Adding the squares on the even columns and propagating the sum
        //////////////////////////////////////////////////////////////////////////////

        mov     Q1, [rsp + 48 /*MEMSLOT0*/]  // Q1 = pSrc
        xor     Q7, Q7

        SQR_DIAGONAL_PROP 0, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 1, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 2, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 3, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 4, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 5, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 6, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 7, Q1, Q3, Q0, QH, Q7


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

//VOID
//SymCryptFdefMontgomeryReduce512Asm(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _Inout_                         PUINT32                 pSrc,
//    _Out_                           PUINT32                 pDst )

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
PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce512Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce512Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce512Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce512Asm)

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

        mov     D4, [Q1 + SymCryptModulusNdigitsOffsetAmd64]            // nDigits
        mov     Q5, [Q1 + SymCryptModulusInv64OffsetAmd64]              // inv64

        lea     Q1, [Q1 + SymCryptModulusValueOffsetAmd64]              // modulus value

        mov     D12, D4         // outer loop counter
        shl     D12, 3          // D12 is in words

        xor     D9, D9

        // General register allocations
        // Q0 = multiply result
        // QH = multiply result
        // Q1 = pointer to modulus value
        // Q2 = pSrc (updated in outer loop)
        // Q3 = pDst
        // D4 = nDigits
        // Q5 = pmMod->inv64
        // Q6 = multiplier in inner loop
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q9 = carry out from last word of previous loop iteration
        // Q10 = running pointer in Src
        // Q11 = running pointer in Mod
        // D12 = loop counter

ALIGN(16)

SymCryptFdefMontgomeryReduce512AsmOuterLoop:

        // start decoder with a few simple instructions, including at least one that requires
        // a uop execution and is on the critical path

        mov     Q6, [Q2]                        // fetch word of Src we want to set to zero
        mov     Q11, Q2
        mov     Q10, Q1

        imul    Q6, Q5                          // lower word is same for signed & unsigned multiply

        xor     D7, D7

        // Q0 = mul scratch
        // QH = mul scratch
        // Q1 = pointer to modulus value
        // Q6 = multiplier
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q10 = running ptr to modulus
        // Q11 = running ptr to input/scratch
        // D12 = outer loop counter (words)

        MULT_DOUBLEADD_128 0, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 2, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 4, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 6, Q10, Q11, Q0, QH, Q6, Q7, Q8

        lea     Q11,[Q11 + 64]

        add     Q7, Q9
        mov     D9, 0
        adc     Q9, 0
        add     Q7, [Q11]
        adc     Q9, 0
        mov     [Q11], Q7

        lea     Q2,[Q2 + 8]

        dec     D12
        jnz     SymCryptFdefMontgomeryReduce512AsmOuterLoop

        //
        // Most of the work is done - now all that is left is subtract the modulus if it is smaller than the result
        //

        // First we compute the pSrc result minus the modulus into the destination
        mov     Q11, Q2         // pSrc
        mov     Q10, Q1         // pMod
        mov     Q7, Q3          // pDst

        // Cy = 0 because the last 'adc Q9,0' resulted in 0, 1, or 2
        mov     Q0,[Q11]
        sbb     Q0,[Q10]
        mov     [Q7], Q0

        mov     Q0,[Q11 + 8]
        sbb     Q0,[Q10 + 8]
        mov     [Q7 + 8], Q0

        mov     Q0,[Q11 + 16]
        sbb     Q0,[Q10 + 16]
        mov     [Q7 + 16], Q0

        mov     Q0,[Q11 + 24]
        sbb     Q0,[Q10 + 24]
        mov     [Q7 + 24], Q0

        mov     Q0,[Q11 + 32]
        sbb     Q0,[Q10 + 32]
        mov     [Q7 + 32], Q0

        mov     Q0,[Q11 + 40]
        sbb     Q0,[Q10 + 40]
        mov     [Q7 + 40], Q0

        mov     Q0,[Q11 + 48]
        sbb     Q0,[Q10 + 48]
        mov     [Q7 + 48], Q0

        mov     Q0,[Q11 + 56]
        sbb     Q0,[Q10 + 56]
        mov     [Q7 + 56], Q0

        lea     Q11,[Q11 + 64]
        lea     Q10,[Q10 + 64]
        lea     Q7,[Q7 + 64]

        // Finally a masked copy from pSrc to pDst
        // copy if: Q9 == 0 && Cy = 1
        sbb     D9, 0

        movd    xmm0, D9            // xmm0[0] = mask
        pcmpeqd xmm1, xmm1          // xmm1 = ff...ff
        pshufd  xmm0, xmm0, 0       // xmm0[0..3] = mask
        pxor    xmm1, xmm0          // xmm1 = not Mask

ALIGN(16)

SymCryptFdefMontgomeryReduce512AsmMaskedCopyLoop:
        movdqa  xmm2, [Q2]          // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3]          // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3], xmm2

        movdqa  xmm2, [Q2 + 16]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 16]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 16], xmm2

        movdqa  xmm2, [Q2 + 32]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 32]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 32], xmm2

        movdqa  xmm2, [Q2 + 48]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 48]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 48], xmm2

        // Move on to the next digit
        lea     Q2,[Q2 + 64]
        lea     Q3,[Q3 + 64]

        dec     D4
        jnz     SymCryptFdefMontgomeryReduce512AsmMaskedCopyLoop


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

// --------------------------------
// 1024-bit size specific functions
// --------------------------------

//VOID
//SYMCRYPT_CALL
//SymCryptFdefRawMul1024Asm(
//    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc1,
//    _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc2,
//                                                        UINT32      nDigits,
//    _Out_writes_(2*nWords)                              PUINT32     pDst )
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
PLATFORM_NAME_MANGLE(SymCryptFdefRawMul1024Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawMul1024Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawMul1024Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawMul1024Asm)

.byte 0x40; push Q6
.seh_pushreg Q6
push Q7
.seh_pushreg Q7

.seh_endprologue

mov Q2, QH

        // Basic structure:
        //   for each word in Src1:
        //       Dst += Src2 * word
        // Register assignments
        //
        // Q0 = tmp for mul
        // QH = tmp for mul
        // Q1 = pSrc1  (updated in outer loop)
        // Q2 = pSrc2 (constant)
        // Q3 = # words left from Src1 to process
        // Q4 = pDst (incremented in outer loop)
        // Q5 = word from Src1 to multiply with
        // Q6 = carry for even words (64 bits)
        // Q7 = carry for odd words (64 bits)

        shl     Q3, 3               // nDigits * 8 = # words in Src1 to process

        mov     Q5, [Q1]          // mulword
        xor     Q6, Q6              // carry

        // First inner loop overwrites Dst, which avoids adding the current Dst value
        MULT_SINGLEADD_128 0, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 2, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 4, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 6, Q2, Q4, Q0, QH, Q5, Q6, Q7

        MULT_SINGLEADD_128 8, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 10, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 12, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_SINGLEADD_128 14, Q2, Q4, Q0, QH, Q5, Q6, Q7

        mov     [Q4 + 128], Q6          // write last word, cannot overflow because Dst is at least 2 digits long

        dec     Q3

ALIGN(16)

SymCryptFdefRawMul1024AsmLoopOuter:

        lea     Q1, [Q1 + 8]        // move to next word of pSrc1
        lea     Q4, [Q4 + 8]        // move Dst pointer one word over

        mov     Q5, [Q1]            // mulword

        xor     Q6, Q6              // carry

        MULT_DOUBLEADD_128 0, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 2, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 4, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 6, Q2, Q4, Q0, QH, Q5, Q6, Q7

        MULT_DOUBLEADD_128 8, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 10, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 12, Q2, Q4, Q0, QH, Q5, Q6, Q7
        MULT_DOUBLEADD_128 14, Q2, Q4, Q0, QH, Q5, Q6, Q7

        mov     [Q4 + 128], Q6    // write last word, cannot overflow because Dst is at least 2 digits long

        dec     Q3
        jnz     SymCryptFdefRawMul1024AsmLoopOuter


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

// VOID
// SYMCRYPT_CALL
// SymCryptFdefRawSquareAsm(
//   _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc,
//                                                       UINT32      nDigits,
//   _Out_writes_(2*nWords)                              PUINT32     pDst )
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
PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare1024Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare1024Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare1024Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefRawSquare1024Asm)

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

        // Register assignments
        //
        // Q0 = tmp for mul
        // QH = tmp for mul
        // Q1 = outer loop pointer into pSrc
        // Q2 = nDigits (constant)
        // Q3 = pDst (constant)
        // Q4 = inner loop pointer into pSrc
        // Q5 = inner loop pointer into pDst
        // Q6 = word from Src to multiply with
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q9 = outer loop pointer into pDst
        // Q10 = outer loop counter of #words left

        mov     [rsp + 48 /*MEMSLOT0*/], Q1   // save pSrc

        ////////////////////////////////////////////////////////////////
        // First Pass - Addition of the cross products x_i*x_j with i!=j
        ////////////////////////////////////////////////////////////////

        mov     Q9, Q3              // Q9 = outer pDst

        mov     Q4, Q1              // Q4 = inner pSrc
        mov     Q5, Q3              // Q5 = inner pDst

        // Initial inner loop overwrites Dst, which avoids adding the current Dst value

        // 15 iterations
        xor     Q8, Q8              // carry = 0 (for "odd" iterations set only the Q8 carry)
        mov     Q6, [Q1]            // mulword
        mov     [Q5], Q8            // Write 0 in the first word

        SQR_SINGLEADD_64 1, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_SINGLEADD_64 2, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 3, Q4, Q5, Q0, QH, Q6, Q8, Q7

        SQR_SINGLEADD_64 4, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 5, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_SINGLEADD_64 6, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 7, Q4, Q5, Q0, QH, Q6, Q8, Q7

        SQR_SINGLEADD_64 8, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 9, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_SINGLEADD_64 10, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 11, Q4, Q5, Q0, QH, Q6, Q8, Q7

        SQR_SINGLEADD_64 12, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 13, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_SINGLEADD_64 14, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_SINGLEADD_64 15, Q4, Q5, Q0, QH, Q6, Q8, Q7

        mov     [Q5 + 16*8], Q7     // write last word, cannot overflow because Dst is at least 2 digits long
        add     Q9, 8               // Skip over the first word

        // 14 iterations (adding the current Dst value)
        xor     Q7, Q7            // carry = 0 (for "even" iterations set only the Q7 carry)
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_2 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_4 2, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_8 6, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 14*8], Q7

        // 13 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7  // Notice the dst_carry is Q7 since all the "double" macros have Q7 as src_carry
        SQR_DOUBLEADD_64_4 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_8 5, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 13*8], Q7

        // 12 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_4 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_8 4, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 12*8], Q7

        // 11 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_DOUBLEADD_64_2 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_8 3, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 11*8], Q7

        // 10 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_2 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_8 2, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 10*8], Q7

        // 9 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_DOUBLEADD_64_8 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 9*8], Q7

        // 8 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_8 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 8*8], Q7

        // 7 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_DOUBLEADD_64_2 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_4 3, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 7*8], Q7

        // 6 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_2 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        SQR_DOUBLEADD_64_4 2, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 6*8], Q7

        // 5 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_DOUBLEADD_64_4 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 5*8], Q7

        // 4 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_4 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 4*8], Q7

        // 3 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        SQR_DOUBLEADD_64_2 1, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 3*8], Q7

        // 2 iterations
        xor     Q7, Q7
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64_2 0, Q4, Q5, Q0, QH, Q6, Q7, Q8
        mov     [Q5 + 2*8], Q7

        // 1 iterations
        xor     Q8, Q8
        SQR_SIZE_SPECIFIC_INIT Q1, Q9, Q4, Q5, Q6
        SQR_DOUBLEADD_64 0, Q4, Q5, Q0, QH, Q6, Q8, Q7
        mov     [Q5 + 8], Q7

        xor     QH, QH
        mov     [Q5 + 16], QH       // Final word = 0


        ////////////////////////////////////////////////////////////////
        // Second Pass - Shifting all results 1 bit left
        ////////////////////////////////////////////////////////////////

        xor     Q0, Q0              // carry flag = 0
        mov     Q5, Q3              // pDst pointer

        SQR_SHIFT_LEFT 0, Q0, Q5
        SQR_SHIFT_LEFT 1, Q0, Q5
        SQR_SHIFT_LEFT 2, Q0, Q5
        SQR_SHIFT_LEFT 3, Q0, Q5

        SQR_SHIFT_LEFT 4, Q0, Q5
        SQR_SHIFT_LEFT 5, Q0, Q5
        SQR_SHIFT_LEFT 6, Q0, Q5
        SQR_SHIFT_LEFT 7, Q0, Q5

        SQR_SHIFT_LEFT 8, Q0, Q5
        SQR_SHIFT_LEFT 9, Q0, Q5
        SQR_SHIFT_LEFT 10, Q0, Q5
        SQR_SHIFT_LEFT 11, Q0, Q5

        SQR_SHIFT_LEFT 12, Q0, Q5
        SQR_SHIFT_LEFT 13, Q0, Q5
        SQR_SHIFT_LEFT 14, Q0, Q5
        SQR_SHIFT_LEFT 15, Q0, Q5

        SQR_SHIFT_LEFT 16, Q0, Q5
        SQR_SHIFT_LEFT 17, Q0, Q5
        SQR_SHIFT_LEFT 18, Q0, Q5
        SQR_SHIFT_LEFT 19, Q0, Q5

        SQR_SHIFT_LEFT 20, Q0, Q5
        SQR_SHIFT_LEFT 21, Q0, Q5
        SQR_SHIFT_LEFT 22, Q0, Q5
        SQR_SHIFT_LEFT 23, Q0, Q5

        SQR_SHIFT_LEFT 24, Q0, Q5
        SQR_SHIFT_LEFT 25, Q0, Q5
        SQR_SHIFT_LEFT 26, Q0, Q5
        SQR_SHIFT_LEFT 27, Q0, Q5

        SQR_SHIFT_LEFT 28, Q0, Q5
        SQR_SHIFT_LEFT 29, Q0, Q5
        SQR_SHIFT_LEFT 30, Q0, Q5
        SQR_SHIFT_LEFT 31, Q0, Q5

        //////////////////////////////////////////////////////////////////////////////
        // Third Pass - Adding the squares on the even columns and propagating the sum
        //////////////////////////////////////////////////////////////////////////////

        mov     Q1, [rsp + 48 /*MEMSLOT0*/]  // Q1 = pSrc
        xor     Q7, Q7

        SQR_DIAGONAL_PROP 0, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 1, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 2, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 3, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 4, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 5, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 6, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 7, Q1, Q3, Q0, QH, Q7

        SQR_DIAGONAL_PROP 8, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 9, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 10, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 11, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 12, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 13, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 14, Q1, Q3, Q0, QH, Q7
        SQR_DIAGONAL_PROP 15, Q1, Q3, Q0, QH, Q7


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

//VOID
//SymCryptFdefMontgomeryReduce1024Asm(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _Inout_                         PUINT32                 pSrc,
//    _Out_                           PUINT32                 pDst )

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
PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce1024Asm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce1024Asm)
.def PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce1024Asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdefMontgomeryReduce1024Asm)

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
        mov     Q5, [Q1 + SymCryptModulusInv64OffsetAmd64]      // inv64

        lea     Q1, [Q1 + SymCryptModulusValueOffsetAmd64]      // modulus value

        mov     D12, D4         // outer loop counter
        shl     D12, 3          // D12 is in words

        xor     D9, D9

        // General register allocations
        // Q0 = multiply result
        // QH = multiply result
        // Q1 = pointer to modulus value
        // Q2 = pSrc (updated in outer loop)
        // Q3 = pDst
        // D4 = nDigits
        // Q5 = pmMod->inv64
        // Q6 = multiplier in inner loop
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q9 = carry out from last word of previous loop iteration
        // Q10 = running pointer in Src
        // Q11 = running pointer in Mod
        // D12 = loop counter

ALIGN(16)

SymCryptFdefMontgomeryReduce1024AsmOuterLoop:

        // start decoder with a few simple instructions, including at least one that requires
        // a uop execution and is on the critical path

        mov     Q6, [Q2]                      // fetch word of Src we want to set to zero
        mov     Q11, Q2
        mov     Q10, Q1

        imul    Q6, Q5                        // lower word is same for signed & unsigned multiply

        xor     D7, D7

        // Q0 = mul scratch
        // QH = mul scratch
        // Q1 = pointer to modulus value
        // Q6 = multiplier
        // Q7 = carry for even words (64 bits)
        // Q8 = carry for odd words (64 bits)
        // Q10  = running ptr to modulus
        // Q11 = running ptr to input/scratch
        // D12 = outer loop counter (words)

        MULT_DOUBLEADD_128 0, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 2, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 4, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 6, Q10, Q11, Q0, QH, Q6, Q7, Q8

        MULT_DOUBLEADD_128 8, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 10, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 12, Q10, Q11, Q0, QH, Q6, Q7, Q8
        MULT_DOUBLEADD_128 14, Q10, Q11, Q0, QH, Q6, Q7, Q8

        lea     Q11,[Q11 + 128]

        add     Q7, Q9
        mov     D9, 0
        adc     Q9, 0
        add     Q7, [Q11]
        adc     Q9, 0
        mov     [Q11], Q7

        lea     Q2,[Q2 + 8]

        dec     D12
        jnz     SymCryptFdefMontgomeryReduce1024AsmOuterLoop

        //
        // Most of the work is done - now all that is left is subtract the modulus if it is smaller than the result
        //

        // First we compute the pSrc result minus the modulus into the destination
        mov     D12, D4         // loop ctr
        mov     Q11, Q2         // pSrc
        mov     Q10, Q1         // pMod
        mov     Q7, Q3          // pDst

        // Cy = 0 because the last 'adc Q9,0' resulted in 0, 1, or 2

ALIGN(16)

SymCryptFdefMontgomeryReduce1024AsmSubLoop:
        mov     Q0,[Q11]
        sbb     Q0,[Q10]
        mov     [Q7], Q0

        mov     Q0,[Q11 + 8]
        sbb     Q0,[Q10 + 8]
        mov     [Q7 + 8], Q0

        mov     Q0,[Q11 + 16]
        sbb     Q0,[Q10 + 16]
        mov     [Q7 + 16], Q0

        mov     Q0,[Q11 + 24]
        sbb     Q0,[Q10 + 24]
        mov     [Q7 + 24], Q0

        mov     Q0,[Q11 + 32]
        sbb     Q0,[Q10 + 32]
        mov     [Q7 + 32], Q0

        mov     Q0,[Q11 + 40]
        sbb     Q0,[Q10 + 40]
        mov     [Q7 + 40], Q0

        mov     Q0,[Q11 + 48]
        sbb     Q0,[Q10 + 48]
        mov     [Q7 + 48], Q0

        mov     Q0,[Q11 + 56]
        sbb     Q0,[Q10 + 56]
        mov     [Q7 + 56], Q0

        lea     Q11,[Q11 + 64]
        lea     Q10,[Q10 + 64]
        lea     Q7,[Q7 + 64]

        dec     D12
        jnz     SymCryptFdefMontgomeryReduce1024AsmSubLoop

        // Finally a masked copy from pSrc to pDst
        // copy if: Q9 == 0 && Cy = 1
        sbb     D9, 0

        movd    xmm0, D9            // xmm0[0] = mask
        pcmpeqd xmm1, xmm1          // xmm1 = ff...ff
        pshufd  xmm0, xmm0, 0       // xmm0[0..3] = mask
        pxor    xmm1, xmm0          // xmm1 = not Mask

ALIGN(16)

SymCryptFdefMontgomeryReduce1024AsmMaskedCopyLoop:
        movdqa  xmm2, [Q2]          // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3]          // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3], xmm2

        movdqa  xmm2, [Q2 + 16]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 16]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 16], xmm2

        movdqa  xmm2, [Q2 + 32]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 32]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 32], xmm2

        movdqa  xmm2, [Q2 + 48]     // xmm2 = pSrc[0]
        movdqa  xmm3, [Q3 + 48]     // xmm3 = pDst[0]
        pand    xmm2, xmm0          //
        pand    xmm3, xmm1          //
        por     xmm2, xmm3
        movdqa  [Q3 + 48], xmm2

        // Move on to the next digit
        lea     Q2,[Q2 + 64]
        lea     Q3,[Q3 + 64]

        dec     D4
        jnz     SymCryptFdefMontgomeryReduce1024AsmMaskedCopyLoop


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
#endif

FILE_END()
