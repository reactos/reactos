#include "precomp.h"

#if  SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#ifdef __clang__
#pragma clang attribute push (__attribute__((target("ssse3"))), apply_to=function)
#else
#pragma GCC push_options
#pragma GCC target("ssse3")
#endif

extern SYMCRYPT_ALIGN_AT(256) const  UINT32 SymCryptSha256K[64];


// Endianness transformation for 4 32-bit values in an XMM register
const SYMCRYPT_ALIGN_AT(16) UINT32 BYTE_REVERSE_32[4] = {
    0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f,
};

// Shuffle 32-bit words in an XMM register: W3 W2 W1 W0 -> 0 0 W2 W0
// Used by the SSSE3 assembly implementation
const SYMCRYPT_ALIGN_AT(16) UINT32 XMM_PACKLOW[4] = {
    0x03020100, 0x0b0a0908, 0x80808080, 0x80808080,
};

// Shuffle 32-bit words in an XMM register: W3 W2 W1 W0 -> W2 W0 0 0
// Used by the SSSE3 assembly implementation
const SYMCRYPT_ALIGN_AT(16) UINT32 XMM_PACKHIGH[4] = {
    0x80808080, 0x80808080, 0x03020100, 0x0b0a0908,
};


#if SYMCRYPT_MS_VC && !defined(__clang__)
#define RORX_U32  _rorx_u32
#define RORX_U64  _rorx_u64
#else
// TODO: implement _rorx functions for clang
#define RORX_U32  ROR32
#define RORX_U64  ROR64
#endif // SYMCRYPT_MS_VC


//
// For documentation on these function see FIPS 180-2
//
// MAJ and CH are the functions Maj and Ch from the standard.
// CSIGMA0 and CSIGMA1 are the capital sigma functions.
// LSIGMA0 and LSIGMA1 are the lowercase sigma functions.
//
// The canonical definitions of the MAJ and CH functions are:
//#define MAJ( x, y, z )    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
//#define CH( x, y, z )  (((x) & (y)) ^ ((~(x)) & (z)))
// We use optimized versions defined below
//
#define MAJ( x, y, z )  ((((z) | (y)) & (x) ) | ((z) & (y)))
#define CH( x, y, z )  ((((z) ^ (y)) & (x)) ^ (z))

#define LSIGMA0( x )    (ROR32((x),  7) ^ ROR32((x), 18) ^ ((x)>> 3))
#define LSIGMA1( x )    (ROR32((x), 17) ^ ROR32((x), 19) ^ ((x)>>10))

#define CSIGMA0(x) (RORX_U32(x, 2) ^ RORX_U32(x, 13) ^ RORX_U32(x, 22))
#define CSIGMA1(x) (RORX_U32(x, 6) ^ RORX_U32(x, 11) ^ RORX_U32(x, 25))


#define LSIGMA0XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,25)  , _mm_srli_epi32(x,  7) ),\
        _mm_slli_epi32(x,14) ), _mm_srli_epi32(x, 18) ),\
        _mm_srli_epi32(x, 3) )
#define LSIGMA1XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,15)  , _mm_srli_epi32(x, 17) ),\
        _mm_slli_epi32(x,13) ), _mm_srli_epi32(x, 19) ),\
        _mm_srli_epi32(x,10) )



// Initial loading of message words and endianness transformation.
// bl : The number of blocks to load,  1 <= bl <= 4.
//
// When bl < 4, the high order lanes of the XMM registers corresponding to the missing blocks are unused.
//
#define SHA256_MSG_LOAD_4BLOCKS(bl) { \
        for(SIZE_T i = 0; i < bl; i++) \
        { \
            Wx.xmm[i +  0] = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*) &pbData[i * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE +  0]), kBYTE_REVERSE_32); \
            Wx.xmm[i +  4] = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*) &pbData[i * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE + 16]), kBYTE_REVERSE_32); \
            Wx.xmm[i +  8] = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*) &pbData[i * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE + 32]), kBYTE_REVERSE_32); \
            Wx.xmm[i + 12] = _mm_shuffle_epi8(_mm_loadu_si128((__m128i*) &pbData[i * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE + 48]), kBYTE_REVERSE_32); \
        } \
}

// Shuffles the initially loaded message words from multiple blocks
// so that each XMM register contains message words with the same index
// within a block (e.g. Wx.xmm[0] contains the first words of each block).
//
// We have to use this macro four times to transform the message blocks of 64-bytes.
// ind=0 processes the first quarter (16-bytes), ind=1 does the second quarter and so on.
//
#define SHA256_MSG_TRANSPOSE_QUARTER_4BLOCKS(ind)  { \
        __m128i t1, t2, t3, t4; \
        t1 = _mm_unpacklo_epi32(Wx.xmm[4 * (ind) + 0], Wx.xmm[4 * (ind) + 1]); \
        t2 = _mm_unpacklo_epi32(Wx.xmm[4 * (ind) + 2], Wx.xmm[4 * (ind) + 3]); \
        t3 = _mm_unpackhi_epi32(Wx.xmm[4 * (ind) + 0], Wx.xmm[4 * (ind) + 1]); \
        t4 = _mm_unpackhi_epi32(Wx.xmm[4 * (ind) + 2], Wx.xmm[4 * (ind) + 3]); \
        Wx.xmm[4 * (ind) + 0] = _mm_unpacklo_epi64(t1, t2); \
        Wx.xmm[4 * (ind) + 1] = _mm_unpackhi_epi64(t1, t2); \
        Wx.xmm[4 * (ind) + 2] = _mm_unpacklo_epi64(t3, t4); \
        Wx.xmm[4 * (ind) + 3] = _mm_unpackhi_epi64(t3, t4); \
}

#define SHA256_MSG_TRANSPOSE_4BLOCKS() { \
        SHA256_MSG_TRANSPOSE_QUARTER_4BLOCKS(0); \
        SHA256_MSG_TRANSPOSE_QUARTER_4BLOCKS(1); \
        SHA256_MSG_TRANSPOSE_QUARTER_4BLOCKS(2); \
        SHA256_MSG_TRANSPOSE_QUARTER_4BLOCKS(3); \
}

// One round message schedule, updates the rth message word. ( 16 <= r < 64 )
// Also adds the constants for round (r-16).
#define SHA256_MSG_EXPAND_4BLOCKS_1ROUND(r) { \
        Wx.xmm[r] = _mm_add_epi32(_mm_add_epi32(_mm_add_epi32(Wx.xmm[r - 16], Wx.xmm[r - 7]), \
                                LSIGMA0XMM(Wx.xmm[r - 15])), LSIGMA1XMM(Wx.xmm[r - 2])); \
        Wx.xmm[r - 16] = _mm_add_epi32(Wx.xmm[r - 16], _mm_set1_epi32(SymCryptSha256K[r - 16])); \
}

// Four rounds of message schedule. Generates message words for rounds r, r+1, r+2, r+3.
#define SHA256_MSG_EXPAND_4BLOCKS_4ROUNDS(r) { \
        SHA256_MSG_EXPAND_4BLOCKS_1ROUND((r) + 0); SHA256_MSG_EXPAND_4BLOCKS_1ROUND((r) + 1); \
        SHA256_MSG_EXPAND_4BLOCKS_1ROUND((r) + 2); SHA256_MSG_EXPAND_4BLOCKS_1ROUND((r) + 3); \
}
// Sixteen rounds of message schedule. Generates message words for rounds r, ..., r+15.
#define SHA256_MSG_EXPAND_4BLOCKS_16ROUNDS(r) { \
        SHA256_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 0); SHA256_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 4); \
        SHA256_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 8); SHA256_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 12); \
}

// Core round function using message words from Wx array.
// Wx contains -interleaved- expanded message words from b blocks.
// i.e. Message words for round r for each block, followed by the message words for the (r+1)^th block.
//
// r16 : round number mod 16
// rb  : base round number so that (rb+r16) gives the actual round number
// b   : message block index, b = 0..3
#define CROUND_4BLOCKS(r16, rb, b) {   \
    Wt = Wx.ul4[(rb)+(r16)][b]; \
    ah[ r16   &7] += CSIGMA1(ah[(r16+3)&7]) + CH(ah[(r16+3)&7], ah[(r16+2)&7], ah[(r16+1)&7]) + Wt;\
    ah[(r16+4)&7] += ah[r16 &7];\
    ah[ r16   &7] += CSIGMA0(ah[(r16+7)&7]) + MAJ(ah[(r16+7)&7], ah[(r16+6)&7], ah[(r16+5)&7]);\
}

//
// Core round function
//
// r16 : round number mod 16
// r   : round number, r = 0..63
//
#define CROUND( r16, r ) {;\
    ah[ r16   &7] += CSIGMA1(ah[(r16+3)&7]) + CH(ah[(r16+3)&7], ah[(r16+2)&7], ah[(r16+1)&7]) + SymCryptSha256K[r] + Wt;\
    ah[(r16+4)&7] += ah[r16 &7];\
    ah[ r16   &7] += CSIGMA0(ah[(r16+7)&7]) + MAJ(ah[(r16+7)&7], ah[(r16+6)&7], ah[(r16+5)&7]);\
}

//
// Initial round that reads the message.
// r is the round number 0..15
//
#define IROUND( r ) {\
    Wt = SYMCRYPT_LOAD_MSBFIRST32( &pbData[ 4*r ] );\
    Wx.ul[r] = Wt; \
    CROUND(r,r);\
}

//
// Subsequent rounds.
// r16 is the round number mod 16. rb is the round number minus r16.
//
#define FROUND(r16, rb) {                                    \
    Wt = LSIGMA1( Wx.ul[(r16-2) & 15] ) +   Wx.ul[(r16-7) & 15] +    \
         LSIGMA0( Wx.ul[(r16-15) & 15]) +   Wx.ul[r16 & 15];         \
    Wx.ul[r16] = Wt; \
    CROUND( r16, r16+rb ); \
}



VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_xmm_4blocks(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE* pChain,
    _In_reads_(cbData)      PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T*                         pcbRemaining)
{

    SYMCRYPT_ALIGN union { UINT32 ul[16]; UINT32 ul4[64][4]; __m128i xmm[64]; } Wx;
    SYMCRYPT_ALIGN UINT32 ah[8];
    UINT32 Wt;
    SIZE_T uWipeSize = (cbData >= (3 * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE)) ? (64 * 4 * sizeof(UINT32)) : (16 * sizeof(UINT32));

    const __m128i kBYTE_REVERSE_32 = _mm_load_si128((const __m128i*)BYTE_REVERSE_32);

    while (cbData >= (3 * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE))
    {
        // If we have 4 or more blocks then process 4, else process whatever is left.
        SIZE_T numBlocks = (cbData >= 4 * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE) ? 4 : (cbData / SYMCRYPT_SHA256_INPUT_BLOCK_SIZE);

        SHA256_MSG_LOAD_4BLOCKS(numBlocks);
        SHA256_MSG_TRANSPOSE_4BLOCKS();

        for (int j = 16; j < 64; j += 16)
        {
            SHA256_MSG_EXPAND_4BLOCKS_16ROUNDS(j);
        }

        // Constants up to r=48 were added during message expansion. Add the remaining ones here.
        for (int i = 48; i < 64; i++)
        {
            Wx.xmm[i] = _mm_add_epi32(Wx.xmm[i], _mm_set1_epi32(SymCryptSha256K[i]));
        }

        for (SIZE_T bl = 0; bl < numBlocks; bl++)
        {
            ah[7] = pChain->H[0];
            ah[6] = pChain->H[1];
            ah[5] = pChain->H[2];
            ah[4] = pChain->H[3];
            ah[3] = pChain->H[4];
            ah[2] = pChain->H[5];
            ah[1] = pChain->H[6];
            ah[0] = pChain->H[7];

            for (int iterCount = 0; iterCount < (64/8); iterCount++)
            {
                const int roundBase = iterCount*8;
                CROUND_4BLOCKS( 0, roundBase, bl);
                CROUND_4BLOCKS( 1, roundBase, bl);
                CROUND_4BLOCKS( 2, roundBase, bl);
                CROUND_4BLOCKS( 3, roundBase, bl);
                CROUND_4BLOCKS( 4, roundBase, bl);
                CROUND_4BLOCKS( 5, roundBase, bl);
                CROUND_4BLOCKS( 6, roundBase, bl);
                CROUND_4BLOCKS( 7, roundBase, bl);
                //CROUND_4BLOCKS( 8, roundBase, bl);
                //CROUND_4BLOCKS( 9, roundBase, bl);
                //CROUND_4BLOCKS(10, roundBase, bl);
                //CROUND_4BLOCKS(11, roundBase, bl);
                //CROUND_4BLOCKS(12, roundBase, bl);
                //CROUND_4BLOCKS(13, roundBase, bl);
                //CROUND_4BLOCKS(14, roundBase, bl);
                //CROUND_4BLOCKS(15, roundBase, bl);
            }

            pChain->H[0] = ah[7] + pChain->H[0];
            pChain->H[1] = ah[6] + pChain->H[1];
            pChain->H[2] = ah[5] + pChain->H[2];
            pChain->H[3] = ah[4] + pChain->H[3];
            pChain->H[4] = ah[3] + pChain->H[4];
            pChain->H[5] = ah[2] + pChain->H[5];
            pChain->H[6] = ah[1] + pChain->H[6];
            pChain->H[7] = ah[0] + pChain->H[7];
        }

        pbData += (numBlocks * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE);
        cbData -= (numBlocks * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE);
    }


    while (cbData >= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE)
    {
        ah[7] = pChain->H[0];
        ah[6] = pChain->H[1];
        ah[5] = pChain->H[2];
        ah[4] = pChain->H[3];
        ah[3] = pChain->H[4];
        ah[2] = pChain->H[5];
        ah[1] = pChain->H[6];
        ah[0] = pChain->H[7];

        //
        // initial rounds 1 to 16
        //

        IROUND(0);
        IROUND(1);
        IROUND(2);
        IROUND(3);
        IROUND(4);
        IROUND(5);
        IROUND(6);
        IROUND(7);
        IROUND(8);
        IROUND(9);
        IROUND(10);
        IROUND(11);
        IROUND(12);
        IROUND(13);
        IROUND(14);
        IROUND(15);


        //
        // rounds 16 to 64.
        //
        for (int iterCount = 1; iterCount < (64/16); iterCount++)
        {
            const int roundBase = iterCount*16;
            FROUND(0, roundBase);
            FROUND(1, roundBase);
            FROUND(2, roundBase);
            FROUND(3, roundBase);
            FROUND(4, roundBase);
            FROUND(5, roundBase);
            FROUND(6, roundBase);
            FROUND(7, roundBase);
            FROUND(8, roundBase);
            FROUND(9, roundBase);
            FROUND(10, roundBase);
            FROUND(11, roundBase);
            FROUND(12, roundBase);
            FROUND(13, roundBase);
            FROUND(14, roundBase);
            FROUND(15, roundBase);
        }

        pChain->H[0] = ah[7] + pChain->H[0];
        pChain->H[1] = ah[6] + pChain->H[1];
        pChain->H[2] = ah[5] + pChain->H[2];
        pChain->H[3] = ah[4] + pChain->H[3];
        pChain->H[4] = ah[3] + pChain->H[4];
        pChain->H[5] = ah[2] + pChain->H[5];
        pChain->H[6] = ah[1] + pChain->H[6];
        pChain->H[7] = ah[0] + pChain->H[7];

        pbData += SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;
        cbData -= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;
    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipe(&Wx, uWipeSize);
    SymCryptWipeKnownSize(ah, sizeof(ah));
}

#ifdef __clang__
#pragma clang attribute pop
#else
#pragma GCC pop_options
#endif

#endif // SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
