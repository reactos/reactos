//
//  fdef_asm.cppasm   Assembler code for large integer arithmetic in the default data format for the arm architecture
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

// #include "ksarm.h"

#include "symcryptasm_shared.cppasm"

// A digit consists of 4 words of 32 bits each

//UINT32
//SYMCRYPT_CALL
// SymCryptFdefRawAdd(
//   _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc1,
//   _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    pSrc2,
//   _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     pDst,
//                                                           UINT32      nDigits )
//
// Initial inputs to registers:
//       pSrc1       -> r0
//       pSrc2       -> r1
//       pDst        -> r2
//       nDigits     -> r3

//    LEAF_ENTRY SymCryptFdefRawAddAsm
SymCryptFdefRawAddAsm:
.globl SymCryptFdefRawAddAsm
.def SymCryptFdefRawAddAsm; .scl 2; .type 32; .endef
.seh_proc SymCryptFdefRawAddAsm
push {r4,r5,r6,r7,r8,r9,lr}
.seh_save_regs_w {r4,r5,r6,r7,r8,r9,lr}
.seh_endprologue
//    push     {r4-r9, lr}

    neg     r3, r3                  // negate the digit count
    mov     r8, #0                  // carry = r8 = 0
    mov     r9, #0                  // r9 = 0

SymCryptFdefRawAddAsmLoop:
    rrxs    r8, r8                  // set the carry flag if bit[0] of r8 is set

    ldmia   r0!, {r4, r6}           // Load two words of pSrc1
    ldmia   r1!, {r5, r7}           // Load two words of pSrc2
    adcs    r4, r4, r5
    adcs    r6, r6, r7
    stmia   r2!, {r4, r6}           // Store the result in the destination

    ldmia   r0!, {r4, r6}           // Load two words of pSrc1
    ldmia   r1!, {r5, r7}           // Load two words of pSrc2
    adcs    r4, r4, r5
    adcs    r6, r6, r7
    stmia   r2!, {r4, r6}           // Store the result in the destination

    adc     r8, r9, r9              // r8 = 1 if the carry flag is set

    adds    r3, r3, #1              // Increment the digit count by one
    bne     SymCryptFdefRawAddAsmLoop

    mov     r0, r8                  // Set the return value equal to the carry

//    pop      {r4-r9, pc}

//    LEAF_END SymCryptFdefRawAddAsm
pop {r4,r5,r6,r7,r8,r9,pc}
.seh_endproc

//UINT32
//SYMCRYPT_CALL
//SymCryptFdefRawSub(
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src1,
//    _In_reads_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )   PCUINT32    Src2,
//    _Out_writes_bytes_(nDigits * SYMCRYPT_FDEF_DIGIT_SIZE ) PUINT32     Dst,
//                                                            UINT32      nDigits )
//
// Initial inputs to registers:
//       pSrc1       -> r0
//       pSrc2       -> r1
//       pDst        -> r2
//       nDigits     -> r3

//    LEAF_ENTRY SymCryptFdefRawSubAsm
SymCryptFdefRawSubAsm:
.globl SymCryptFdefRawSubAsm
.def SymCryptFdefRawSubAsm; .scl 2; .type 32; .endef
.seh_proc SymCryptFdefRawSubAsm
push {r4,r5,r6,r7,r8,r9,lr}
.seh_save_regs_w {r4,r5,r6,r7,r8,r9,lr}
.seh_endprologue
//    push     {r4-r9, lr}

    neg     r3, r3                  // negate the digit count
    mov     r8, #0                  // borrow = r8 = 0
    mov     r9, #0                  // r9 = 0

SymCryptFdefRawSubAsmLoop:
    subs    r8, r9, r8              // if r8>0 then the "borrow flag" is set

    ldmia   r0!, {r4, r6}           // Load two words of pSrc1
    ldmia   r1!, {r5, r7}           // Load two words of pSrc2
    sbcs    r4, r4, r5
    sbcs    r6, r6, r7
    stmia   r2!, {r4, r6}           // Store the result in the destination

    ldmia   r0!, {r4, r6}           // Load two words of pSrc1
    ldmia   r1!, {r5, r7}           // Load two words of pSrc2
    sbcs    r4, r4, r5
    sbcs    r6, r6, r7
    stmia   r2!, {r4, r6}           // Store the result in the destination

    sbc     r8, r9, r9              // If borrow=1, then r8 = -1 = 0xffffffff

    adds    r3, r3, #1              // Increment the digit count by one
    bne     SymCryptFdefRawSubAsmLoop

    and     r0, r8, #1             // If r8>0, set the return value to 1

//    pop      {r4-r9, pc}

//    LEAF_END SymCryptFdefRawSubAsm
pop {r4,r5,r6,r7,r8,r9,pc}
.seh_endproc

//VOID
//SYMCRYPT_CALL
//SymCryptFdefMaskedCopy(
//    _In_reads_bytes_( nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )      PCBYTE      pbSrc,
//    _InOut_writes_bytes_( nDigits * SYMCRYPT_FDEF_DIGIT_SIZE )  PBYTE       pbDst,
//                                                                UINT32      nDigits,
//                                                                UINT32      mask )

//    LEAF_ENTRY SymCryptFdefMaskedCopyAsm
SymCryptFdefMaskedCopyAsm:
.globl SymCryptFdefMaskedCopyAsm
.def SymCryptFdefMaskedCopyAsm; .scl 2; .type 32; .endef
.seh_proc SymCryptFdefMaskedCopyAsm
push {r4,r5,r6,r7,r8,r9,lr}
.seh_save_regs_w {r4,r5,r6,r7,r8,r9,lr}
.seh_endprologue
//    push     {r4-r9, lr}

    neg     r2, r2                  // negate the digit count
    mov     r9, #0                  // r9 = 0

    subs    r4, r9, r3              // If (r3 > 0) clear the carry flag (i.e. borrow)
    sbc     r3, r9, r9              // r3 = mask = 0xffffffff if the carry flag is clear
    // orn     r9, r9, r3              // r9 = NOT(MASK) = 0 if r3 = 0xffffffff
    orr     r9, r9, r3
    mvn     r9, r9

    mov     r8, r1                  // save the destination pointer

SymCryptFdefMaskedCopyAsmLoop:
    ldmia   r0!, {r4, r6}           // Load two words of the source
    ldmia   r1!, {r5, r7}           // Load two words of the destination
    and     r4, r4, r3
    and     r5, r5, r9
    orr     r4, r4, r5
    and     r6, r6, r3
    and     r7, r7, r9
    orr     r6, r6, r7
    stmia   r8!, {r4, r6}           // Store the two words in the destination

    ldmia   r0!, {r4, r6}           // Load two words of the source
    ldmia   r1!, {r5, r7}           // Load two words of the destination
    and     r4, r4, r3
    and     r5, r5, r9
    orr     r4, r4, r5
    and     r6, r6, r3
    and     r7, r7, r9
    orr     r6, r6, r7
    stmia   r8!, {r4, r6}           // Store the two words in the destination

    adds    r2, r2, #1              // Increment the digit count by one
    bne     SymCryptFdefMaskedCopyAsmLoop

    // Done, no return value

//    pop      {r4-r9, pc}

//    LEAF_END SymCryptFdefMaskedCopyAsm
pop {r4,r5,r6,r7,r8,r9,pc}
.seh_endproc

//VOID
//SYMCRYPT_CALL
//SymCryptFdefRawMul(
//    _In_reads_(nWords1)             PCUINT32    pSrc1,
//                                    UINT32      nDigits1,
//    _In_reads_(nWords2)             PCUINT32    pSrc2,
//                                    UINT32      nDigits2,
//    _Out_writes_(nWords1 + nWords2) PUINT32     pDst )
//
// Initial inputs to registers:
//       pSrc1       -> r0
//       nDigits1    -> r1
//       pSrc2       -> r2
//       nDigits2    -> r3
//       pDst        -> In the stack
//
// Basic structure:
//   for each 2 words in Src1:
//       Dst += Src2 * (2 words of Src1)
//
// Register assignments
//       r0  = pSrc1 (moving forward one word every outer loop)
//       r1  = negated word count of pSrc1
//       r2  = pSrc2 (moving forward one *digit* every inner loop)
//       r3  = negated digit count of pSrc2 and pDst
//       r4  = pDst (moving forward one *digit* every inner loop)
//       r5  = Stored pDst (moving forward one word every outer loop)
//       r6, r7  = Current words loaded from pSrc1
//       r8, r9  = Current words loaded from pSrc2
//       <r12:r11:r10> = "96-bit" sliding register to hold the result of multiplies
//
// Stack assignments
#define pSrc2 0               // Stored pSrc2 in stack
#define nDigits2 4               // Stored negated digit count of pSrc2 in stack


//    LEAF_ENTRY SymCryptFdefRawMulAsm
SymCryptFdefRawMulAsm:
.globl SymCryptFdefRawMulAsm
.def SymCryptFdefRawMulAsm; .scl 2; .type 32; .endef
.seh_proc SymCryptFdefRawMulAsm
push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
.seh_save_regs_w {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
.seh_endprologue
    // push         {r4-r12, lr}
    sub     sp, sp, #8

    lsl     r1, r1, #2                  // Calculate word count

    ldr     r4, [sp, #(8+4*10)]         // load pDst

    neg     r1, r1                      // negate nWords1
    neg     r3, r3                      // negate nDigits2

    mov     r5, r4                      // store pDst
    str     r2, [sp, #pSrc2]            // store pSrc2
    str     r3, [sp, #nDigits2]         // store -nDigits2 for later

    //
    // First iteration of main loop (no adding of previous values from pDst)
    //
    mov     r11, #0                     // Setting r11 = 0
    mov     r12, #0                     // and r12 = 0
    ldmia   r0!, {r6, r7}               // Load two words from pSrc1

SymCryptFdefRawMulAsmLoopInner1:

    adds    r3, r3, #1                  // move one digit up

    ldmia   r2!, {r8, r9}               // Load two words from pSrc2

    mov     r10, #0                     // Setting r10 = 0
    umaal   r10, r11, r6, r8            // <r11:r10> = r6 * r8 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r8            // <r12:r11> = r7 * r8 + r11

    mov     r10, #0                     // Setting r10 = 0
    umaal   r10, r11, r6, r9            // <r11:r10> = r6 * r9 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r9            // <r12:r11> = r7 * r9 + r11

    ldmia   r2!, {r8, r9}               // Load two words from pSrc2

    mov     r10, #0                     // Setting r10 = 0
    umaal   r10, r11, r6, r8            // <r11:r10> = r6 * r8 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r8            // <r12:r11> = r7 * r8 + r11

    mov     r10, #0                     // Setting r10 = 0
    umaal   r10, r11, r6, r9            // <r11:r10> = r6 * r9 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r9            // <r12:r11> = r7 * r9 + r11

    bne     SymCryptFdefRawMulAsmLoopInner1

    stmia   r4, {r11, r12}              // Store the top two words in the destination

    add     r1, r1, #2                  // move two words up
    add     r5, r5, #8                  // move start of pDst two words up

    //
    // MAIN LOOP
    //
SymCryptFdefRawMulAsmLoopOuter:
    ldr     r3, [sp, #nDigits2]         // set -nDigits2
    ldr     r2, [sp, #pSrc2]            // set pSrc2
    mov     r4, r5                      // set pDst

    mov     r11, #0                     // Setting r11 = 0
    mov     r12, #0                     // and r12 = 0
    ldmia   r0!, {r6, r7}               // Load two words from pSrc1

SymCryptFdefRawMulAsmLoopInner:

    adds    r3, r3, #1                  // move one digit up

    ldmia   r2!, {r8, r9}               // Load two words from pSrc2

    ldr     r10, [r4]                   // load 1 word from pDst
    umaal   r10, r11, r6, r8            // <r11:r10> = r6 * r8 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r8            // <r12:r11> = r7 * r8 + r11

    ldr     r10, [r4]                   // load 1 word from pDst
    umaal   r10, r11, r6, r9            // <r11:r10> = r6 * r9 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r9            // <r12:r11> = r7 * r9 + r11

    ldmia   r2!, {r8, r9}               // Load two words from pSrc2

    ldr     r10, [r4]                   // load 1 word from pDst
    umaal   r10, r11, r6, r8            // <r11:r10> = r6 * r8 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r8            // <r12:r11> = r7 * r8 + r11

    ldr     r10, [r4]                   // load 1 word from pDst
    umaal   r10, r11, r6, r9            // <r11:r10> = r6 * r9 + r10 + r11
    str     r10, [r4], #4               // Store to destination
    umaal   r11, r12, r7, r9            // <r12:r11> = r7 * r9 + r11

    bne     SymCryptFdefRawMulAsmLoopInner

    adds    r1, r1, #2                  // move two words up
    add     r5, r5, #8                  // move start of pDst two words up

    stmia   r4, {r11, r12}              // Store the top two words in the destination

    bne     SymCryptFdefRawMulAsmLoopOuter

    // Done, no return value

    add     sp, sp, #8
    // pop          {r4-r12, pc}

//    LEAF_END SymCryptFdefRawMulAsm
pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,pc}
.seh_endproc

    // Macro for the first loop of the first pass of RawSquareAsm.
    // It takes one word from the source, multiplies it with the mulword,
    // adds the high level word of the previous macro call, and stores it into
    // the destination.
    //
    // No word is taken from the destination// thus r10 is always set to 0.
    //
    // No carry flag is propagated from the previous macro call as the maximum is
    // (2^32-1)^2 + 2^32-1 = 2^64 - 2^32
.macro SQR_SINGLEADD_32  index

        mov     r10, #0
        ldr     r8, [r2, #4*\index]     // pSrc[i+j]

        umaal   r10, r11, r6, r8        // <r11:r10> = r6 * r8 + r10 + r11

        str     r10, [r4, #4*\index]    // Store to destination
.endm

    // Macro for the remaining loops of the first pass of RawSquareAsm.
    // The only difference to the above is that it also adds the word loaded
    // from the destination buffer.
    //
    // No carry flag is propagated from the previous macro call as the maximum is
    // (2^32-1)^2 + 2(2^32-1) = 2^64 - 1
.macro SQR_DOUBLEADD_32  index

        ldr     r8, [r2, #4*\index]     // pSrc[i+j]
        ldr     r10, [r4, #4*\index]    // pDst[2*(i+j)]

        umaal   r10, r11, r6, r8        // <r11:r10> = r6 * r8 + r10 + r11

        str     r10, [r4, #4*\index]    // Store to destination

.endm

    // Macro for the third pass loop of RawSquareAsm.
    // It takes one mulword from the source, squares it, and
    // adds it to the even columns of the destination. The carries are propagated
    // to the odd columns.
    //
    // Here we can have a (1-bit) carry to the next call because the maximum value for
    // a pair of columns is (2^32-1)^2+(2^64-1)+1 = 2^65 - 2^33 + 1 < 2^65 - 1
.macro SQR_DIAGONAL_PROP  index
        ldr     r6, [r0, #4*\index]     // mulword

        umull   r10, r11, r6, r6

        ldr     r8, [r4, #8*\index]     // Load
        ldr     r9, [r4, #8*\index + 4] // Load

        // Adding the square to the even column
        adcs    r10, r10, r8            // carry from previous and update the flags

        // Propagating the sum to the next column
        adcs    r11, r11, r9            // This can generate a carry

        str     r10, [r4, #8*\index]    // Store
        str     r11, [r4, #8*\index + 4]// Store
.endm

// VOID
// SYMCRYPT_CALL
// SymCryptFdefRawSquareAsm(
//   _In_reads_(nDigits*SYMCRYPT_FDEF_DIGIT_NUINT32)     PCUINT32    pSrc,
//                                                       UINT32      nDigits,
//   _Out_writes_(2*nWords)                              PUINT32     pDst )
//
// Initial inputs to registers:
//       pSrc        -> r0
//       nDigits     -> r1
//       pDst        -> r2
//
// Register assignments
//       r0  = pSrc
//       r1  = negated word count of pSrc
//       r2  = pSrc (moving forward one digit / 4 words every inner loop)
//       r3  = negated digit count of pSrc
//       r4  = pDst (moving forward one digit every inner loop)
//       r5  = pDst (moving forward one word every outer loop)
//       r6  = mulword from pSrc
//       r7  = Stored negated digit count of pSrc
//       r8  = Current words loaded from pSrc
//       r9  = Cyclic counter for the jumps
//       r10, r11 = "64-bit" sliding register to hold the result of multiplies,
//                   r10 also receives a word from pDst
//       r12 = Negated digit counter of pSrc (updated every 4 iterations of main loop)
//
// Stack assignments
#define pDstSq 0   // Stored pDst in stack
#define pSrc 4   // Stored pSrc in stack


//    LEAF_ENTRY SymCryptFdefRawSquareAsm
SymCryptFdefRawSquareAsm:
.globl SymCryptFdefRawSquareAsm
.def SymCryptFdefRawSquareAsm; .scl 2; .type 32; .endef
.seh_proc SymCryptFdefRawSquareAsm
push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
.seh_save_regs_w {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
.seh_endprologue
    // push         {r4-r12, lr}
    sub     sp, sp, #8


    mov     r3, r1                      // digit count into r3

    lsl     r1, r1, #2                  // Calculate word count

    neg     r1, r1                      // negate nWords
    neg     r3, r3                      // negate nDigitsSq

    mov     r4, r2                      // pDst
    mov     r5, r2                      // store pDst

    str     r0, [sp, #pSrc]             // store pSrc
    str     r5, [sp, #pDstSq]           // store pDst
    mov     r7, r3                      // store -nDigits for later
    mov     r12, r3                     // Negated digit counter of pSrc

    mov     r2, r0                      // inner loop pSrc

    //
    // First iteration of main loop (no adding of previous values from pDst)
    //
    ands    r11, r11, #0                // Clearing the carry flag and setting r11 = 0
    ldr     r6, [r0]                    // load the first word from pSrc1
    str     r11, [r4]                   // store 0 for the first word

    b       SymCryptFdefRawSquareAsmInnerLoopInit_Word1

SymCryptFdefRawSquareAsmInnerLoopInit_Word0:
    SQR_SINGLEADD_32    0

SymCryptFdefRawSquareAsmInnerLoopInit_Word1:
    SQR_SINGLEADD_32    1

    SQR_SINGLEADD_32    2

    SQR_SINGLEADD_32    3


    add     r2, r2, #16
    add     r4, r4, #16

    adds    r3, r3, #1                  // move one digit up
    bne     SymCryptFdefRawSquareAsmInnerLoopInit_Word0

    str     r11, [r4]                   // Store the next word into the destination
    add     r1, r1, #2                  // move two words up (so we stop when real word count is "-1")
    mov     r9, #1                      // Cyclic counter

    //
    // MAIN LOOP
    //
SymCryptFdefRawSquareAsmOuterLoop:

    add     r5, r5, #4                  // move start of pDst one word up

    mov     r3, r12                     // set -nDigits
    mov     r2, r0                      // set pSrc
    mov     r4, r5                      // set pDst

    ands    r11, r11, #0                // Clearing the carry flag and setting r11 = 0
    ldr     r6, [r0, r9, LSL #2]        // load the next word from pSrc

    // Cyclic counter and jump logic
    add     r9, r9, #1
    cmp     r9, #1
    beq     SymCryptFdefRawSquareAsmInnerLoop_Word1
    cmp     r9, #2
    beq     SymCryptFdefRawSquareAsmInnerLoop_Word2
    cmp     r9, #3
    beq     SymCryptFdefRawSquareAsmInnerLoop_Word3

    // The following instructions are only executed when r9 == 4
    mov     r9, #0                      // Set it to 0

    add     r0, r0, #16                 // move start of pSrc 4 words up
    add     r5, r5, #16                 // move pDst 4 words up

    mov     r2, r0                      // set pSrc
    mov     r4, r5                      // set pDst

    adds    r3, r3, #1                  // add 1 digit
    mov     r12, r3                     // set the new digit counter

SymCryptFdefRawSquareAsmInnerLoop_Word0:
    SQR_DOUBLEADD_32    0

SymCryptFdefRawSquareAsmInnerLoop_Word1:
    SQR_DOUBLEADD_32    1

SymCryptFdefRawSquareAsmInnerLoop_Word2:
    SQR_DOUBLEADD_32    2

SymCryptFdefRawSquareAsmInnerLoop_Word3:
    SQR_DOUBLEADD_32    3


    add     r2, r2, #16
    add     r4, r4, #16
    adds    r3, r3, #1              // move one digit up
    bne     SymCryptFdefRawSquareAsmInnerLoop_Word0

    str     r11, [r4]               // Store the next word into the destination

    adds    r1, r1, #1              // move one word up
    bne     SymCryptFdefRawSquareAsmOuterLoop

    eor     r11, r11, r11           // Setting r11 = 0
    str     r11, [r5, #20]          // Store 0 to destination for the top word

    // //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Second Pass - Shifting all results 1 bit left
    // Third Pass - Adding the squares on the even columns and propagating the sum
    // //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    mov     r3, r7                  // -nDigits
    lsl     r3, r3, #1              // Double digits
    ldr     r4, [sp, #pDstSq]       // pDst pointer
    ands    r1, r1, #0              // Clear the flags
    ands    r2, r2, #0              // Clear the flags

SymCryptFdefRawSquareAsmSecondPass:
    rrxs    r2, r2                  // set the carry flag if bit[0] of r2 is set

    ldmia   r4, {r8, r9}
    adcs    r8, r8, r8              // Shift left and add the carry
    adcs    r9, r9, r9
    stmia   r4!, {r8, r9}

    ldmia   r4, {r10, r11}
    adcs    r10, r10, r10           // Shift left and add the carry
    adcs    r11, r11, r11
    stmia   r4!, {r10, r11}

    adc     r2, r1, r1

    adds     r3, r3, #1             // move one digit up
    bne     SymCryptFdefRawSquareAsmSecondPass



    ldr     r0, [sp, #pSrc]         // src pointer
    ldr     r4, [sp, #pDstSq]       // pDst pointer
    // mov r3, r7                    // Use r7 as the digit counter
    // ands    r1, r1, #0            // Clear the flags
    ands    r2, r2, #0              // Clear the flags

SymCryptFdefRawSquareAsmThirdPass:
    rrxs    r2, r2                  // set the carry flag if bit[0] of r2 is set

    SQR_DIAGONAL_PROP 0
    SQR_DIAGONAL_PROP 1
    SQR_DIAGONAL_PROP 2
    SQR_DIAGONAL_PROP 3

    adc     r2, r1, r1

    add     r0, r0, #16             // One digit up (not updated in SQR_DIAGONAL_PROP)
    add     r4, r4, #32             // Two digits up (not updated in SQR_DIAGONAL_PROP)

    adds    r7, r7, #1              // move one digit up
    bne     SymCryptFdefRawSquareAsmThirdPass

    // Done, no return value

    add     sp, sp, #8
    // pop          {r4-r12, pc}

//    LEAF_END SymCryptFdefRawSquareAsm
pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,pc}
.seh_endproc

//VOID
//SymCryptFdefMontgomeryReduceAsm(
//    _In_                            PCSYMCRYPT_MODULUS      pmMod,
//    _Inout_                         PUINT32                 pSrc,
//    _Out_                           PUINT32                 pDst )
//
// Initial inputs to registers:
//       pmMod       -> r0
//       pSrc        -> r1
//       pDst        -> r2
//
// Register assignments
//       r0  = pMod (moving forward one *digit* every inner loop)
//       r1  = pSrc (moving forward one *digit* every inner loop)
//       r2  = Stored pSrc (moving forward one word every outer loop)
//       r3  = negated digit count of pSrc and pMod
//       r4  = negated word count of pSrc
//       r5, r6  = m = pSrc[i]*Inv64
//       r7  = hc = high carry variable
//       r8, r9 = Current words loaded from pMod
//       <r12:r11:r10> = "96-bit" sliding register to hold the result of multiplies

// Stack assignments
#define pMod 0               // Stored pMod
#define pDst 4               // Stored pDst
#define nDigits 8               // Stored negated digit count of pSrc
#define inv64 12              // Inv64 of modulus

//    LEAF_ENTRY SymCryptFdefMontgomeryReduceAsm
SymCryptFdefMontgomeryReduceAsm:
.globl SymCryptFdefMontgomeryReduceAsm
.def SymCryptFdefMontgomeryReduceAsm; .scl 2; .type 32; .endef
.seh_proc SymCryptFdefMontgomeryReduceAsm
push {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
.seh_save_regs_w {r4,r5,r6,r7,r8,r9,r10,r11,r12,lr}
.seh_endprologue
    // push         {r4-r12, lr}
    sub     sp, sp, #16

    str     r2, [sp, #pDst]                             // Store pDst in the stack
    ldr     r3, [r0, #SymCryptModulusNdigitsOffsetArm]  // # of Digits
    ldr     r5, [r0, #SymCryptModulusInv64OffsetArm]    // Inv64 of modulus
    add     r0, r0, #SymCryptModulusValueOffsetArm      // pMod
    str     r5, [sp, #inv64]                            // Store inv64 in the stack

    lsl     r4, r3, #2                  // Multiply by 4 to get the number of words

    neg     r3, r3                      // Negate the digit count
    neg     r4, r4                      // Negate the word count

    str     r0, [sp, #pMod]             // Store the pMod pointer
    mov     r2, r1                      // Store the pSrc pointer
    str     r3, [sp, #nDigits]          // Store the digit count for later

    eor     r7, r7, r7                  // Set hc to 0

    //
    // Main loop
    //
SymCryptFdefMontgomeryReduceAsmOuter:
    ldr     r3, [sp, #inv64]            // Inv64 of modulus

    ldmia   r1, {r10, r12}              // Load two words from pSrc
    ldmia   r0, {r8,r9}                 // Load two words from pMod
    mov     r11, #0
    mul     r5, r10, r3                 // <31:0> bits of pSrc[i]*Inv64 = m1 (first multiplier)
    umaal   r10, r11, r5, r8            // r11 <-- High( m1*pMod[0] + pSrc[i] )
    umaal   r12, r11, r5, r9            // Calculate pSrc[i+1] = Low( m1*pMod[1] + pSrc[i+1] + High( m1*pMod[0] + pSrc[i] ))
    mul     r6, r12, r3                 // <31:0> bits of pSrc[i+1]*Inv64 = m2

    ldr     r3, [sp, #nDigits]          // Reset the digit counter
    mov     r11, #0                     // Set c to 0
    mov     r12, #0                     // Set c to 0

SymCryptFdefMontgomeryReduceAsmInner:
    adds    r3, r3, #1                  // Move one digit up (none of the commands updates the carry)

    ldmia   r0!, {r8, r9}               // Load two words from pMod[]

    ldr     r10, [r1]                   // pSrc[j]
    umaal   r10, r11, r5, r8            // c = <r11:r10> = m1 * pMod[j] + pSrc[j] + c
    str     r10, [r1], #4               // pSrc[j] = (UINT32) c
    umaal   r11, r12, r6, r8            // c = <r12:r11> = m2 * pMod[j] + c

    ldr     r10, [r1]                   // pSrc[j]
    umaal   r10, r11, r5, r9            // c = <r11:r10> = m1 * pMod[j] + pSrc[j] + c
    str     r10, [r1], #4               // pSrc[j] = (UINT32) c
    umaal   r11, r12, r6, r9            // c = <r12:r11> = m2 * pMod[j] + c

    ldmia   r0!, {r8, r9}               // Load two words from pMod[]

    ldr     r10, [r1]                   // pSrc[j]
    umaal   r10, r11, r5, r8            // c = <r11:r10> = m1 * pMod[j] + pSrc[j] + c
    str     r10, [r1], #4               // pSrc[j] = (UINT32) c
    umaal   r11, r12, r6, r8            // c = <r12:r11> = m2 * pMod[j] + c

    ldr     r10, [r1]                   // pSrc[j]
    umaal   r10, r11, r5, r9            // c = <r11:r10> = m1 * pMod[j] + pSrc[j] + c
    str     r10, [r1], #4               // pSrc[j] = (UINT32) c
    umaal   r11, r12, r6, r9            // c = <r12:r11> = m2 * pMod[j] + c

    bne     SymCryptFdefMontgomeryReduceAsmInner

    mov     r8, #0                      // r8 = 0
    mov     r9, #0                      // r9 = 0

    ldmia   r1, {r5, r6}                // Load pSrc[nWords] and pSrc[nWords+1]

    adds    r11, r11, r5                // c + pSrc[nWords]
    adc     r8, r8, #0                  // Add the carry if any
    adds    r11, r11, r7                // c + pSrc[nWords] + hc
    adc     r8, r8, #0                  // Add the carry if any
    str     r11, [r1], #4               // pSrc[nWords] = c

    adds    r12, r12, r6                // c + pSrc[nWords+1]
    adc     r9, r9, #0                  // Add the carry if any
    adds    r12, r12, r8                // c + pSrc[nWords] + hc
    adc     r7, r9, #0                  // Add the carry if any
    str     r12, [r1]                   // pSrc[nWords+1] = c

    adds    r4, r4, #2                  // Move two words up

    add     r2, r2, #8                  // Move stored pSrc pointer two words up
    ldr     r0, [sp, #pMod]             // Restore the pMod pointer
    mov     r1, r2                      // Restore the pSrc pointer

    bne     SymCryptFdefMontgomeryReduceAsmOuter

    //
    // Subtraction
    //

    // Prepare the pointers for subtract
    mov     r0, r2                  // pSrc
    mov     r11, r2                 // Store pSrc for later
    ldr     r1, [sp, #pMod]         // pMod
    ldr     r2, [sp, #pDst]         // pDst
    ldr     r3, [sp, #nDigits]      // Reset the digit counter

    mov     r10, r7                 // r10 = hc

    mov     r8, #0                  // borrow = r8 = 0
    mov     r9, #0                  // r9 = 0

SymCryptFdefMontgomeryReduceRawSubAsmLoop:
    subs    r8, r9, r8              // if r8>0 then the "borrow flag" is set

    ldmia   r0!, {r4, r6}           // Load two words of pSrc1
    ldmia   r1!, {r5, r7}           // Load two words of pSrc2
    sbcs    r4, r4, r5
    sbcs    r6, r6, r7
    stmia   r2!, {r4, r6}           // Store the result in the destination

    ldmia   r0!, {r4, r6}           // Load two words of pSrc1
    ldmia   r1!, {r5, r7}           // Load two words of pSrc2
    sbcs    r4, r4, r5
    sbcs    r6, r6, r7
    stmia   r2!, {r4, r6}           // Store the result in the destination

    sbc     r8, r9, r9              // If borrow=1, then r8 = -1 = 0xffffffff

    adds    r3, r3, #1              // Increment the digit count by one
    bne     SymCryptFdefMontgomeryReduceRawSubAsmLoop

    // Prepare the pointers for masked copy
    mov     r0, r11                 // pSrc
    ldr     r1, [sp, #pDst]         // pDst

    and     r9, r8, #1              // If r8>0, set the return value to 1
    orr     r11, r10, r9            // r11 = hc|d

    ldr     r2, [sp, #nDigits]      // Restore the digit counter

    mov     r9, #0                  // r9 = 0

    subs    r4, r10, r11            // If (r11 > r10) clear the carry flag (i.e. borrow)
    sbc     r3, r9, r9              // r3 = mask = 0xffffffff if the carry flag is clear
    // orn     r9, r9, r3              // r9 = NOT(MASK) = 0 if r3 = 0xffffffff
    orr     r9, r9, r3
    mvn     r9, r9
    mov     r8, r1                  // save the destination pointer

SymCryptFdefMontgomeryReduceMaskedCopyAsmLoop:
    ldmia   r0!, {r4, r6}           // Load two words of the source
    ldmia   r1!, {r5, r7}           // Load two words of the destination
    and     r4, r4, r3
    and     r5, r5, r9
    orr     r4, r4, r5
    and     r6, r6, r3
    and     r7, r7, r9
    orr     r6, r6, r7
    stmia   r8!, {r4, r6}           // Store the two words in the destination

    ldmia   r0!, {r4, r6}           // Load two words of the source
    ldmia   r1!, {r5, r7}           // Load two words of the destination
    and     r4, r4, r3
    and     r5, r5, r9
    orr     r4, r4, r5
    and     r6, r6, r3
    and     r7, r7, r9
    orr     r6, r6, r7
    stmia   r8!, {r4, r6}           // Store the two words in the destination

    adds    r2, r2, #1              // Increment the digit count by one
    bne     SymCryptFdefMontgomeryReduceMaskedCopyAsmLoop

    // Done, no return value

    add     sp, sp, #16
//    pop          {r4-r12, pc}

//    LEAF_END SymCryptFdefMontgomeryReduceAsm
pop {r4,r5,r6,r7,r8,r9,r10,r11,r12,pc}
.seh_endproc
