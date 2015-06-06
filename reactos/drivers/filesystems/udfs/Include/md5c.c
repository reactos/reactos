/*
 * MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
 *
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 *
 * This code is the same as the code published by RSA Inc.  It has been
 * edited for clarity and style only.
 */

/*
 * This file should be kept in sync with src/lib/libmd/md5c.c
 */
//#include "md5.h"

void UDF_MD5Transform(uint32 [4], const unsigned char [64]);

/*
 * Encodes input (uint32) into output (unsigned char). Assumes len is
 * a multiple of 4.
 */

#define htole32(a)  (a)
#define le32dec(a)  (*(a))

void
UDF_Encode (unsigned char *output, uint32 *input, unsigned int len)
{
    unsigned int i;
    unsigned int *op = (unsigned int *)output;

    for (i = 0; i < len / 4; i++)
        op[i] = htole32(input[i]);
}

/*
 * Decodes input (unsigned char) into output (uint32). Assumes len is
 * a multiple of 4.
 */

void
UDF_Decode (uint32 *output, const unsigned char *input, unsigned int len)
{
    unsigned int i;
    const unsigned int *ip = (const unsigned int *)input;

    for (i = 0; i < len / 4; i++)
        output[i] = le32dec(&ip[i]);
}

static const unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G, H and I are basic MD5 functions. */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits. */
#ifdef _X86_
__declspec (naked)
uint32
__fastcall
ROTATE_LEFT(
    uint32 x, // ECX
    uint32 n  // EDX
    )
{
  _asm {
    push  ecx
    mov   eax,ecx
    mov   ecx,edx
    rol   eax,cl
    pop   ecx
    ret
  }
}
#else   // NO X86 optimization , use generic C/C++
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#endif // _X86_

/*
 * FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 * Rotation is separate from addition to prevent recomputation.
 */

void
FF(
/*    uint32 &a,
    uint32 b,
    uint32 c,
    uint32 d,*/
    uint32 a[4],
    uint32 x,
    uint32 s,
    uint32 ac
    )
{
    a[0] += F (a[1], a[2], a[3]) + (x) + (uint32)(ac);
    a[0] = ROTATE_LEFT (a[0], (s));
    a[0] += (a[1]);
}

void
GG(
/*    uint32 &a,
    uint32 b,
    uint32 c,
    uint32 d,*/
    uint32 a[4],
    uint32 x,
    uint32 s,
    uint32 ac
    )
{
/*    a += G ((b), (c), (d)) + (x) + (uint32)(ac);
    a = ROTATE_LEFT ((a), (s));
    a += (b);*/
    a[0] += G (a[1], a[2], a[3]) + (x) + (uint32)(ac);
    a[0] = ROTATE_LEFT (a[0], (s));
    a[0] += (a[1]);
}

void
HH(
/*    uint32 &a,
    uint32 b,
    uint32 c,
    uint32 d,*/
    uint32 a[4],
    uint32 x,
    uint32 s,
    uint32 ac
    )
{
/*    a += H ((b), (c), (d)) + (x) + (uint32)(ac);
    a = ROTATE_LEFT ((a), (s));
    a += (b);*/
    a[0] += H (a[1], a[2], a[3]) + (x) + (uint32)(ac);
    a[0] = ROTATE_LEFT (a[0], (s));
    a[0] += (a[1]);
}

void
II(
/*    uint32 &a,
    uint32 b,
    uint32 c,
    uint32 d,*/
    uint32 a[4],
    uint32 x,
    uint32 s,
    uint32 ac
    )
{
/*    a += I ((b), (c), (d)) + (x) + (uint32)(ac);
    a = ROTATE_LEFT ((a), (s));
    a += (b);*/
    a[0] += I (a[1], a[2], a[3]) + (x) + (uint32)(ac);
    a[0] = ROTATE_LEFT (a[0], (s));
    a[0] += (a[1]);
}

#if 0   // NO X86 optimization , use generic C/C++

#define FF(a, b, c, d, x, s, ac) { \
    (a) += F ((b), (c), (d)) + (x) + (uint32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }
#define GG(a, b, c, d, x, s, ac) { \
    (a) += G ((b), (c), (d)) + (x) + (uint32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }
#define HH(a, b, c, d, x, s, ac) { \
    (a) += H ((b), (c), (d)) + (x) + (uint32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }
#define II(a, b, c, d, x, s, ac) { \
    (a) += I ((b), (c), (d)) + (x) + (uint32)(ac); \
    (a) = ROTATE_LEFT ((a), (s)); \
    (a) += (b); \
    }
#endif // _X86_

/* MD5 initialization. Begins an MD5 operation, writing a new context. */

static const uint32 UDF_MD5Init_state[4] = {
    0x67452301,
    0xefcdab89,
    0x98badcfe,
    0x10325476
    };
    
void
UDF_MD5Init (
    UDF_MD5_CTX *context
    )
{

    context->count[0] = context->count[1] = 0;

    /* Load magic initialization constants.  */
#if 0
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
#endif
    memcpy(context->state, UDF_MD5Init_state, sizeof(UDF_MD5Init_state));
}

/* 
 * MD5 block update operation. Continues an MD5 message-digest
 * operation, processing another message block, and updating the
 * context.
 */

void
UDF_MD5Update (
    UDF_MD5_CTX *context,
    const unsigned char *input,
    unsigned int inputLen
    )
{
    unsigned int i, index, partLen;

    /* Compute number of bytes mod 64 */
    index = (unsigned int)((context->count[0] >> 3) & 0x3F);

    /* Update number of bits */
    if ((context->count[0] += ((uint32)inputLen << 3))
        < ((uint32)inputLen << 3))
        context->count[1]++;
    context->count[1] += ((uint32)inputLen >> 29);

    partLen = 64 - index;

    /* Transform as many times as possible. */
    if (inputLen >= partLen) {
        memcpy((void *)&context->buffer[index], (const void *)input,
            partLen);
        UDF_MD5Transform (context->state, context->buffer);

        for (i = partLen; i + 63 < inputLen; i += 64)
            UDF_MD5Transform (context->state, &input[i]);

        index = 0;
    }
    else
        i = 0;

    /* Buffer remaining input */
    memcpy ((void *)&context->buffer[index], (const void *)&input[i],
        inputLen-i);
}

/*
 * MD5 padding. Adds padding followed by original length.
 */

void
UDF_MD5Pad (
    UDF_MD5_CTX *context
       )    
{
    unsigned char bits[8];
    unsigned int index, padLen;

    /* Save number of bits */
    UDF_Encode (bits, context->count, 8);

    /* Pad out to 56 mod 64. */
    index = (unsigned int)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    UDF_MD5Update (context, PADDING, padLen);

    /* Append length (before padding) */
    UDF_MD5Update (context, bits, 8);
}

/*
 * MD5 finalization. Ends an MD5 message-digest operation, writing the
 * the message digest and zeroizing the context.
 */

void
UDF_MD5Final (
    unsigned char digest[16],
    UDF_MD5_CTX *context
    )
{
    /* Do padding. */
    UDF_MD5Pad (context);

    /* Store state in digest */
    UDF_Encode (digest, context->state, 16);

    /* Zeroize sensitive information. */
    memset ((void *)context, 0, sizeof (*context));
}

/* MD5 basic transformation. Transforms state based on block. */

static const uint32 UDF_MD5Transform_dwords[4][16] = {
    {
    0xd76aa478,    0xe8c7b756,    0x242070db,    0xc1bdceee,
    0xf57c0faf,    0x4787c62a,    0xa8304613,    0xfd469501,
    0x698098d8,    0x8b44f7af,    0xffff5bb1,    0x895cd7be,
    0x6b901122,    0xfd987193,    0xa679438e,    0x49b40821
    },
    {
    0xf61e2562,    0xc040b340,    0x265e5a51,    0xe9b6c7aa,
    0xd62f105d,     0x2441453,    0xd8a1e681,    0xe7d3fbc8,
    0x21e1cde6,    0xc33707d6,    0xf4d50d87,    0x455a14ed,
    0xa9e3e905,    0xfcefa3f8,    0x676f02d9,    0x8d2a4c8a
    },
    {
    0xfffa3942,    0x8771f681,    0x6d9d6122,    0xfde5380c,
    0xa4beea44,    0x4bdecfa9,    0xf6bb4b60,    0xbebfbc70,
    0x289b7ec6,    0xeaa127fa,    0xd4ef3085,     0x4881d05,
    0xd9d4d039,    0xe6db99e5,    0x1fa27cf8,    0xc4ac5665
    },
    {
    0xf4292244,    0x432aff97,    0xab9423a7,    0xfc93a039,
    0x655b59c3,    0x8f0ccc92,    0xffeff47d,    0x85845dd1,
    0x6fa87e4f,    0xfe2ce6e0,    0xa3014314,    0x4e0811a1,
    0xf7537e82,    0xbd3af235,    0x2ad7d2bb,    0xeb86d391
    }
};

static const uint32 UDF_MD5Transform_idx[4][16] = {
    {
     0,  1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, 14, 15
    },
    {
     1,  6, 11,  0,  5, 10, 15,  4,
     9, 14,  3,  8, 13,  2,  7, 12
    },
    {
     5,  8, 11, 14,  1,  4,  7, 10,
    13,  0,  3,  6,  9, 12, 15,  2
    },
    {
     0,  7, 14,  5, 12,  3, 10,  1,
     8, 15,  6, 13,  4, 11,  2,  9
    }
};

static const uint32 UDF_MD5Transform_Sxx[4][4] = {
    {    7 ,    12,    17,    22    },
    {    5 ,    9 ,    14,    20    },
    {    4 ,    11,    16,    23    },
    {    6 ,    10,    15,    21    }
};

void
UDF_MD5Rotate (
    uint32 state[4]
    )
{
    uint32 a = state[3];
    /* Load magic initialization constants.  */
#if 0
    state[3] = state[0];
    state[2] = state[3];
    state[1] = state[4];
#endif
    memmove(&state[1], &state[0], sizeof(state[0])*3);
    state[0] = a;
}

typedef void (*P_MD5_XX)
    (
/*    uint32 &a,
    uint32 b,
    uint32 c,
    uint32 d,*/
    uint32 a[4],
    uint32 x,
    uint32 s,
    uint32 ac
    );

void
UDF_MD5Transform (
    uint32 state[4],
    const unsigned char block[64]
    )
{
//    uint32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    uint32 x[16];
    uint32 state1[4];
    uint32 i, j, k;

    P_MD5_XX MD5_func[] = {FF, GG, HH, II};

    memcpy(state1, state, sizeof(state1));

    UDF_Decode (x, block, 64);

    for(j=0; j<4; j++) {
        for(i=0; i<16;) {
            for(k=0; k<4; k++, i++) {
                MD5_func[j] (state1, x[UDF_MD5Transform_idx[j][i]], UDF_MD5Transform_Sxx[j][k], UDF_MD5Transform_dwords[j][i]); /* 1 */
                UDF_MD5Rotate(state1);
            }
        }
    }
#if 0
    for(i=0; i<16;) {
        #define j 0
        for(k=0; k<4; k++, i++) {
            FF (state1, x[UDF_MD5Transform_idx[j][i]], UDF_MD5Transform_Sxx[j][k], UDF_MD5Transform_dwords[j][i]); /* 1 */
            UDF_MD5Rotate(state1);
        }
        #undef j
    }
    for(i=0; i<16;) {
        #define j 1
        for(k=0; k<4; k++, i++) {
            GG (state1, x[UDF_MD5Transform_idx[j][i]], UDF_MD5Transform_Sxx[j][k], UDF_MD5Transform_dwords[j][i]); /* 1 */
            UDF_MD5Rotate(state1);
        }
        #undef j
    }
    for(i=0; i<16;) {
        #define j 2
        for(k=0; k<4; k++, i++) {
            HH (state1, x[UDF_MD5Transform_idx[j][i]], UDF_MD5Transform_Sxx[j][k], UDF_MD5Transform_dwords[j][i]); /* 1 */
            UDF_MD5Rotate(state1);
        }
        #undef j
    }
    for(i=0; i<16;) {
        #define j 3
        for(k=0; k<4; k++, i++) {
            II (state1, x[UDF_MD5Transform_idx[j][i]], UDF_MD5Transform_Sxx[j][k], UDF_MD5Transform_dwords[j][i]); /* 1 */
            UDF_MD5Rotate(state1);
        }
        #undef j
    }
#endif //0

#if 0
    /* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
    FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
    FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
    FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
    FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
    FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
    FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
    FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
    FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
    FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
    FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
    GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
    HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
    II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */
#endif //0

    state[0] += state1[0];
    state[1] += state1[1];
    state[2] += state1[2];
    state[3] += state1[3];

    /* Zeroize sensitive information. */
    memset ((void *)x, 0, sizeof (x));
}
