//
//  fdef369_asm.symcryptasm   Assembler code for large integer arithmetic in the default data format
//  Expresses asm in a generic enough way to enable generation of MASM and GAS using the
//  symcryptasm_processor.py script and C preprocessor
//
// This file contains alternative routines that pretend that each digit is only 3 words.
// This gets used if the number is 1, 2, 3, 5, 6, or 9 digits long.
// The immediate advantage is that it improves EC performance on 192, 384, and 521-bit curves.
//
// Most of this code is a direct copy of the default code.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "symcryptasm_shared.cppasm"

// A digit consists of 3 words of 64 bits each

//UINT32
//SYMCRYPT_CALL
//SymCryptFdef369RawAddAsm(
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src2,
//    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
//                                                            UINT32      nDigits )

#define X_0 x0
#define W_0 w0
#define X_1 x1
#define W_1 w1
#define X_2 x2
#define W_2 w2
#define X_3 x3
#define W_3 w3
#define X_4 x4
#define W_4 w4
#define X_5 x5
#define W_5 w5
#define X_6 x6
#define W_6 w6
#define X_7 x7
#define W_7 w7
PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369RawAddAsm)
    .seh_endprologue

    ldp     X_4, X_6, [X_0]         // Load two words of pSrc1
    ldp     X_5, X_7, [X_1]         // Load two words of pSrc2
    adds    X_4, X_4, X_5
    adcs    X_6, X_6, X_7
    stp     X_4, X_6, [X_2]         // Store the result in the destination

    ldr     X_4, [X_0, #16]         // Load one word of pSrc1
    sub     X_3, X_3, #1            // Decrement the digit count by one
    ldr     X_5, [X_1, #16]         // Load one word of pSrc2
    adcs    X_4, X_4, X_5
    str     X_4, [X_2, #16]         // Store the result in the destination

    cbz     X_3, SymCryptFdef369RawAddAsmEnd

LABEL(SymCryptFdef369RawAddAsmLoop)
    // carry is in the carry flag
    // only update pointers to srcs and destination once per loop to reduce uops and dependencies
    ldp     X_4, X_6, [X_0, #24]!   // Load two words of pSrc1
    ldp     X_5, X_7, [X_1, #24]!   // Load two words of pSrc2
    adcs    X_4, X_4, X_5
    adcs    X_6, X_6, X_7
    stp     X_4, X_6, [X_2, #24]!   // Store the result in the destination

    ldr     X_4, [X_0, #16]         // Load one word of pSrc1
    sub     X_3, X_3, #1            // Decrement the digit count by one
    ldr     X_5, [X_1, #16]         // Load one word of pSrc2
    adcs    X_4, X_4, X_5
    str     X_4, [X_2, #16]         // Store the result in the destination

    cbnz    X_3, SymCryptFdef369RawAddAsmLoop

    ALIGN(4)
LABEL(SymCryptFdef369RawAddAsmEnd)
    cset    X_0, cs                 // Set the return value equal to the carry

    ret
.seh_endproc
#undef X_0
#undef W_0
#undef X_1
#undef W_1
#undef X_2
#undef W_2
#undef X_3
#undef W_3
#undef X_4
#undef W_4
#undef X_5
#undef W_5
#undef X_6
#undef W_6
#undef X_7
#undef W_7

//UINT32
//SYMCRYPT_CALL
//SymCryptFdef369RawSubAsm(
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
//    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
//                                                            UINT32      nDigits )

#define X_0 x0
#define W_0 w0
#define X_1 x1
#define W_1 w1
#define X_2 x2
#define W_2 w2
#define X_3 x3
#define W_3 w3
#define X_4 x4
#define W_4 w4
#define X_5 x5
#define W_5 w5
#define X_6 x6
#define W_6 w6
#define X_7 x7
#define W_7 w7
PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369RawSubAsm)
    .seh_endprologue

    ldp     X_4, X_6, [X_0]         // Load two words of pSrc1
    ldp     X_5, X_7, [X_1]         // Load two words of pSrc2
    subs    X_4, X_4, X_5
    sbcs    X_6, X_6, X_7
    stp     X_4, X_6, [X_2]         // Store the result in the destination

    ldr     X_4, [X_0, #16]         // Load one word of pSrc1
    sub     X_3, X_3, #1            // Decrement the digit count by one
    ldr     X_5, [X_1, #16]         // Load one word of pSrc2
    sbcs    X_4, X_4, X_5
    str     X_4, [X_2, #16]         // Store the result in the destination

    cbz     X_3, SymCryptFdef369RawSubAsmEnd

LABEL(SymCryptFdef369RawSubAsmLoop)
    // borrow is in the carry flag (flipped)
    // only update pointers to srcs and destination once per loop to reduce uops and dependencies
    ldp     X_4, X_6, [X_0, #24]!   // Load two words of pSrc1
    ldp     X_5, X_7, [X_1, #24]!   // Load two words of pSrc2
    sbcs    X_4, X_4, X_5
    sbcs    X_6, X_6, X_7
    stp     X_4, X_6, [X_2, #24]!   // Store the result in the destination

    ldr     X_4, [X_0, #16]         // Load one word of pSrc1
    sub     X_3, X_3, #1            // Decrement the digit count by one
    ldr     X_5, [X_1, #16]         // Load one word of pSrc2
    sbcs    X_4, X_4, X_5
    str     X_4, [X_2, #16]         // Store the result in the destination

    cbnz    X_3, SymCryptFdef369RawSubAsmLoop

    ALIGN(4)
LABEL(SymCryptFdef369RawSubAsmEnd)
    cset    X_0, cc                 // If the carry is clear (borrow), set the return value to 1

    ret
.seh_endproc
#undef X_0
#undef W_0
#undef X_1
#undef W_1
#undef X_2
#undef W_2
#undef X_3
#undef W_3
#undef X_4
#undef W_4
#undef X_5
#undef W_5
#undef X_6
#undef W_6
#undef X_7
#undef W_7

//VOID
//SYMCRYPT_CALL
//SymCryptFdef369MaskedCopyAsm(
//    _In_reads_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )        PCBYTE      pbSrc,
//    _Inout_updates_bytes_( nDigits*SYMCRYPT_FDEF_DIGIT_SIZE )   PBYTE       pbDst,
//                                                                UINT32      nDigits,
//                                                                UINT32      mask )

#define X_0 x0
#define W_0 w0
#define X_1 x1
#define W_1 w1
#define X_2 x2
#define W_2 w2
#define X_3 x3
#define W_3 w3
#define X_4 x4
#define W_4 w4
#define X_5 x5
#define W_5 w5
#define X_6 x6
#define W_6 w6
PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369MaskedCopyAsm)
    .seh_endprologue

    subs    xzr, xzr, X_3           // If (X_3 > 0) clear the carry flag (i.e. borrow)

    ldp     X_3, X_5, [X_0]         // Load two words of the source
    ldp     X_4, X_6, [X_1]         // Load two words of the destination
    csel    X_3, X_3, X_4, cc       // If the carry is clear, select the source operand
    csel    X_5, X_5, X_6, cc
    stp     X_3, X_5, [X_1]         // Store the two words in the destination

    ldr     X_3, [X_0, #16]         // Load one word of the source
    sub     X_2, X_2, #1            // Decrement the digit count by one
    ldr     X_4, [X_1, #16]         // Load one word of the destination
    csel    X_3, X_3, X_4, cc
    str     X_3, [X_1, #16]         // Store the one word in the destination

    cbz     X_2, SymCryptFdef369MaskedCopyAsmEnd

LABEL(SymCryptFdef369MaskedCopyAsmLoop)
    ldp     X_3, X_5, [X_0, #24]!   // Load two words of the source
    ldp     X_4, X_6, [X_1, #24]!   // Load two words of the destination
    csel    X_3, X_3, X_4, cc       // If the carry is clear, select the source operand
    csel    X_5, X_5, X_6, cc
    stp     X_3, X_5, [X_1]         // Store the two words in the destination

    ldr     X_3, [X_0, #16]         // Load one word of the source
    sub     X_2, X_2, #1            // Decrement the digit count by one
    ldr     X_4, [X_1, #16]         // Load one word of the destination
    csel    X_3, X_3, X_4, cc
    str     X_3, [X_1, #16]         // Store the one word in the destination

    cbnz    X_2, SymCryptFdef369MaskedCopyAsmLoop

LABEL(SymCryptFdef369MaskedCopyAsmEnd)
    // Done, no return value

    ret
.seh_endproc
#undef X_0
#undef W_0
#undef X_1
#undef W_1
#undef X_2
#undef W_2
#undef X_3
#undef W_3
#undef X_4
#undef W_4
#undef X_5
#undef W_5
#undef X_6
#undef W_6

//VOID
//SYMCRYPT_CALL
//SymCryptFdef369RawMulAsm(
//    _In_reads_(nDigits1*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc1,
//                                                                    UINT32      nDigits1,
//    _In_reads_(nDigits2*SYMCRYPT_FDEF_DIGIT_NUINT32)                PCUINT32    pSrc2,
//                                                                    UINT32      nDigits2,
//    _Out_writes_((nDigits1+nDigits2)*SYMCRYPT_FDEF_DIGIT_NUINT32)   PUINT32     pDst )
//
// Basic structure:
//   for each word in Src1:
//       Dst += Src2 * word
//
// Register assignments
//       X_0  = pSrc1 (moving forward one word every outer loop)
//       X_1  = word count of pSrc1
//       X_2  = pSrc2 (moving forward one *digit* every inner loop)
//       X_3  = digit count of pSrc2 and pDst
//       X_4  = pDst (moving forward one *digit* every inner loop)
//       X_5  = Stored pDst (moving forward one word every outer loop)
//       X_6  = Current word loaded from pSrc1
//       X_7, X_8   = Current words loaded in pairs from pSrc2
//       X_9, X_10  = Current words loaded in pairs from pDst
//       X_11, X_12 = Scratch registers for holding the results of multiplies
//       X_13 = Stored pSrc2
//       X_14 = Stored digit count of pSrc2
//       X_15 = Scratch register for holding the results of multiplies

#define X_0 x0
#define W_0 w0
#define X_1 x1
#define W_1 w1
#define X_2 x2
#define W_2 w2
#define X_3 x3
#define W_3 w3
#define X_4 x4
#define W_4 w4
#define X_5 x5
#define W_5 w5
#define X_6 x6
#define W_6 w6
#define X_7 x7
#define W_7 w7
#define X_8 x8
#define W_8 w8
#define X_9 x9
#define W_9 w9
#define X_10 x10
#define W_10 w10
#define X_11 x11
#define W_11 w11
#define X_12 x12
#define W_12 w12
#define X_13 x13
#define W_13 w13
#define X_14 x14
#define W_14 w14
#define X_15 x15
#define W_15 w15
PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369RawMulAsm)
    .seh_endprologue

    add     X_1, X_1, X_1, LSL #1       // Calculate word count (X_1 * 3)

    sub     X_2, X_2, #24               // offset pSrc2 so we can use pre-increment form of loads
    sub     X_4, X_4, #24               // offset pDst so we can use pre-increment form of loads

    mov     X_5, X_4                    // store pDst
    mov     X_13, X_2                   // store pSrc2
    mov     X_14, X_3                   // store nDigits2 for later

    //
    // First iteration of main loop (no adding of previous values from pDst)
    //
    ands    X_12, X_12, xzr             // Clearing the carry flag and setting X_12 = 0
    ldr     X_6, [X_0]                  // load the first word from pSrc1

LABEL(SymCryptFdef369RawMulAsmLoopInner1)
    sub     X_3, X_3, #1                // move one digit up

    ldp     X_7, X_8, [X_2, #24]!       // load 2 words from pSrc2

    mul     X_11, X_6, X_7              // Bits <63:0> of pSrc1[0]*pSrc2[j]
    adcs    X_11, X_11, X_12            // Adding the previous word (if there was a carry from the last addition it is added)
    umulh   X_12, X_6, X_7              // Bits <127:64> of pSrc1[0]*pSrc2[j]

    mul     X_15, X_6, X_8              // Bits <63:0> of pSrc1[0]*pSrc2[j+1]
    adcs    X_15, X_15, X_12            // Adding the previous word (if there was a carry from the last addition it is added)
    umulh   X_12, X_6, X_8              // Bits <127:64> of pSrc1[0]*pSrc2[j+1]

    stp     X_11, X_15, [X_4, #24]!     // Store to destination
    ldr     X_7, [X_2, #16]             // load 1 word from pSrc2

    mul     X_11, X_6, X_7              // Bits <63:0> of pSrc1[0]*pSrc2[j+2]
    adcs    X_11, X_11, X_12            // Adding the previous word (if there was a carry from the last addition it is added)
    umulh   X_12, X_6, X_7              // Bits <127:64> of pSrc1[0]*pSrc2[j+2]

    str     X_11, [X_4, #16]            // Store to destination

    cbnz    X_3, SymCryptFdef369RawMulAsmLoopInner1

    adc     X_12, X_12, xzr             // Store the next word into the destination (with the carry if any)
    str     X_12, [X_4, #24]

    sub     X_1, X_1, #1                // move one word up
    add     X_0, X_0, #8                // move start of pSrc1 one word up
    add     X_5, X_5, #8                // move start of pDst one word up

    //
    // MAIN LOOP
    //
LABEL(SymCryptFdef369RawMulAsmLoopOuter)
    mov     X_3, X_14                   // set nDigits2
    mov     X_2, X_13                   // set pSrc2
    mov     X_4, X_5                    // set pDst

    ands    X_12, X_12, xzr             // Clearing the carry flag and setting X_12 = 0
    ldr     X_6, [X_0]                  // load the next word from pSrc1

LABEL(SymCryptFdef369RawMulAsmLoopInner)
    sub     X_3, X_3, #1                // move one digit up

    ldp     X_7, X_8, [X_2, #24]!       // load 2 words from pSrc2
    ldp     X_9, X_10, [X_4, #24]!      // load 2 words from pDst

    adcs    X_9, X_9, X_12              // Adding the previous word (if there was a carry from the last addition it is added)
    umulh   X_11, X_6, X_7              // Bits <127:64> of pSrc1[i]*pSrc2[j]
    adcs    X_10, X_11, X_10            // Adding the previous word (if there was a carry from the last addition it is added)
    umulh   X_12, X_6, X_8              // Bits <127:64> of pSrc1[i]*pSrc2[j+1]
    adc     X_12, X_12, xzr             // Add the carry if any and don't update the flags

    mul     X_11, X_6, X_7              // Bits <63:0> of pSrc1[i]*pSrc2[j]
    adds    X_9, X_9, X_11              // add the word from the destination and update the flags (this can overflow)
    mul     X_11, X_6, X_8              // Bits <63:0> of pSrc1[i]*pSrc2[j+1]
    adcs    X_10, X_10, X_11            // add the word from the destination and update the flags (this can overflow)

    stp     X_9, X_10, [X_4]            // Store to destination

    ldr     X_7, [X_2, #16]             // load 1 word from pSrc2
    ldr     X_9, [X_4, #16]             // load 1 word from pDst

    adcs    X_9, X_9, X_12              // Adding the previous word (if there was a carry from the last addition it is added)
    umulh   X_12, X_6, X_7              // Bits <127:64> of pSrc1[i]*pSrc2[j+2]
    adc     X_12, X_12, xzr             // Add the carry if any and don't update the flags

    mul     X_11, X_6, X_7              // Bits <63:0> of pSrc1[i]*pSrc2[j+2]
    adds    X_9, X_9, X_11              // add the word from the destination and update the flags (this can overflow)

    str     X_9, [X_4, #16]             // Store to destination

    cbnz    X_3, SymCryptFdef369RawMulAsmLoopInner

    adc     X_12, X_12, xzr             // Store the next word into the destination (with the carry if any)
    str     X_12, [X_4, #24]

    subs    X_1, X_1, #1                // move one word up
    add     X_0, X_0, #8                // move start of pSrc1 one word up
    add     X_5, X_5, #8                // move start of pDst one word up

    bne     SymCryptFdef369RawMulAsmLoopOuter

    // Done, no return value

    ret
.seh_endproc
#undef X_0
#undef W_0
#undef X_1
#undef W_1
#undef X_2
#undef W_2
#undef X_3
#undef W_3
#undef X_4
#undef W_4
#undef X_5
#undef W_5
#undef X_6
#undef W_6
#undef X_7
#undef W_7
#undef X_8
#undef W_8
#undef X_9
#undef W_9
#undef X_10
#undef W_10
#undef X_11
#undef W_11
#undef X_12
#undef W_12
#undef X_13
#undef W_13
#undef X_14
#undef W_14
#undef X_15
#undef W_15


//VOID
//SYMCRYPT_CALL
//SymCryptFdef369MontgomeryReduceAsm(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _Inout_                         PUINT32                 pSrc,
//    _Out_                           PUINT32                 pDst )
//
// Register assignments
//       X_0  = pMod (moving forward one *digit* every inner loop)
//       X_1  = pSrc (moving forward one *digit* every inner loop)
//       X_2  = pDst (used only in the end for subtract / result)
//       X_3  = digit count of pSrc and pMod
//       X_4  = word count of pSrc
//       X_5  = Inv64 of the modulus
//       X_6  = m = pSrc[i]*Inv64
//       X_7  = hc = high carry variable
//       X_8, X_9   = Current words loaded in pairs from pSrc
//       X_10, X_11 = Current words loaded in pairs from pMod
//       X_12, X_13 = c variable = "128-bit" register to hold the result of multiplies
//                  It is flipped between [X_12:X_13] and [X_13:X_12] instead of doing c>>=64
//       X_14 = Temporary intermediate result
//       X_15 = Stored digit count of pSrc
//       X_16 = Stored pMod pointer
//       X_17 = Stored pSrc pointer (moving forward one word every outer loop)

#define X_0 x0
#define W_0 w0
#define X_1 x1
#define W_1 w1
#define X_2 x2
#define W_2 w2
#define X_3 x3
#define W_3 w3
#define X_4 x4
#define W_4 w4
#define X_5 x5
#define W_5 w5
#define X_6 x6
#define W_6 w6
#define X_7 x7
#define W_7 w7
#define X_8 x8
#define W_8 w8
#define X_9 x9
#define W_9 w9
#define X_10 x10
#define W_10 w10
#define X_11 x11
#define W_11 w11
#define X_12 x12
#define W_12 w12
#define X_13 x13
#define W_13 w13
#define X_14 x14
#define W_14 w14
#define X_15 x15
#define W_15 w15
#define X_16 x16
#define W_16 w16
#define X_17 x17
#define W_17 w17
PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm):
.globl PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm)
.def PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptFdef369MontgomeryReduceAsm)
    .seh_endprologue

    ldr     W_3, [X_0, #SymCryptModulusNdigitsOffsetArm64]  // # of Digits
    ldr     X_5, [X_0, #SymCryptModulusInv64OffsetArm64]    // Inv64 of modulus
    add     X_0, X_0, #SymCryptModulusValueOffsetArm64      // pMod

    add     X_4, X_3, X_3, LSL #1       // Calculate word count (X_3 * 3)

    sub     X_0, X_0, #24               // offset pMod so we can use pre-increment form of loads
    sub     X_1, X_1, #24               // offset pSrc so we can use pre-increment form of loads
    sub     X_2, X_2, #24               // offset pDst so we can use pre-increment form of loads

    mov     X_15, X_3                   // Store the digit count for later
    mov     X_16, X_0                   // Store the pMod pointer
    mov     X_17, X_1                   // Store the pSrc pointer

    mov     X_7, xzr                    // Set hc to 0

    //
    // Main loop
    //
LABEL(SymCryptFdef369MontgomeryReduceAsmOuter)
    ldr     X_8, [X_1, #24]             // Load 1 word from pSrc
    mul     X_6, X_8, X_5               // <63:0> bits of pSrc[i]*Inv64 = m

    mov     X_12, xzr                   // Set c to 0

LABEL(SymCryptFdef369MontgomeryReduceAsmInner)
    ldp     X_10, X_11, [X_0, #24]!     // pMod[j]
    ldp     X_8, X_9, [X_1, #24]!       // pSrc[j]

    mul     X_14, X_6, X_10             // <63:0> of pMod[j]*m
    adds    X_14, X_14, X_8             // Adding pSrc[j]
    umulh   X_13, X_6, X_10             // <127:64> of pMod[j]*m
    adc     X_13, X_13, xzr             // Add the carry if any (***)
    adds    X_12, X_12, X_14            // Add the lower bits of c
    adc     X_13, X_13, xzr             // Add the carry if any (***)
    // ***: These cannot produce extra carry as the maximum is
    //      (2^64 - 1)*(2^64-1) + 2^64-1 + 2^64-1 = 2^128 - 1
    str     X_12, [X_1]                 // pSrc[j] = (UINT64) c4) c
    mov     X_12, X_13                  // c >>= 64

    mul     X_14, X_6, X_11             // <63:0> of pMod[j]*m
    adds    X_14, X_14, X_9             // Adding pSrc[j]
    umulh   X_13, X_6, X_11             // <127:64> of pMod[j]*m
    adc     X_13, X_13, xzr             // Add the carry if any (***)
    adds    X_12, X_12, X_14            // Add the lower bits of c
    adc     X_13, X_13, xzr             // Add the carry if any (***)
    str     X_12, [X_1, #8]             // pSrc[j] = (UINT64) c
    mov     X_12, X_13                  // c >>= 64

    ldr     X_10, [X_0, #16]            // pMod[j]
    ldr     X_8, [X_1, #16]             // pSrc[j]

    mul     X_14, X_6, X_10             // <63:0> of pMod[j]*m
    adds    X_14, X_14, X_8             // Adding pSrc[j]
    umulh   X_13, X_6, X_10             // <127:64> of pMod[j]*m
    adc     X_13, X_13, xzr             // Add the carry if any (***)
    adds    X_12, X_12, X_14            // Add the lower bits of c
    adc     X_13, X_13, xzr             // Add the carry if any (***)
    str     X_12, [X_1, #16]            // pSrc[j] = (UINT64) c4) c
    mov     X_12, X_13                  // c >>= 64

    subs    X_3, X_3, #1                // Move one digit up
    bne     SymCryptFdef369MontgomeryReduceAsmInner

    ldr     X_8, [X_1, #24]             // pSrc[nWords]
    adds    X_12, X_12, X_8             // c + pSrc[nWords]
    adc     X_13, xzr, xzr              // Add the carry if any

    adds    X_12, X_12, X_7             // c + pSrc[nWords] + hc
    adc     X_7, X_13, xzr              // Add the carry if any and store into hc

    str     X_12, [X_1, #24]            // pSrc[nWords] = c

    subs    X_4, X_4, #1                // Move one word up

    add     X_17, X_17, #8              // Move stored pSrc pointer one word up
    mov     X_0, X_16                   // Restore pMod pointer
    mov     X_1, X_17                   // Restore pSrc pointer

    mov     X_3, X_15                   // Restore the digit counter

    bne     SymCryptFdef369MontgomeryReduceAsmOuter

    //
    // Subtraction
    //

    mov     X_14, X_2               // Store pDst pointer

    // Prepare the pointers for subtract
    mov     X_0, X_17               // pSrc
    mov     X_1, X_16               // pMod

    mov     X_10, X_7               // X_10 = hc
    mov     X_3, X_15               // Restore the digit counter
    subs    X_4, X_4, X_4           // Set the carry flag (i.e. no borrow)

LABEL(SymCryptFdef369MontgomeryReduceRawSubAsmLoop)
    sub     X_3, X_3, #1            // Decrement the digit count by one
    // borrow is in the carry flag (flipped)

    ldp     X_4, X_6, [X_0, #24]!   // Load two words of pSrc1
    ldp     X_5, X_7, [X_1, #24]!   // Load two words of pSrc2
    sbcs    X_4, X_4, X_5
    sbcs    X_6, X_6, X_7
    stp     X_4, X_6, [X_2, #24]!   // Store the result in the destination

    ldr     X_4, [X_0, #16]         // Load one word of pSrc1
    ldr     X_5, [X_1, #16]         // Load one word of pSrc2
    sbcs    X_4, X_4, X_5
    str     X_4, [X_2, #16]         // Store the result in the destination

    cbnz    X_3, SymCryptFdef369MontgomeryReduceRawSubAsmLoop

    cset    X_0, cc                 // If the carry is clear (borrow), set the return value to 1

    orr     X_11, X_10, X_0         // X_11 = hc|d

    // Prepare the pointers for masked copy
    mov     X_0, X_17               // pSrc
    mov     X_1, X_14               // pDst

    mov     X_2, X_15               // Restore the digit counter
    subs    X_4, X_10, X_11         // If (X_11 > X_10) clear the carry flag (i.e. borrow)

LABEL(SymCryptFdef369MontgomeryReduceMaskedCopyAsmLoop)
    sub     X_2, X_2, #1            // decrement the digit count by one

    ldp     X_4, X_6, [X_0, #24]!   // Load two words of the source
    ldp     X_5, X_7, [X_1, #24]!   // Load two words of the destination
    csel    X_4, X_4, X_5, cc       // If the carry is clear, select the source operands
    csel    X_6, X_6, X_7, cc
    stp     X_4, X_6, [X_1]         // Store the two words in the destination

    ldr     X_4, [X_0, #16]         // Load one word of the source
    ldr     X_5, [X_1, #16]         // Load one word of the destination
    csel    X_4, X_4, X_5, cc       // If the carry is clear, select the source operands
    str     X_4, [X_1, #16]         // Store the one word in the destination

    cbnz    X_2, SymCryptFdef369MontgomeryReduceMaskedCopyAsmLoop

    // Done, no return value

    ret
.seh_endproc
#undef X_0
#undef W_0
#undef X_1
#undef W_1
#undef X_2
#undef W_2
#undef X_3
#undef W_3
#undef X_4
#undef W_4
#undef X_5
#undef W_5
#undef X_6
#undef W_6
#undef X_7
#undef W_7
#undef X_8
#undef W_8
#undef X_9
#undef W_9
#undef X_10
#undef W_10
#undef X_11
#undef W_11
#undef X_12
#undef W_12
#undef X_13
#undef W_13
#undef X_14
#undef W_14
#undef X_15
#undef W_15
#undef X_16
#undef W_16
#undef X_17
#undef W_17

    FILE_END()
