//
//  sha256xmm_asm.symcryptasm   Assembler code for SHA-256 hash function. Based on
//  the intrinsics implementation SymCryptSha256AppendBlocks_xmm_4blocks() defined in
//  sha256-xmm.c
//
//  Expresses asm in a generic enough way to enable generation of MASM and GAS using the
//  symcryptasm_processor.py script and C preprocessor
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.


#include "symcryptasm_shared.cppasm"

EXTERN(SymCryptSha256K:DWORD)
EXTERN(BYTE_REVERSE_32:DWORD)
EXTERN(XMM_PACKLOW:DWORD)
EXTERN(XMM_PACKHIGH:DWORD)


SET(SHA2_INPUT_BLOCK_BYTES_LOG2,    6)
SET(SHA2_INPUT_BLOCK_BYTES,         64)
SET(SHA2_ROUNDS,                    64)
SET(SHA2_BYTES_PER_WORD,            4)
SET(SHA2_SIMD_REG_SIZE,             16)
SET(SHA2_SINGLE_BLOCK_THRESHOLD,    (3 * SHA2_INPUT_BLOCK_BYTES))   // Minimum number of message bytes required for using vectorized implementation


//
//  sha2common_asm.symcryptasm   Generic assembler routines used in SHA-2 implementations
//
//  Expresses asm in a generic enough way to enable generation of MASM and GAS using the
//  symcryptasm_processor.py script and C preprocessor
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.



SET(SHA2_SIMD_LANES,                ((SHA2_SIMD_REG_SIZE) / (SHA2_BYTES_PER_WORD)))
SET(SHA2_EXPANDED_MESSAGE_SIZE,     ((SHA2_ROUNDS) * (SHA2_SIMD_REG_SIZE)))


// Local variables used by all SHA-2 implementations
#define W               rsp
#define LocalsBase      (W + SHA2_EXPANDED_MESSAGE_SIZE)
#define numBlocks       (LocalsBase +  0 * 8)
#define numBytesToWipe  (LocalsBase +  1 * 8)


//
// Load one message word (4- or 8-bytes depending on the size of register t1),
// do the endianness transformation and store it in message buffer.
//
// ptr [in] : register pointing to the beginning of a message
// ind [in] : message index within a block (ind = 0..15)
// res [out]: output message word
//
.macro LOAD_MSG_WORD  ptr, res, ind

        mov     \res, [\ptr + (\ind) * SHA2_BYTES_PER_WORD]
        bswap   \res
        mov     [W + (\ind) * SHA2_BYTES_PER_WORD], \res

.endm


//
// Get the number of message blocks that could be processed in parallel using SIMD implementation
//
// cbMsg [in/out]   : message size in bytes on input, set to min(r1 / SHA2_INPUT_BLOCK_BYTES, SHA2_SIMD_LANES) on output
// t                : temporary register
//
.macro GET_SIMD_BLOCK_COUNT  cbMsg, t

        shr     \cbMsg, SHA2_INPUT_BLOCK_BYTES_LOG2
        mov     \t, SHA2_SIMD_LANES
        cmp     \cbMsg, \t
        cmova   \cbMsg, \t

.endm

//
// Get the number of processed message bytes
//
// The return value is a multiple of SHA2_INPUT_BLOCK_BYTES, depending
// on the level of parallelism in the vectorized implementation and available
// message bytes to process.
//
// cbMsg [in]       : message length in bytes
// cbProcessed [out]: number of processed bytes
// t                : temporary register
//
.macro GET_PROCESSED_BYTES  cbMsg, cbProcessed, t

        mov     \cbProcessed, \cbMsg
        mov     \t, SHA2_SIMD_LANES * SHA2_INPUT_BLOCK_BYTES
        cmp     \cbProcessed, \t
        cmova   \cbProcessed, \t
        and     \cbProcessed, -SHA2_INPUT_BLOCK_BYTES

.endm


//
// SHA-2 core round function that uses BMI2 instructions
//
// a, b, c, e, f, g [in]: hash function state
// d, h [in/out]        : hash function state
// rnd [in]             : round number, used for indexing into the Wk array
// t1..t5               : temporary registers
// Wk [in]              : register pointing to message/constant buffer
// scale [in]           : granularity of elements in message buffer /constant array
// c0_r1, c0_r2, c0_r3  : rotation constants for CSIGMA0 function
// c1_r1, c1_r2, c1_r3  : rotation constants for CSIGMA1 function
//
.macro ROUND_T5_BMI2_V1  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale, c0_r1, c0_r2, c0_r3, c1_r1, c1_r2, c1_r3

    //                  SELECT          CSIGMA0         CSIGMA1         MAJ
    //-------------------------------------------------------------------------------
                                                         rorx \t5, \e, \c1_r1
                                                         rorx \t4, \e, \c1_r2
                        mov  \t1, \f
                        andn \t2, \e, \g
                                                         rorx \t3, \e, \c1_r3
        add \h, [\Wk + (\rnd) * (\scale)]
                        and  \t1, \e
                                                         xor  \t5, \t4
                        xor  \t1, \t2
                                                         xor  \t5, \t3
        add \h, \t1
                                         rorx \t2, \a, \c0_r1
                                                                        mov \t3, \b
                                                                        mov \t4, \b
                                         rorx \t1, \a, \c0_r2
        add \h, \t5
                                                                        or  \t3, \c
                                                                        and \t4, \c
                                                                        and \t3, \a
                                                                        or  \t4, \t3
        add \d, \h
                                         xor  \t2, \t1
                                         rorx \t5, \a, \c0_r3
                                         xor  \t2, \t5
        add \h, \t4
        add \h, \t2

.endm


.macro ROUND_T5_BMI2_V2  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale, c0_r1, c0_r2, c0_r3, c1_r1, c1_r2, c1_r3

    //                  SELECT          CSIGMA0         CSIGMA1         MAJ
    //-------------------------------------------------------------------------------
                        mov  \t1, \f
                        andn \t2, \e, \g
                                                         rorx \t5, \e, \c1_r1
                        and  \t1, \e
                                                         rorx \t4, \e, \c1_r2
                                                         rorx \t3, \e, \c1_r3
        add \h, [\Wk + (\rnd) * (\scale)]
                                                         xor \t5, \t4
                        xor  \t1, \t2
                                                         xor  \t5, \t3
        add \h, \t1
        add \h, \t5
                                                                        mov \t3, \b
                                         rorx \t1, \a, \c0_r2
                                         rorx \t2, \a, \c0_r1
                                                                        mov \t4, \b
                                                                        or  \t3, \c
                                                                        and \t4, \c
        add \d, \h
                                                                        and \t3, \a
                                                                        or  \t4, \t3
                                         xor  \t2, \t1
                                         rorx \t5, \a, \c0_r3
                                         xor  \t2, \t5
        add \h, \t4
        add \h, \t2

.endm



.macro ROUND_256  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale

    ROUND_T5_BMI2_V1 \a, \b, \c, \d, \e, \f, \g, \h, \rnd, \t1, \t2, \t3, \t4, \t5, \Wk, \scale, 2, 13, 22, 6, 11, 25

.endm


.macro ROUND_512  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale

    ROUND_T5_BMI2_V1 \a, \b, \c, \d, \e, \f, \g, \h, \rnd, \t1, \t2, \t3, \t4, \t5, \Wk, \scale, 28, 34, 39, 14, 18, 41

.endm



.macro SHA2_UPDATE_CV_HELPER  rcv, r0, r1, r2, r3, r4, r5, r6, r7

        add \r0, [\rcv + 0 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 0 * SHA2_BYTES_PER_WORD], \r0
        add \r1, [\rcv + 1 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 1 * SHA2_BYTES_PER_WORD], \r1
        add \r2, [\rcv + 2 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 2 * SHA2_BYTES_PER_WORD], \r2
        add \r3, [\rcv + 3 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 3 * SHA2_BYTES_PER_WORD], \r3
        add \r4, [\rcv + 4 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 4 * SHA2_BYTES_PER_WORD], \r4
        add \r5, [\rcv + 5 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 5 * SHA2_BYTES_PER_WORD], \r5
        add \r6, [\rcv + 6 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 6 * SHA2_BYTES_PER_WORD], \r6
        add \r7, [\rcv + 7 * SHA2_BYTES_PER_WORD]
        mov [\rcv + 7 * SHA2_BYTES_PER_WORD], \r7

.endm

#define SHA256_UPDATE_CV(rcv)   SHA2_UPDATE_CV_HELPER rcv, D0, D1, D2, D3, D4, D5, D6, D7

#define SHA512_UPDATE_CV(rcv)   SHA2_UPDATE_CV_HELPER rcv, Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7


//
//  Load message words, do the endianness transformation and transpose for one or more message blocks.
//
//  P [in]          : register pointing to the beginning of a message
//  N [in]          : number of blocks ( N = 3 or 4 )
//  t, v            : temporary registers
//  ind             : index of which quarter of the block we're processing (ind = 0..3)
//  kreverse        : YMM register containing the value used to do the endianness transform
//  y1..y4  [out]   : YMM registers containing the transposed message words
//  t1..t4          : temporary YMM registers
.macro SHA512_MSG_LOAD_TRANSPOSE_YMM  P, N, t, v, ind, kreverse, y1, y2, y3, y4, t1, t2, t3, t4

        // Make \t point to the third or the fourth block depending on the
        // number of message blocks we have
        mov \t, 2 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 32
        mov \v, 3 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 32
        cmp \N, 4
        cmove \t, \v

        // We have at least three message blocks
        vmovdqu \y1, YMMWORD ptr [\P + 0 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 32]
        vpshufb \y1, \y1, \kreverse
        vmovdqu \y2, YMMWORD ptr [\P + 1 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 32]
        vpshufb \y2, \y2, \kreverse
        vmovdqu \y3, YMMWORD ptr [\P + 2 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 32]
        vpshufb \y3, \y3, \kreverse

        // If there are only three blocks then \t points to the third block and
        // that block is read again, otherwise fourth block is loaded. Rereading
        // a block when we don'\t have the fourth block is to avoid conditional
        // loading, the value will not be used.
        vmovdqu \y4, YMMWORD ptr [\P + \t]
        vpshufb \y4, \y4, \kreverse

        SHA512_MSG_TRANSPOSE_YMM \ind, \y1, \y2, \y3, \y4, \t1, \t2, \t3, \t4

.endm

//
// Transpose message words from 4 blocks so that each YMM register contains message
// words with the same index within a message block. This macro transforms four message words at a time,
// hence needs to be called four times in order to transform 16 message words.
//
// The transformation is the same whether we have 4 or less blocks. If we have less than 4 blocks,
// the corresponding high order lanes contain garbage, and will not be used in round processing.
//
// ind [in]         : index to which quarter the transformation will take place (ind = 0..3)
// y1..y4 [in/out] : YMM registers each containing 4 message words from different blocks
//                        (y1 = W[4 * ind + 0], y2 = W[4 * ind + 1], y3 = W[4 * ind + 2], y4 = W[4 * ind + 3])
// t1..t4           : temporary YMM registers
//
.macro SHA512_MSG_TRANSPOSE_YMM  ind, y1, y2, y3, y4, t1, t2, t3, t4

        vpunpcklqdq \t1, \y1, \y2
        vpunpcklqdq \t2, \y3, \y4
        vpunpckhqdq \t3, \y1, \y2
        vpunpckhqdq \t4, \y3, \y4
        vperm2i128  \y1, \t1, \t2, HEX(20)
        vperm2i128  \y2, \t3, \t4, HEX(20)
        vperm2i128  \y3, \t1, \t2, HEX(31)
        vperm2i128  \y4, \t3, \t4, HEX(31)
        vmovdqu     YMMWORD ptr [W + (\ind) * 128 + 0 * 32], \y1
        vmovdqu     YMMWORD ptr [W + (\ind) * 128 + 1 * 32], \y2
        vmovdqu     YMMWORD ptr [W + (\ind) * 128 + 2 * 32], \y3
        vmovdqu     YMMWORD ptr [W + (\ind) * 128 + 3 * 32], \y4

.endm



//
//  Load message words, do the endianness transformation and transpose
//  the first 16 bytes of each message block.
//
//  P [in]          : register pointing to the beginning of a message
//  N [in]          : number of blocks (N = 3 or 4)
//  t1,t2           : temporary registers
//  ind             : index corresponding to which quarter of the message blocks
//                    the transformation will take place, 0 <= ind <= 3
//  kreverse [in]   : XMM register containing the value for endianness transformation
//
//
.macro SHA256_MSG_LOAD_TRANSPOSE_XMM  P, N, t1, t2, ind, kreverse, x0, x1, x2, x3, xt0, xt1, xt2, xt3

        // Make \t2 point to the third or the fourth block depending on the
        // number of message blocks we have
        mov     \t2, 2 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 16
        mov     \t1, 3 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 16
        cmp     \N, 4
        cmove   \t2, \t1

        // Load from the first three message blocks
        movdqu  \x0, XMMWORD ptr [\P + 0 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 16]
        pshufb  \x0, \kreverse
        movdqu  \x1, XMMWORD ptr [\P + 1 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 16]
        pshufb  \x1, \kreverse
        movdqu  \x2, XMMWORD ptr [\P + 2 * SHA2_INPUT_BLOCK_BYTES + (\ind) * 16]
        pshufb  \x2, \kreverse

        // If there are only three blocks then \t2 points to the third block and
        // that block is read again, otherwise fourth block is loaded. Rereading
        // a block when we don't have the fourth block is to avoid conditional
        // loading, the value will not be used.
        movdqu  \x3, XMMWORD ptr [\P + \t2]
        pshufb  \x3, \kreverse

        SHA256_MSG_TRANSPOSE_XMM \ind, \x0, \x1, \x2, \x3, \xt0, \xt1, \xt2, \xt3

.endm

//
// Transpose message words from 4 blocks so that each XMM register contains message
// words with the same index within a message block. This macro transforms four message words at a time,
// hence needs to be called four times in order to transform 16 message words.
//
// The transformation is the same whether we have 4 or less blocks. If we have less than 4 blocks,
// the corresponding high order lanes contain garbage, and will not be used in round processing.
//
// ind [in] : index to which quarter the transformation will take place (ind = 0..3)
// x0..x3   : XMM registers containing the message words with same indices from different blocks
//            for instance when ind=0, each xi contains the first 16 bytes of consecutive message blocks
// t0..t3   : temporary registers
//
.macro SHA256_MSG_TRANSPOSE_XMM  ind, x0, x1, x2, x3, t0, t1, t2, t3

        // \x0 = A3 A2 A1 A0
        // \x1 = B3 B2 B1 B0
        // \x2 = C3 C2 C1 C0
        // \x3 = D3 D2 D1 D0
        movdqa      \t0, \x0
        punpckhdq   \t0, \x1  // \t0 = B3 A3 B2 A2
        punpckldq   \x0, \x1  // \x0 = B1 A1 B0 A0
        movdqa      \t1, \x2
        punpckhdq   \t1, \x3  // \t1 = D3 C3 D2 C2
        punpckldq   \x2, \x3  // \x2 = D1 C1 D0 C0

        movdqa      \x1, \x0
        punpckhqdq  \x1, \x2  // \x1 = D1 C1 B1 A1
        punpcklqdq  \x0, \x2  // \x0 = D0 C0 B0 A0
        movdqa      XMMWORD ptr [W + 64 * (\ind) + 0 * 16], \x0
        movdqa      XMMWORD ptr [W + 64 * (\ind) + 1 * 16], \x1

        movdqa      \x3, \t0
        punpckhqdq  \x3, \t1  // \x3 = D3 C3 B3 A3
        punpcklqdq  \t0, \t1  // \t0 = D2 C2 B2 A2
        movdqa      XMMWORD ptr [W + 64 * (\ind) + 2 * 16], \t0
        movdqa      XMMWORD ptr [W + 64 * (\ind) + 3 * 16], \x3

.endm


//
// Rotate each 32-bit value in an XMM register
//
// x [in]   : XMM register holding four 32-bit integers
// c [in]   : rotation count
// res [out]: XMM register holding the rotated values
// t1       : temporary XMM register
//
.macro ROR32_XMM  x, c, res, t1

    movdqa  \res, \x
    movdqa  \t1, \x
    psrld   \res, \c
    pslld   \t1, 32 - \c
    pxor    \res, \t1

.endm


//
// LSIGMA function as defined in FIPS 180-4 acting on four parallel 32-bit values in a XMM register.
//
// x [in]       : XMM register holding four 32-bit integers
// c1..c3 [in]  : rotation and shift counts
// res [out]    : output of the LSIGMA function as four 32-bit integer values
// t1,t2        : temporary XMM registers
//
.macro LSIGMA_XMM  x, c1, c2, c3, res, t1, t2

        ROR32_XMM   \x, \c1, \res, \t1
        ROR32_XMM   \x, \c2, \t2, \t1
        movdqa      \t1, \x
        psrld       \t1, \c3
        pxor        \res, \t2
        pxor        \res, \t1

.endm



//
// Message expansion for 4 consecutive message blocks and adds constants to round (rnd - 16)
//
// y0 [in/out]      : W_{rnd - 16}, on macro exit it is loaded with W_{rnd - 14} and used as y1 for the
//                    subsequent macro invocation.
// y1 [in]          : W_{rnd - 15}
// y9 [in]          : W_{rnd - 7}
// y14 [in]         : W_{rnd - 2}
// rnd [in]         : round number, rnd = 16..24, uses the previous 16 message word state to generate the next one
// t1 [out]         : expanded message word
// t2..t6           : temporary XMM registers
// Wx [in]          : pointer to the message buffer
// k256 [in]        : pointer to the constants
//
.macro SHA256_MSG_EXPAND_4BLOCKS  y0, y1, y9, y14, rnd, t1, t2, t3, t4, t5, t6, Wx, k256

        movd        \t1, DWORD ptr [\k256 + 4 * (\rnd - 16)]
        pshufd      \t1, \t1, 0
        paddd       \t1, \y0                                      // \t1 = W_{t-16} + K_{t-16}
        movdqa      XMMWORD ptr [\Wx + (\rnd - 16) * 16], \t1      // store W_{t-16} + K_{t-16}

        LSIGMA_XMM  \y14, 17, 19, 10, \t4, \t5, \t3                 // \t4 = LSIGMA1(W_{t-2})
        LSIGMA_XMM  \y1,   7, 18,  3, \t1, \t5, \t3                 // \t1 = LSIGMA0(W_{t-15})
        paddd       \y0, \y9                                      // \y0 = W_{t-16} + W_{t-7}
        paddd       \t1, \y0                                      // \t1 = W_{t-16} + W_{t-7} + LSIGMA0(W_{t-15})
        paddd       \t1, \t4                                      // \t1 = W_{t-16} + W_{t-7} + LSIGMA0(W_{t-15}) + LSIGMA1(W_{t-2})
        movdqa      \y0, XMMWORD ptr [\Wx + (\rnd - 14) * 16]      // \y0 = W_{t-14}, load W_{t-15} for next round
        movdqa      XMMWORD ptr [\Wx + (\rnd) * 16], \t1           // store W_t

.endm


//
// Single block message expansion using XMM registers
//
// x0..x3 [in/out]  : 16 word message state
// t1..t6           : temporary XMM registers
// karr             : pointer to the round constants
// ind              : index used to calculate the offsets for loading constants and storing words to
//                    message buffer W, each increment points to next 4 round constant and message words.
// packlow, packhigh: constants for shuffling words and clearing top/bottom halves of an XMM register
//
.macro SHA256_MSG_EXPAND_1BLOCK  x0, x1, x2, x3, t1, t2, t3, t4, t5, t6, karr, ind, packlow, packhigh

        // Message word state before the expansion
        // \x0 =  W3  W2  W1  W0
        // \x1 =  W7  W6  W5  W4
        // \x2 = W11 W10  W9  W8
        // \x3 = W15 W14 W13 W12

        // After the expansion we will have
        // \x1 =  W7  W6  W5  W4
        // \x2 = W11 W10  W9  W8
        // \x3 = W15 W14 W13 W12
        // \x0 = W19 W18 W17 W16

        movdqa      \t2, \x1
        palignr     \t2, \x0, 4                           // \t2 = W4 W3 W2 W1

        // LSIGMA1(W15 W14)
        pshufd      \t1, \x3, HEX(0fa)                    // \t1 = W15 W15 W14 W14
        movdqa      \t5, \t1
        movdqa      \t3, \t1
        psrlq       \t5, 17
        psrlq       \t3, 19
        pxor        \t5, \t3
        psrld       \t1, 10
        pxor        \t5, \t1
        pshufb      \t5, \packlow                         // \t5 = 0 0 LSIGMA1(W15 W14)

        LSIGMA_XMM  \t2, 7, 18, 3, \t3, \t1, \t6            // \t3 = LSIGMA0(W4 W3 W2 W1)

        paddd       \x0, \t5                              // \x0 = (W3 W2 W1 W0) + (0 0 LSIGMA1(W15 W14))

        movdqa      \t4, \x3
        palignr     \t4, \x2, 4                           // \t4 = W12 W11 W10 W9

        paddd       \t4, \t3                              // \t4 = (W12 W11 W10 W9) + LSIGMA0(W4 W3 W2 W1)
        paddd       \x0, \t4                              // \x0 = (W3 W2 W1 W0) + (0 0 LSIGMA1(W15 W14)) + (W12 W11 W10 W9) + LSIGMA0(W4 W3 W2 W1)

        // LSIGMA1(W17 W16)
        pshufd      \t1, \x0, HEX(50)                     // \t1 = W17 W17 W16 W16
        movdqa      \t2, \t1
        movdqa      \t3, \t1
        psrlq       \t2, 17
        psrlq       \t3, 19
        pxor        \t2, \t3
        psrld       \t1, 10
        pxor        \t2, \t1
        pshufb      \t2, \packhigh                        // \t2 = LSIGMA1(W17 W16) 0 0

        movdqa      \t6, XMMWORD ptr [\karr + \ind * 16]   // \t6 = K19 K18 K17 K16
        paddd       \x0, \t2                              // \x0 = W19 W18 W17 W16
        paddd       \t6, \x0                              // \t6 = (K19 K18 K17 K16) + (W19 W18 W17 W16)
        movdqa      XMMWORD ptr [rsp + \ind * 16], \t6

.endm


//
// Add one set of constants to eight message words from multiple blocks in an XMM register
//
// rnd [in]     : round index, rnd = 0..7 (Wx and k256 are adjusted so that this macro always acts on the next 8 rounds)
// t1, t2       : temporary XMM registers
// Wx [in]      : pointer to the message buffer
// k256 [in]    : pointer to the constants array
//
.macro SHA256_MSG_ADD_CONST  rnd, t1, t2, Wx, k256

        movd    \t1, DWORD ptr [\k256 + 4 * (\rnd)]
        pshufd  \t1, \t1, 0
        movdqa  \t2, XMMWORD ptr [\Wx + 16 * (\rnd)]
        paddd   \t1, \t2
        movdqa XMMWORD ptr [\Wx + (\rnd) * 16], \t1

.endm






//VOID
//SYMCRYPT_CALL
//SymCryptSha256AppendBlocks(
//    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE* pChain,
//    _In_reads_(cbData)      PCBYTE                          pbData,
//                            SIZE_T                          cbData,
//    _Out_                   SIZE_T*                         pcbRemaining)


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
#define Q13 r14
#define D13 r14d
#define W13 r14w
#define B13 r14b
#define Q14 r15
#define D14 r15d
#define W14 r15w
#define B14 r15b
PLATFORM_NAME_MANGLE(SymCryptSha256AppendBlocks_xmm_ssse3_asm):
.globl PLATFORM_NAME_MANGLE(SymCryptSha256AppendBlocks_xmm_ssse3_asm)
.def PLATFORM_NAME_MANGLE(SymCryptSha256AppendBlocks_xmm_ssse3_asm); .scl 2; .type 32; .endef
.seh_proc PLATFORM_NAME_MANGLE(SymCryptSha256AppendBlocks_xmm_ssse3_asm)

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
push Q13
.seh_pushreg Q13
push Q14
.seh_pushreg Q14
sub rsp, 1208
.seh_stackalloc 1208
movdqa xmmword ptr [rsp + 1040], xmm6
.seh_savexmm xmm6, 1040
movdqa xmmword ptr [rsp + 1056], xmm7
.seh_savexmm xmm7, 1056
movdqa xmmword ptr [rsp + 1072], xmm8
.seh_savexmm xmm8, 1072
movdqa xmmword ptr [rsp + 1088], xmm9
.seh_savexmm xmm9, 1088
movdqa xmmword ptr [rsp + 1104], xmm10
.seh_savexmm xmm10, 1104
movdqa xmmword ptr [rsp + 1120], xmm11
.seh_savexmm xmm11, 1120
movdqa xmmword ptr [rsp + 1136], xmm12
.seh_savexmm xmm12, 1136
movdqa xmmword ptr [rsp + 1152], xmm13
.seh_savexmm xmm13, 1152
movdqa xmmword ptr [rsp + 1168], xmm14
.seh_savexmm xmm14, 1168
movdqa xmmword ptr [rsp + 1184], xmm15
.seh_savexmm xmm15, 1184

.seh_endprologue


        // Q1 = pChain
        // Q2 = pbData
        // Q3 = cbData
        // Q4 = pcbRemaining

        mov     [rsp + 1280 /*MEMSLOT0*/], Q1
        mov     [rsp + 1288 /*MEMSLOT1*/], Q2
        mov     [rsp + 1296 /*MEMSLOT2*/], Q3
        mov     [rsp + 1304 /*MEMSLOT3*/], Q4


        // We have two implementations using different message buffer sizes. The code below checks the
        // input message size and helps avoid wiping the larger buffer if we're not using it.
        //
        // If we're processing SHA2_SINGLE_BLOCK_THRESHOLD or more bytes, vectorized message expansion is
        // used, which expands the message words for 4 blocks. Message expansion for single block processing
        // uses a buffer of 16 message words. Both buffers start at address W (rsp).
        //
        // numBytesToWipe variable holds the number of bytes to wipe from the expanded message buffer
        // before returning from this call.
        //
        // Q3 [in]  : cbData
        // D8 [out] : numBytesToWipe
        mov     D8, 16 * SHA2_BYTES_PER_WORD
        mov     D9, SHA2_EXPANDED_MESSAGE_SIZE
        cmp     Q3, SHA2_SINGLE_BLOCK_THRESHOLD
        cmovae  D8, D9
        mov     [numBytesToWipe], D8

        mov     Q10, Q1
        mov     D0, DWORD ptr [Q10 +  0]
        mov     D1, DWORD ptr [Q10 +  4]
        mov     D2, DWORD ptr [Q10 +  8]
        mov     D3, DWORD ptr [Q10 + 12]
        mov     D4, DWORD ptr [Q10 + 16]
        mov     D5, DWORD ptr [Q10 + 20]
        mov     D6, DWORD ptr [Q10 + 24]
        mov     D7, DWORD ptr [Q10 + 28]

        // If message size is less than SHA2_SINGLE_BLOCK_THRESHOLD then use single block message expansion,
        // otherwise use vectorized message expansion.
        mov     Q8, [rsp + 1296 /*MEMSLOT2*/]
        cmp     Q8, SHA2_SINGLE_BLOCK_THRESHOLD
        jb      single_block_entry

        ALIGN(16)
process_blocks:
        // Calculate the number of blocks to process, Q8 = cbData
        GET_SIMD_BLOCK_COUNT Q8, Q9     // Q8 = min(cbData / 64, 4)
        mov     [numBlocks], Q8

        //
        // Load and transpose message words
        //
        mov Q9, [rsp + 1288 /*MEMSLOT1*/]
        movdqa xmm8, XMMWORD ptr [GET_SYMBOL_ADDRESS(BYTE_REVERSE_32)]
        SHA256_MSG_LOAD_TRANSPOSE_XMM Q9, Q8, Q10, Q11, 0, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7
        SHA256_MSG_LOAD_TRANSPOSE_XMM Q9, Q8, Q10, Q11, 1, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7
        SHA256_MSG_LOAD_TRANSPOSE_XMM Q9, Q8, Q10, Q11, 2, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7
        SHA256_MSG_LOAD_TRANSPOSE_XMM Q9, Q8, Q10, Q11, 3, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7

        // Load transposed message words to registers to be used in message expansion
        // Each register contains message words (of the same index within a block) from multiple blocks
        movdqa xmm0, XMMWORD ptr [W + 16 *  0]
        movdqa xmm1, XMMWORD ptr [W + 16 *  1]
        movdqa xmm2, XMMWORD ptr [W + 16 *  9]
        movdqa xmm3, XMMWORD ptr [W + 16 * 10]
        movdqa xmm4, XMMWORD ptr [W + 16 * 11]
        movdqa xmm5, XMMWORD ptr [W + 16 * 12]
        movdqa xmm6, XMMWORD ptr [W + 16 * 13]
        movdqa xmm7, XMMWORD ptr [W + 16 * 14]
        movdqa xmm8, XMMWORD ptr [W + 16 * 15]

        lea     Q13, [W]                                    // pointer to expanded message words
        lea     Q14, [GET_SYMBOL_ADDRESS(SymCryptSha256K)]  // pointer to constants

expand_process_first_block:

        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm2, xmm7, (16 + 0), xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm3, xmm8, (16 + 1), xmm2, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14
        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm4, xmm9, (16 + 2), xmm3, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm5, xmm2, (16 + 3), xmm4, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14
        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm6, xmm3, (16 + 4), xmm5, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm7, xmm4, (16 + 5), xmm6, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14
        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm8, xmm5, (16 + 6), xmm7, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm9, xmm6, (16 + 7), xmm8, xmm10, xmm11, xmm12, xmm13, xmm14, Q13, Q14

        ROUND_256    D0, D1, D2, D3, D4, D5, D6, D7, 0, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D7, D0, D1, D2, D3, D4, D5, D6, 1, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D6, D7, D0, D1, D2, D3, D4, D5, 2, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D5, D6, D7, D0, D1, D2, D3, D4, 3, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D4, D5, D6, D7, D0, D1, D2, D3, 4, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D3, D4, D5, D6, D7, D0, D1, D2, 5, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D2, D3, D4, D5, D6, D7, D0, D1, 6, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D1, D2, D3, D4, D5, D6, D7, D0, 7, D8, D9, D10, D11, D12, Q13, 16

        lea     Q8, [GET_SYMBOL_ADDRESS(SymCryptSha256K) + 48 * 4]
        add     Q13, 8 * 16 // next message words
        add     Q14, 8 * 4  // next constants
        cmp     Q14, Q8
        jb      expand_process_first_block

        // Final 16 rounds
final_rounds:
        SHA256_MSG_ADD_CONST 0, xmm1, xmm2, Q13, Q14
        SHA256_MSG_ADD_CONST 1, xmm1, xmm2, Q13, Q14
        SHA256_MSG_ADD_CONST 2, xmm1, xmm2, Q13, Q14
        SHA256_MSG_ADD_CONST 3, xmm1, xmm2, Q13, Q14
        SHA256_MSG_ADD_CONST 4, xmm1, xmm2, Q13, Q14
        SHA256_MSG_ADD_CONST 5, xmm1, xmm2, Q13, Q14
        SHA256_MSG_ADD_CONST 6, xmm1, xmm2, Q13, Q14
        SHA256_MSG_ADD_CONST 7, xmm1, xmm2, Q13, Q14
        ROUND_256    D0, D1, D2, D3, D4, D5, D6, D7, 0, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D7, D0, D1, D2, D3, D4, D5, D6, 1, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D6, D7, D0, D1, D2, D3, D4, D5, 2, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D5, D6, D7, D0, D1, D2, D3, D4, 3, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D4, D5, D6, D7, D0, D1, D2, D3, 4, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D3, D4, D5, D6, D7, D0, D1, D2, 5, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D2, D3, D4, D5, D6, D7, D0, D1, 6, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D1, D2, D3, D4, D5, D6, D7, D0, 7, D8, D9, D10, D11, D12, Q13, 16

        lea     Q8, [GET_SYMBOL_ADDRESS(SymCryptSha256K) + 64 * 4]
        add     Q13, 8 * 16 // next message words
        add     Q14, 8 * 4  // next constants
        cmp     Q14, Q8
        jb      final_rounds

        mov     Q8, [rsp + 1280 /*MEMSLOT0*/]
        SHA256_UPDATE_CV(Q8)

        // We've processed one block, update the variable.
        // Note: We always have more than one block, no need to check the result of the decrement.
        dec qword ptr [numBlocks]

        lea     Q13, [W + 4]    // second message block words

block_begin:

        mov     D14, 64 / 8

        ALIGN(16)
inner_loop:
        //                                                                      Wk  scale
        ROUND_256    D0, D1, D2, D3, D4, D5, D6, D7,  0, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D7, D0, D1, D2, D3, D4, D5, D6,  1, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D6, D7, D0, D1, D2, D3, D4, D5,  2, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D5, D6, D7, D0, D1, D2, D3, D4,  3, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D4, D5, D6, D7, D0, D1, D2, D3,  4, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D3, D4, D5, D6, D7, D0, D1, D2,  5, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D2, D3, D4, D5, D6, D7, D0, D1,  6, D8, D9, D10, D11, D12, Q13, 16
        ROUND_256    D1, D2, D3, D4, D5, D6, D7, D0,  7, D8, D9, D10, D11, D12, Q13, 16

        add     Q13, 8 * 16 // advance to next 8 message words
        sub     D14, 1
        jnz     inner_loop

        add     Q13, (4 - 64 * 16)  // advance to the beginning of message words for the next block

        mov     Q8, [rsp + 1280 /*MEMSLOT0*/]
        SHA256_UPDATE_CV(Q8)

        dec     QWORD ptr [numBlocks]
        jnz     block_begin

        // Update pbData and cbData
        mov     Q8, [rsp + 1296 /*MEMSLOT2*/]
        GET_PROCESSED_BYTES Q8, Q9, Q10     // Q9 = bytesProcessed
        sub     Q8, Q9
        add     QWORD ptr [rsp + 1288 /*MEMSLOT1*/], Q9
        mov     QWORD ptr [rsp + 1296 /*MEMSLOT2*/], Q8
        cmp     Q8, SHA2_SINGLE_BLOCK_THRESHOLD
        jae     process_blocks


        ALIGN(16)
single_block_entry:

        cmp     Q8, SHA2_INPUT_BLOCK_BYTES      // Q8 = cbData
        jb      done

        // Load the constants once before the block processing loop begins
        // These registers are not touched during block processing
        movdqa  xmm13, XMMWORD ptr [GET_SYMBOL_ADDRESS(BYTE_REVERSE_32)]
        movdqa  xmm14, XMMWORD ptr [GET_SYMBOL_ADDRESS(XMM_PACKLOW)]
        movdqa  xmm15, XMMWORD ptr [GET_SYMBOL_ADDRESS(XMM_PACKHIGH)]

single_block_start:

        mov     Q13, [rsp + 1288 /*MEMSLOT1*/]
        lea     Q14, [GET_SYMBOL_ADDRESS(SymCryptSha256K)]

        //
        // Load first 16 message words into xmm0..xmm3 and do the endianness transformation
        // Store the constant added words to message buffer W
        //
        movdqu  xmm0, XMMWORD ptr [Q13 + 0 * 16]
        movdqu  xmm1, XMMWORD ptr [Q13 + 1 * 16]
        movdqu  xmm2, XMMWORD ptr [Q13 + 2 * 16]
        movdqu  xmm3, XMMWORD ptr [Q13 + 3 * 16]
        pshufb  xmm0, xmm13
        pshufb  xmm1, xmm13
        pshufb  xmm2, xmm13
        pshufb  xmm3, xmm13
        movdqa  xmm8,  XMMWORD ptr [Q14 + 0 * 16]
        movdqa  xmm9,  XMMWORD ptr [Q14 + 1 * 16]
        movdqa  xmm10, XMMWORD ptr [Q14 + 2 * 16]
        movdqa  xmm11, XMMWORD ptr [Q14 + 3 * 16]
        paddd   xmm8,  xmm0
        paddd   xmm9,  xmm1
        paddd   xmm10, xmm2
        paddd   xmm11, xmm3
        movdqa  XMMWORD ptr [W + 0 * 16], xmm8
        movdqa  XMMWORD ptr [W + 1 * 16], xmm9
        movdqa  XMMWORD ptr [W + 2 * 16], xmm10
        movdqa  XMMWORD ptr [W + 3 * 16], xmm11

inner_loop_single:

        add     Q14, 16 * 4

        //                                                                                    karr ind packlo packhi
        ROUND_256    D0, D1, D2, D3, D4, D5, D6, D7,  0, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D7, D0, D1, D2, D3, D4, D5, D6,  1, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D6, D7, D0, D1, D2, D3, D4, D5,  2, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D5, D6, D7, D0, D1, D2, D3, D4,  3, D8, D9, D10, D11, D12, W, 4
        SHA256_MSG_EXPAND_1BLOCK xmm0, xmm1, xmm2, xmm3,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  Q14, 0, xmm14, xmm15

        ROUND_256    D4, D5, D6, D7, D0, D1, D2, D3,  4, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D3, D4, D5, D6, D7, D0, D1, D2,  5, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D2, D3, D4, D5, D6, D7, D0, D1,  6, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D1, D2, D3, D4, D5, D6, D7, D0,  7, D8, D9, D10, D11, D12, W, 4
        SHA256_MSG_EXPAND_1BLOCK xmm1, xmm2, xmm3, xmm0,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  Q14, 1, xmm14, xmm15

        ROUND_256    D0, D1, D2, D3, D4, D5, D6, D7,  8, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D7, D0, D1, D2, D3, D4, D5, D6,  9, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D6, D7, D0, D1, D2, D3, D4, D5, 10, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D5, D6, D7, D0, D1, D2, D3, D4, 11, D8, D9, D10, D11, D12, W, 4
        SHA256_MSG_EXPAND_1BLOCK xmm2, xmm3, xmm0, xmm1,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  Q14, 2, xmm14, xmm15

        ROUND_256    D4, D5, D6, D7, D0, D1, D2, D3, 12, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D3, D4, D5, D6, D7, D0, D1, D2, 13, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D2, D3, D4, D5, D6, D7, D0, D1, 14, D8, D9, D10, D11, D12, W, 4
        ROUND_256    D1, D2, D3, D4, D5, D6, D7, D0, 15, D8, D9, D10, D11, D12, W, 4
        SHA256_MSG_EXPAND_1BLOCK xmm3, xmm0, xmm1, xmm2,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  Q14, 3, xmm14, xmm15

        lea     Q8, [GET_SYMBOL_ADDRESS(SymCryptSha256K) + 48 * 4]
        cmp     Q14, Q8
        jb      inner_loop_single

        lea Q13, [W]
        lea Q14, [W + 16 * 4]

single_block_final_rounds:

        ROUND_256    D0, D1, D2, D3, D4, D5, D6, D7,  0, D8, D9, D10, D11, D12, Q13, 4
        ROUND_256    D7, D0, D1, D2, D3, D4, D5, D6,  1, D8, D9, D10, D11, D12, Q13, 4
        ROUND_256    D6, D7, D0, D1, D2, D3, D4, D5,  2, D8, D9, D10, D11, D12, Q13, 4
        ROUND_256    D5, D6, D7, D0, D1, D2, D3, D4,  3, D8, D9, D10, D11, D12, Q13, 4
        ROUND_256    D4, D5, D6, D7, D0, D1, D2, D3,  4, D8, D9, D10, D11, D12, Q13, 4
        ROUND_256    D3, D4, D5, D6, D7, D0, D1, D2,  5, D8, D9, D10, D11, D12, Q13, 4
        ROUND_256    D2, D3, D4, D5, D6, D7, D0, D1,  6, D8, D9, D10, D11, D12, Q13, 4
        ROUND_256    D1, D2, D3, D4, D5, D6, D7, D0,  7, D8, D9, D10, D11, D12, Q13, 4

        add Q13, 8 * 4
        cmp Q13, Q14
        jb single_block_final_rounds

        mov     Q8, [rsp + 1280 /*MEMSLOT0*/]
        SHA256_UPDATE_CV(Q8)

        // Update pbData and cbData
        mov     Q8, [rsp + 1296 /*MEMSLOT2*/]
        sub     Q8, SHA2_INPUT_BLOCK_BYTES
        add     QWORD ptr [rsp + 1288 /*MEMSLOT1*/], SHA2_INPUT_BLOCK_BYTES
        mov     QWORD ptr [rsp + 1296 /*MEMSLOT2*/], Q8
        cmp     Q8, SHA2_INPUT_BLOCK_BYTES
        jae     single_block_start

done:

        //mov   Q8, [rsp + 1296 /*MEMSLOT2*/]
        mov     Q9, [rsp + 1304 /*MEMSLOT3*/]
        mov     QWORD ptr [Q9], Q8

        // Wipe expanded message words
        xor     rax, rax
        mov     rdi, rsp
        mov     ecx, [numBytesToWipe]

        // wipe first 64 bytes, the size of the small buffer
        pxor    xmm0, xmm0
        movaps  [rdi + 0 * 16], xmm0
        movaps  [rdi + 1 * 16], xmm0
        movaps  [rdi + 2 * 16], xmm0
        movaps  [rdi + 3 * 16], xmm0
        add     rdi, 4 * 16

        //  if we used vectorized message expansion, wipe the large buffer
        sub     ecx, 4 * 16 // already wiped above
        jz      nowipe
        rep     stosb

nowipe:



movdqa xmm6, xmmword ptr [rsp + 1040]
movdqa xmm7, xmmword ptr [rsp + 1056]
movdqa xmm8, xmmword ptr [rsp + 1072]
movdqa xmm9, xmmword ptr [rsp + 1088]
movdqa xmm10, xmmword ptr [rsp + 1104]
movdqa xmm11, xmmword ptr [rsp + 1120]
movdqa xmm12, xmmword ptr [rsp + 1136]
movdqa xmm13, xmmword ptr [rsp + 1152]
movdqa xmm14, xmmword ptr [rsp + 1168]
movdqa xmm15, xmmword ptr [rsp + 1184]
add rsp, 1208
pop Q14
pop Q13
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
#undef Q13
#undef D13
#undef W13
#undef B13
#undef Q14
#undef D14
#undef W14
#undef B14

FILE_END()
