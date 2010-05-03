/*
 * FILE:	sha2.c
 * AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
 *
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <string.h>
#include <assert.h>
#include "sha2.h"

/*
 * ASSERT NOTE:
 * Some sanity checking code is included using assert().  On my FreeBSD
 * system, this additional code can be removed by compiling with NDEBUG
 * defined.  Check your own systems manpage on assert() to see how to
 * compile WITHOUT the sanity checking code on your system.
 *
 * UNROLLED TRANSFORM LOOP NOTE:
 * You can define SHA2_UNROLL_TRANSFORM to use the unrolled transform
 * loop version for the hash transform rounds (defined using macros
 * later in this file).  Either define on the command line, for example:
 *
 *   cc -DSHA2_UNROLL_TRANSFORM -o sha2 sha2.c sha2prog.c
 *
 * or define below:
 *
 *   #define SHA2_UNROLL_TRANSFORM
 *
 */

/*** SHA-256/384/512 Various Length Definitions ***********************/
/* NOTE: Most of these are in sha2.h */
#define SHA256_SHORT_BLOCK_LENGTH	(SHA256_BLOCK_LENGTH - 8)
#define SHA384_SHORT_BLOCK_LENGTH	(SHA384_BLOCK_LENGTH - 16)
#define SHA512_SHORT_BLOCK_LENGTH	(SHA512_BLOCK_LENGTH - 16)

#define SHA2_WORD64_CONST(dw1, dw2) (((sha2_word64)(dw1) << 32) | (dw2))

/*** ENDIAN REVERSAL MACROS *******************************************/
#ifndef WORDS_BIGENDIAN
#define REVERSE32(w,x)	{ \
	sha2_word32 tmp = (w); \
	tmp = (tmp >> 16) | (tmp << 16); \
	(x) = ((tmp & 0xff00ff00) >> 8) | ((tmp & 0x00ff00ff) << 8); \
}
#define REVERSE64(w,x)	{ \
	sha2_word64 tmp = (w); \
	tmp = (tmp >> 32) | (tmp << 32); \
	tmp = ((tmp & SHA2_WORD64_CONST(0xff00ff00, 0xff00ff00)) >> 8) | \
	      ((tmp & SHA2_WORD64_CONST(0x00ff00ff, 0x00ff00ff)) << 8); \
	(x) = ((tmp & SHA2_WORD64_CONST(0xffff0000, 0xffff0000)) >> 16) | \
	      ((tmp & SHA2_WORD64_CONST(0x0000ffff, 0x0000ffff)) << 16); \
}
#endif

/*
 * Macro for incrementally adding the unsigned 64-bit integer n to the
 * unsigned 128-bit integer (represented using a two-element array of
 * 64-bit words):
 */
#define ADDINC128(w,n)	{ \
	(w)[0] += (sha2_word64)(n); \
	if ((w)[0] < (n)) { \
		(w)[1]++; \
	} \
}

/*
 * Macros for copying blocks of memory and for zeroing out ranges
 * of memory.  Using these macros makes it easy to switch from
 * using memset()/memcpy() and using bzero()/bcopy().
 *
 * Please define either SHA2_USE_MEMSET_MEMCPY or define
 * SHA2_USE_BZERO_BCOPY depending on which function set you
 * choose to use:
 */
#if !defined(SHA2_USE_MEMSET_MEMCPY) && !defined(SHA2_USE_BZERO_BCOPY)
/* Default to memset()/memcpy() if no option is specified */
#define	SHA2_USE_MEMSET_MEMCPY	1
#endif
#if defined(SHA2_USE_MEMSET_MEMCPY) && defined(SHA2_USE_BZERO_BCOPY)
/* Abort with an error if BOTH options are defined */
#error Define either SHA2_USE_MEMSET_MEMCPY or SHA2_USE_BZERO_BCOPY, not both!
#endif

#ifdef SHA2_USE_MEMSET_MEMCPY
#define MEMSET_BZERO(p,l)	memset((p), 0, (l))
#define MEMCPY_BCOPY(d,s,l)	memcpy((d), (s), (l))
#endif
#ifdef SHA2_USE_BZERO_BCOPY
#define MEMSET_BZERO(p,l)	bzero((p), (l))
#define MEMCPY_BCOPY(d,s,l)	bcopy((s), (d), (l))
#endif


/*** THE SIX LOGICAL FUNCTIONS ****************************************/
/*
 * Bit shifting and rotation (used by the six SHA-XYZ logical functions:
 *
 *   NOTE:  The naming of R and S appears backwards here (R is a SHIFT and
 *   S is a ROTATION) because the SHA-256/384/512 description document
 *   (see http://csrc.nist.gov/cryptval/shs/sha256-384-512.pdf) uses this
 *   same "backwards" definition.
 */
/* Shift-right (used in SHA-256, SHA-384, and SHA-512): */
#define R(b,x) 		((x) >> (b))
/* 32-bit Rotate-right (used in SHA-256): */
#define S32(b,x)	(((x) >> (b)) | ((x) << (32 - (b))))
/* 64-bit Rotate-right (used in SHA-384 and SHA-512): */
#define S64(b,x)	(((x) >> (b)) | ((x) << (64 - (b))))

/* Two of six logical functions used in SHA-256, SHA-384, and SHA-512: */
#define Ch(x,y,z)	(((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)	(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Four of six logical functions used in SHA-256: */
#define Sigma0_256(x)	(S32(2,  (x)) ^ S32(13, (x)) ^ S32(22, (x)))
#define Sigma1_256(x)	(S32(6,  (x)) ^ S32(11, (x)) ^ S32(25, (x)))
#define sigma0_256(x)	(S32(7,  (x)) ^ S32(18, (x)) ^ R(3 ,   (x)))
#define sigma1_256(x)	(S32(17, (x)) ^ S32(19, (x)) ^ R(10,   (x)))

/* Four of six logical functions used in SHA-384 and SHA-512: */
#define Sigma0_512(x)	(S64(28, (x)) ^ S64(34, (x)) ^ S64(39, (x)))
#define Sigma1_512(x)	(S64(14, (x)) ^ S64(18, (x)) ^ S64(41, (x)))
#define sigma0_512(x)	(S64( 1, (x)) ^ S64( 8, (x)) ^ R( 7,   (x)))
#define sigma1_512(x)	(S64(19, (x)) ^ S64(61, (x)) ^ R( 6,   (x)))

/*** INTERNAL FUNCTION PROTOTYPES *************************************/
/* NOTE: These should not be accessed directly from outside this
 * library -- they are intended for private internal visibility/use
 * only.
 */
void SHA512_Last(SHA512_CTX*);
void SHA256_Transform(SHA256_CTX*, const sha2_word32*);
void SHA512_Transform(SHA512_CTX*, const sha2_word64*);


/*** SHA-XYZ INITIAL HASH VALUES AND CONSTANTS ************************/
/* Hash constant words K for SHA-256: */
static const sha2_word32 K256[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Initial hash value H for SHA-256: */
static const sha2_word32 sha256_initial_hash_value[8] = {
	0x6a09e667,
	0xbb67ae85,
	0x3c6ef372,
	0xa54ff53a,
	0x510e527f,
	0x9b05688c,
	0x1f83d9ab,
	0x5be0cd19
};

/* Hash constant words K for SHA-384 and SHA-512: */
static const sha2_word64 K512[80] = {
	SHA2_WORD64_CONST(0x428a2f98, 0xd728ae22), SHA2_WORD64_CONST(0x71374491, 0x23ef65cd),
	SHA2_WORD64_CONST(0xb5c0fbcf, 0xec4d3b2f), SHA2_WORD64_CONST(0xe9b5dba5, 0x8189dbbc),
	SHA2_WORD64_CONST(0x3956c25b, 0xf348b538), SHA2_WORD64_CONST(0x59f111f1, 0xb605d019),
	SHA2_WORD64_CONST(0x923f82a4, 0xaf194f9b), SHA2_WORD64_CONST(0xab1c5ed5, 0xda6d8118),
	SHA2_WORD64_CONST(0xd807aa98, 0xa3030242), SHA2_WORD64_CONST(0x12835b01, 0x45706fbe),
	SHA2_WORD64_CONST(0x243185be, 0x4ee4b28c), SHA2_WORD64_CONST(0x550c7dc3, 0xd5ffb4e2),
	SHA2_WORD64_CONST(0x72be5d74, 0xf27b896f), SHA2_WORD64_CONST(0x80deb1fe, 0x3b1696b1),
	SHA2_WORD64_CONST(0x9bdc06a7, 0x25c71235), SHA2_WORD64_CONST(0xc19bf174, 0xcf692694),
	SHA2_WORD64_CONST(0xe49b69c1, 0x9ef14ad2), SHA2_WORD64_CONST(0xefbe4786, 0x384f25e3),
	SHA2_WORD64_CONST(0x0fc19dc6, 0x8b8cd5b5), SHA2_WORD64_CONST(0x240ca1cc, 0x77ac9c65),
	SHA2_WORD64_CONST(0x2de92c6f, 0x592b0275), SHA2_WORD64_CONST(0x4a7484aa, 0x6ea6e483),
	SHA2_WORD64_CONST(0x5cb0a9dc, 0xbd41fbd4), SHA2_WORD64_CONST(0x76f988da, 0x831153b5),
	SHA2_WORD64_CONST(0x983e5152, 0xee66dfab), SHA2_WORD64_CONST(0xa831c66d, 0x2db43210),
	SHA2_WORD64_CONST(0xb00327c8, 0x98fb213f), SHA2_WORD64_CONST(0xbf597fc7, 0xbeef0ee4),
	SHA2_WORD64_CONST(0xc6e00bf3, 0x3da88fc2), SHA2_WORD64_CONST(0xd5a79147, 0x930aa725),
	SHA2_WORD64_CONST(0x06ca6351, 0xe003826f), SHA2_WORD64_CONST(0x14292967, 0x0a0e6e70),
	SHA2_WORD64_CONST(0x27b70a85, 0x46d22ffc), SHA2_WORD64_CONST(0x2e1b2138, 0x5c26c926),
	SHA2_WORD64_CONST(0x4d2c6dfc, 0x5ac42aed), SHA2_WORD64_CONST(0x53380d13, 0x9d95b3df),
	SHA2_WORD64_CONST(0x650a7354, 0x8baf63de), SHA2_WORD64_CONST(0x766a0abb, 0x3c77b2a8),
	SHA2_WORD64_CONST(0x81c2c92e, 0x47edaee6), SHA2_WORD64_CONST(0x92722c85, 0x1482353b),
	SHA2_WORD64_CONST(0xa2bfe8a1, 0x4cf10364), SHA2_WORD64_CONST(0xa81a664b, 0xbc423001),
	SHA2_WORD64_CONST(0xc24b8b70, 0xd0f89791), SHA2_WORD64_CONST(0xc76c51a3, 0x0654be30),
	SHA2_WORD64_CONST(0xd192e819, 0xd6ef5218), SHA2_WORD64_CONST(0xd6990624, 0x5565a910),
	SHA2_WORD64_CONST(0xf40e3585, 0x5771202a), SHA2_WORD64_CONST(0x106aa070, 0x32bbd1b8),
	SHA2_WORD64_CONST(0x19a4c116, 0xb8d2d0c8), SHA2_WORD64_CONST(0x1e376c08, 0x5141ab53),
	SHA2_WORD64_CONST(0x2748774c, 0xdf8eeb99), SHA2_WORD64_CONST(0x34b0bcb5, 0xe19b48a8),
	SHA2_WORD64_CONST(0x391c0cb3, 0xc5c95a63), SHA2_WORD64_CONST(0x4ed8aa4a, 0xe3418acb),
	SHA2_WORD64_CONST(0x5b9cca4f, 0x7763e373), SHA2_WORD64_CONST(0x682e6ff3, 0xd6b2b8a3),
	SHA2_WORD64_CONST(0x748f82ee, 0x5defb2fc), SHA2_WORD64_CONST(0x78a5636f, 0x43172f60),
	SHA2_WORD64_CONST(0x84c87814, 0xa1f0ab72), SHA2_WORD64_CONST(0x8cc70208, 0x1a6439ec),
	SHA2_WORD64_CONST(0x90befffa, 0x23631e28), SHA2_WORD64_CONST(0xa4506ceb, 0xde82bde9),
	SHA2_WORD64_CONST(0xbef9a3f7, 0xb2c67915), SHA2_WORD64_CONST(0xc67178f2, 0xe372532b),
	SHA2_WORD64_CONST(0xca273ece, 0xea26619c), SHA2_WORD64_CONST(0xd186b8c7, 0x21c0c207),
	SHA2_WORD64_CONST(0xeada7dd6, 0xcde0eb1e), SHA2_WORD64_CONST(0xf57d4f7f, 0xee6ed178),
	SHA2_WORD64_CONST(0x06f067aa, 0x72176fba), SHA2_WORD64_CONST(0x0a637dc5, 0xa2c898a6),
	SHA2_WORD64_CONST(0x113f9804, 0xbef90dae), SHA2_WORD64_CONST(0x1b710b35, 0x131c471b),
	SHA2_WORD64_CONST(0x28db77f5, 0x23047d84), SHA2_WORD64_CONST(0x32caab7b, 0x40c72493),
	SHA2_WORD64_CONST(0x3c9ebe0a, 0x15c9bebc), SHA2_WORD64_CONST(0x431d67c4, 0x9c100d4c),
	SHA2_WORD64_CONST(0x4cc5d4be, 0xcb3e42b6), SHA2_WORD64_CONST(0x597f299c, 0xfc657e2a),
	SHA2_WORD64_CONST(0x5fcb6fab, 0x3ad6faec), SHA2_WORD64_CONST(0x6c44198c, 0x4a475817)
};

/* Initial hash value H for SHA-384 */
static const sha2_word64 sha384_initial_hash_value[8] = {
	SHA2_WORD64_CONST(0xcbbb9d5d, 0xc1059ed8),
	SHA2_WORD64_CONST(0x629a292a, 0x367cd507),
	SHA2_WORD64_CONST(0x9159015a, 0x3070dd17),
	SHA2_WORD64_CONST(0x152fecd8, 0xf70e5939),
	SHA2_WORD64_CONST(0x67332667, 0xffc00b31),
	SHA2_WORD64_CONST(0x8eb44a87, 0x68581511),
	SHA2_WORD64_CONST(0xdb0c2e0d, 0x64f98fa7),
	SHA2_WORD64_CONST(0x47b5481d, 0xbefa4fa4)
};

/* Initial hash value H for SHA-512 */
static const sha2_word64 sha512_initial_hash_value[8] = {
	SHA2_WORD64_CONST(0x6a09e667, 0xf3bcc908),
	SHA2_WORD64_CONST(0xbb67ae85, 0x84caa73b),
	SHA2_WORD64_CONST(0x3c6ef372, 0xfe94f82b),
	SHA2_WORD64_CONST(0xa54ff53a, 0x5f1d36f1),
	SHA2_WORD64_CONST(0x510e527f, 0xade682d1),
	SHA2_WORD64_CONST(0x9b05688c, 0x2b3e6c1f),
	SHA2_WORD64_CONST(0x1f83d9ab, 0xfb41bd6b),
	SHA2_WORD64_CONST(0x5be0cd19, 0x137e2179)
};

/*
 * Constant used by SHA256/384/512_End() functions for converting the
 * digest to a readable hexadecimal character string:
 */
static const char sha2_hex_digits[] = "0123456789abcdef";


/*** SHA-256: *********************************************************/
void SHA256_Init(SHA256_CTX* context) {
	if (context == (SHA256_CTX*)0) {
		return;
	}
	MEMCPY_BCOPY(context->state, sha256_initial_hash_value, SHA256_DIGEST_LENGTH);
	MEMSET_BZERO(context->buffer, SHA256_BLOCK_LENGTH);
	context->bitcount = 0;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-256 round macros: */

#ifndef WORDS_BIGENDIAN

#define ROUND256_0_TO_15(a,b,c,d,e,f,g,h)	\
	REVERSE32(*data++, W256[j]); \
	T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + \
             K256[j] + W256[j]; \
	(d) += T1; \
	(h) = T1 + Sigma0_256(a) + Maj((a), (b), (c)); \
	j++


#else

#define ROUND256_0_TO_15(a,b,c,d,e,f,g,h)	\
	T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + \
	     K256[j] + (W256[j] = *data++); \
	(d) += T1; \
	(h) = T1 + Sigma0_256(a) + Maj((a), (b), (c)); \
	j++

#endif

#define ROUND256(a,b,c,d,e,f,g,h)	\
	s0 = W256[(j+1)&0x0f]; \
	s0 = sigma0_256(s0); \
	s1 = W256[(j+14)&0x0f]; \
	s1 = sigma1_256(s1); \
	T1 = (h) + Sigma1_256(e) + Ch((e), (f), (g)) + K256[j] + \
	     (W256[j&0x0f] += s1 + W256[(j+9)&0x0f] + s0); \
	(d) += T1; \
	(h) = T1 + Sigma0_256(a) + Maj((a), (b), (c)); \
	j++

void SHA256_Transform(SHA256_CTX* context, const sha2_word32* data) {
	sha2_word32	a, b, c, d, e, f, g, h, s0, s1;
	sha2_word32	T1, *W256;
	int		j;

	W256 = (sha2_word32*)context->buffer;

	/* Initialize registers with the prev. intermediate value */
	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];
	f = context->state[5];
	g = context->state[6];
	h = context->state[7];

	j = 0;
	do {
		/* Rounds 0 to 15 (unrolled): */
		ROUND256_0_TO_15(a,b,c,d,e,f,g,h);
		ROUND256_0_TO_15(h,a,b,c,d,e,f,g);
		ROUND256_0_TO_15(g,h,a,b,c,d,e,f);
		ROUND256_0_TO_15(f,g,h,a,b,c,d,e);
		ROUND256_0_TO_15(e,f,g,h,a,b,c,d);
		ROUND256_0_TO_15(d,e,f,g,h,a,b,c);
		ROUND256_0_TO_15(c,d,e,f,g,h,a,b);
		ROUND256_0_TO_15(b,c,d,e,f,g,h,a);
	} while (j < 16);

	/* Now for the remaining rounds to 64: */
	do {
		ROUND256(a,b,c,d,e,f,g,h);
		ROUND256(h,a,b,c,d,e,f,g);
		ROUND256(g,h,a,b,c,d,e,f);
		ROUND256(f,g,h,a,b,c,d,e);
		ROUND256(e,f,g,h,a,b,c,d);
		ROUND256(d,e,f,g,h,a,b,c);
		ROUND256(c,d,e,f,g,h,a,b);
		ROUND256(b,c,d,e,f,g,h,a);
	} while (j < 64);

	/* Compute the current intermediate hash value */
	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
	context->state[5] += f;
	context->state[6] += g;
	context->state[7] += h;

	/* Clean up */
	a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

void SHA256_Transform(SHA256_CTX* context, const sha2_word32* data) {
	sha2_word32	a, b, c, d, e, f, g, h, s0, s1;
	sha2_word32	T1, T2, *W256;
	int		j;

	W256 = (sha2_word32*)context->buffer;

	/* Initialize registers with the prev. intermediate value */
	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];
	f = context->state[5];
	g = context->state[6];
	h = context->state[7];

	j = 0;
	do {
#ifndef WORDS_BIGENDIAN
		/* Copy data while converting to host byte order */
		REVERSE32(*data++,W256[j]);
		/* Apply the SHA-256 compression function to update a..h */
		T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + W256[j];
#else
		/* Apply the SHA-256 compression function to update a..h with copy */
		T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] + (W256[j] = *data++);
#endif
		T2 = Sigma0_256(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 16);

	do {
		/* Part of the message block expansion: */
		s0 = W256[(j+1)&0x0f];
		s0 = sigma0_256(s0);
		s1 = W256[(j+14)&0x0f];
		s1 = sigma1_256(s1);

		/* Apply the SHA-256 compression function to update a..h */
		T1 = h + Sigma1_256(e) + Ch(e, f, g) + K256[j] +
		     (W256[j&0x0f] += s1 + W256[(j+9)&0x0f] + s0);
		T2 = Sigma0_256(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 64);

	/* Compute the current intermediate hash value */
	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
	context->state[5] += f;
	context->state[6] += g;
	context->state[7] += h;

	/* Clean up */
	a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void SHA256_Update(SHA256_CTX* context, const sha2_byte *data, size_t len) {
	unsigned int	freespace, usedspace;

	if (len == 0) {
		/* Calling with no data is valid - we do nothing */
		return;
	}

	/* Sanity check: */
	assert(context != (SHA256_CTX*)0 && data != (sha2_byte*)0);

	usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
	if (usedspace > 0) {
		/* Calculate how much free space is available in the buffer */
		freespace = SHA256_BLOCK_LENGTH - usedspace;

		if (len >= freespace) {
			/* Fill the buffer completely and process it */
			MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
			context->bitcount += freespace << 3;
			len -= freespace;
			data += freespace;
			SHA256_Transform(context, (sha2_word32*)context->buffer);
		} else {
			/* The buffer is not yet full */
			MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
			context->bitcount += len << 3;
			/* Clean up: */
			usedspace = freespace = 0;
			return;
		}
	}
	while (len >= SHA256_BLOCK_LENGTH) {
		/* Process as many complete blocks as we can */
		SHA256_Transform(context, (sha2_word32*)data);
		context->bitcount += SHA256_BLOCK_LENGTH << 3;
		len -= SHA256_BLOCK_LENGTH;
		data += SHA256_BLOCK_LENGTH;
	}
	if (len > 0) {
		/* There's left-overs, so save 'em */
		MEMCPY_BCOPY(context->buffer, data, len);
		context->bitcount += len << 3;
	}
	/* Clean up: */
	usedspace = freespace = 0;
}

void SHA256_Final(sha2_byte digest[], SHA256_CTX* context) {
	sha2_word32	*d = (sha2_word32*)digest;
	unsigned int	usedspace;

	/* Sanity check: */
	assert(context != (SHA256_CTX*)0);

	/* If no digest buffer is passed, we don't bother doing this: */
	if (digest != (sha2_byte*)0) {
		usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
#ifndef WORDS_BIGENDIAN
		/* Convert FROM host byte order */
		REVERSE64(context->bitcount,context->bitcount);
#endif
		if (usedspace > 0) {
			/* Begin padding with a 1 bit: */
			context->buffer[usedspace++] = 0x80;

			if (usedspace <= SHA256_SHORT_BLOCK_LENGTH) {
				/* Set-up for the last transform: */
				MEMSET_BZERO(&context->buffer[usedspace], SHA256_SHORT_BLOCK_LENGTH - usedspace);
			} else {
				if (usedspace < SHA256_BLOCK_LENGTH) {
					MEMSET_BZERO(&context->buffer[usedspace], SHA256_BLOCK_LENGTH - usedspace);
				}
				/* Do second-to-last transform: */
				SHA256_Transform(context, (sha2_word32*)context->buffer);

				/* And set-up for the last transform: */
				MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);
			}
		} else {
			/* Set-up for the last transform: */
			MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);

			/* Begin padding with a 1 bit: */
			*context->buffer = 0x80;
		}
		/* Set the bit count: */
		*(sha2_word64*)&context->buffer[SHA256_SHORT_BLOCK_LENGTH] = context->bitcount;

		/* Final transform: */
		SHA256_Transform(context, (sha2_word32*)context->buffer);

#ifndef WORDS_BIGENDIAN
		{
			/* Convert TO host byte order */
			int	j;
			for (j = 0; j < 8; j++) {
				REVERSE32(context->state[j],context->state[j]);
				*d++ = context->state[j];
			}
		}
#else
		MEMCPY_BCOPY(d, context->state, SHA256_DIGEST_LENGTH);
#endif
	}

	/* Clean up state data: */
	MEMSET_BZERO(context, sizeof(context));
	usedspace = 0;
}

char *SHA256_End(SHA256_CTX* context, char buffer[]) {
	sha2_byte	digest[SHA256_DIGEST_LENGTH], *d = digest;
	int		i;

	/* Sanity check: */
	assert(context != (SHA256_CTX*)0);

	if (buffer != (char*)0) {
		SHA256_Final(digest, context);

		for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
			*buffer++ = sha2_hex_digits[(*d & 0xf0) >> 4];
			*buffer++ = sha2_hex_digits[*d & 0x0f];
			d++;
		}
		*buffer = (char)0;
	} else {
		MEMSET_BZERO(context, sizeof(context));
	}
	MEMSET_BZERO(digest, SHA256_DIGEST_LENGTH);
	return buffer;
}

char* SHA256_Data(const sha2_byte* data, size_t len, char digest[SHA256_DIGEST_STRING_LENGTH]) {
	SHA256_CTX	context;

	SHA256_Init(&context);
	SHA256_Update(&context, data, len);
	return SHA256_End(&context, digest);
}


/*** SHA-512: *********************************************************/
void SHA512_Init(SHA512_CTX* context) {
	if (context == (SHA512_CTX*)0) {
		return;
	}
	MEMCPY_BCOPY(context->state, sha512_initial_hash_value, SHA512_DIGEST_LENGTH);
	MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH);
	context->bitcount[0] = context->bitcount[1] =  0;
}

#ifdef SHA2_UNROLL_TRANSFORM

/* Unrolled SHA-512 round macros: */
#ifndef WORDS_BIGENDIAN

#define ROUND512_0_TO_15(a,b,c,d,e,f,g,h)	\
	REVERSE64(*data++, W512[j]); \
	T1 = (h) + Sigma1_512(e) + Ch((e), (f), (g)) + \
             K512[j] + W512[j]; \
	(d) += T1, \
	(h) = T1 + Sigma0_512(a) + Maj((a), (b), (c)), \
	j++


#else

#define ROUND512_0_TO_15(a,b,c,d,e,f,g,h)	\
	T1 = (h) + Sigma1_512(e) + Ch((e), (f), (g)) + \
             K512[j] + (W512[j] = *data++); \
	(d) += T1; \
	(h) = T1 + Sigma0_512(a) + Maj((a), (b), (c)); \
	j++

#endif

#define ROUND512(a,b,c,d,e,f,g,h)	\
	s0 = W512[(j+1)&0x0f]; \
	s0 = sigma0_512(s0); \
	s1 = W512[(j+14)&0x0f]; \
	s1 = sigma1_512(s1); \
	T1 = (h) + Sigma1_512(e) + Ch((e), (f), (g)) + K512[j] + \
             (W512[j&0x0f] += s1 + W512[(j+9)&0x0f] + s0); \
	(d) += T1; \
	(h) = T1 + Sigma0_512(a) + Maj((a), (b), (c)); \
	j++

void SHA512_Transform(SHA512_CTX* context, const sha2_word64* data) {
	sha2_word64	a, b, c, d, e, f, g, h, s0, s1;
	sha2_word64	T1, *W512 = (sha2_word64*)context->buffer;
	int		j;

	/* Initialize registers with the prev. intermediate value */
	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];
	f = context->state[5];
	g = context->state[6];
	h = context->state[7];

	j = 0;
	do {
		ROUND512_0_TO_15(a,b,c,d,e,f,g,h);
		ROUND512_0_TO_15(h,a,b,c,d,e,f,g);
		ROUND512_0_TO_15(g,h,a,b,c,d,e,f);
		ROUND512_0_TO_15(f,g,h,a,b,c,d,e);
		ROUND512_0_TO_15(e,f,g,h,a,b,c,d);
		ROUND512_0_TO_15(d,e,f,g,h,a,b,c);
		ROUND512_0_TO_15(c,d,e,f,g,h,a,b);
		ROUND512_0_TO_15(b,c,d,e,f,g,h,a);
	} while (j < 16);

	/* Now for the remaining rounds up to 79: */
	do {
		ROUND512(a,b,c,d,e,f,g,h);
		ROUND512(h,a,b,c,d,e,f,g);
		ROUND512(g,h,a,b,c,d,e,f);
		ROUND512(f,g,h,a,b,c,d,e);
		ROUND512(e,f,g,h,a,b,c,d);
		ROUND512(d,e,f,g,h,a,b,c);
		ROUND512(c,d,e,f,g,h,a,b);
		ROUND512(b,c,d,e,f,g,h,a);
	} while (j < 80);

	/* Compute the current intermediate hash value */
	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
	context->state[5] += f;
	context->state[6] += g;
	context->state[7] += h;

	/* Clean up */
	a = b = c = d = e = f = g = h = T1 = 0;
}

#else /* SHA2_UNROLL_TRANSFORM */

void SHA512_Transform(SHA512_CTX* context, const sha2_word64* data) {
	sha2_word64	a, b, c, d, e, f, g, h, s0, s1;
	sha2_word64	T1, T2, *W512 = (sha2_word64*)context->buffer;
	int		j;

	/* Initialize registers with the prev. intermediate value */
	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];
	f = context->state[5];
	g = context->state[6];
	h = context->state[7];

	j = 0;
	do {
#ifndef WORDS_BIGENDIAN
		/* Convert TO host byte order */
		REVERSE64(*data++, W512[j]);
		/* Apply the SHA-512 compression function to update a..h */
		T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] + W512[j];
#else
		/* Apply the SHA-512 compression function to update a..h with copy */
		T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] + (W512[j] = *data++);
#endif
		T2 = Sigma0_512(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 16);

	do {
		/* Part of the message block expansion: */
		s0 = W512[(j+1)&0x0f];
		s0 = sigma0_512(s0);
		s1 = W512[(j+14)&0x0f];
		s1 =  sigma1_512(s1);

		/* Apply the SHA-512 compression function to update a..h */
		T1 = h + Sigma1_512(e) + Ch(e, f, g) + K512[j] +
		     (W512[j&0x0f] += s1 + W512[(j+9)&0x0f] + s0);
		T2 = Sigma0_512(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 80);

	/* Compute the current intermediate hash value */
	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
	context->state[5] += f;
	context->state[6] += g;
	context->state[7] += h;

	/* Clean up */
	a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

#endif /* SHA2_UNROLL_TRANSFORM */

void SHA512_Update(SHA512_CTX* context, const sha2_byte *data, size_t len) {
	unsigned int	freespace, usedspace;

	if (len == 0) {
		/* Calling with no data is valid - we do nothing */
		return;
	}

	/* Sanity check: */
	assert(context != (SHA512_CTX*)0 && data != (sha2_byte*)0);

	usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
	if (usedspace > 0) {
		/* Calculate how much free space is available in the buffer */
		freespace = SHA512_BLOCK_LENGTH - usedspace;

		if (len >= freespace) {
			/* Fill the buffer completely and process it */
			MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
			ADDINC128(context->bitcount, freespace << 3);
			len -= freespace;
			data += freespace;
			SHA512_Transform(context, (sha2_word64*)context->buffer);
		} else {
			/* The buffer is not yet full */
			MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
			ADDINC128(context->bitcount, len << 3);
			/* Clean up: */
			usedspace = freespace = 0;
			return;
		}
	}
	while (len >= SHA512_BLOCK_LENGTH) {
		/* Process as many complete blocks as we can */
		SHA512_Transform(context, (sha2_word64*)data);
		ADDINC128(context->bitcount, SHA512_BLOCK_LENGTH << 3);
		len -= SHA512_BLOCK_LENGTH;
		data += SHA512_BLOCK_LENGTH;
	}
	if (len > 0) {
		/* There's left-overs, so save 'em */
		MEMCPY_BCOPY(context->buffer, data, len);
		ADDINC128(context->bitcount, len << 3);
	}
	/* Clean up: */
	usedspace = freespace = 0;
}

void SHA512_Last(SHA512_CTX* context) {
	unsigned int	usedspace;

	usedspace = (context->bitcount[0] >> 3) % SHA512_BLOCK_LENGTH;
#ifndef WORDS_BIGENDIAN
	/* Convert FROM host byte order */
	REVERSE64(context->bitcount[0],context->bitcount[0]);
	REVERSE64(context->bitcount[1],context->bitcount[1]);
#endif
	if (usedspace > 0) {
		/* Begin padding with a 1 bit: */
		context->buffer[usedspace++] = 0x80;

		if (usedspace <= SHA512_SHORT_BLOCK_LENGTH) {
			/* Set-up for the last transform: */
			MEMSET_BZERO(&context->buffer[usedspace], SHA512_SHORT_BLOCK_LENGTH - usedspace);
		} else {
			if (usedspace < SHA512_BLOCK_LENGTH) {
				MEMSET_BZERO(&context->buffer[usedspace], SHA512_BLOCK_LENGTH - usedspace);
			}
			/* Do second-to-last transform: */
			SHA512_Transform(context, (sha2_word64*)context->buffer);

			/* And set-up for the last transform: */
			MEMSET_BZERO(context->buffer, SHA512_BLOCK_LENGTH - 2);
		}
	} else {
		/* Prepare for final transform: */
		MEMSET_BZERO(context->buffer, SHA512_SHORT_BLOCK_LENGTH);

		/* Begin padding with a 1 bit: */
		*context->buffer = 0x80;
	}
	/* Store the length of input data (in bits): */
	*(sha2_word64*)&context->buffer[SHA512_SHORT_BLOCK_LENGTH] = context->bitcount[1];
	*(sha2_word64*)&context->buffer[SHA512_SHORT_BLOCK_LENGTH+8] = context->bitcount[0];

	/* Final transform: */
	SHA512_Transform(context, (sha2_word64*)context->buffer);
}

void SHA512_Final(sha2_byte digest[], SHA512_CTX* context) {
	sha2_word64	*d = (sha2_word64*)digest;

	/* Sanity check: */
	assert(context != (SHA512_CTX*)0);

	/* If no digest buffer is passed, we don't bother doing this: */
	if (digest != (sha2_byte*)0) {
		SHA512_Last(context);

		/* Save the hash data for output: */
#ifndef WORDS_BIGENDIAN
		{
			/* Convert TO host byte order */
			int	j;
			for (j = 0; j < 8; j++) {
				REVERSE64(context->state[j],context->state[j]);
				*d++ = context->state[j];
			}
		}
#else
		MEMCPY_BCOPY(d, context->state, SHA512_DIGEST_LENGTH);
#endif
	}

	/* Zero out state data */
	MEMSET_BZERO(context, sizeof(context));
}

char *SHA512_End(SHA512_CTX* context, char buffer[]) {
	sha2_byte	digest[SHA512_DIGEST_LENGTH], *d = digest;
	int		i;

	/* Sanity check: */
	assert(context != (SHA512_CTX*)0);

	if (buffer != (char*)0) {
		SHA512_Final(digest, context);

		for (i = 0; i < SHA512_DIGEST_LENGTH; i++) {
			*buffer++ = sha2_hex_digits[(*d & 0xf0) >> 4];
			*buffer++ = sha2_hex_digits[*d & 0x0f];
			d++;
		}
		*buffer = (char)0;
	} else {
		MEMSET_BZERO(context, sizeof(context));
	}
	MEMSET_BZERO(digest, SHA512_DIGEST_LENGTH);
	return buffer;
}

char* SHA512_Data(const sha2_byte* data, size_t len, char digest[SHA512_DIGEST_STRING_LENGTH]) {
	SHA512_CTX	context;

	SHA512_Init(&context);
	SHA512_Update(&context, data, len);
	return SHA512_End(&context, digest);
}


/*** SHA-384: *********************************************************/
void SHA384_Init(SHA384_CTX* context) {
	if (context == (SHA384_CTX*)0) {
		return;
	}
	MEMCPY_BCOPY(context->state, sha384_initial_hash_value, SHA512_DIGEST_LENGTH);
	MEMSET_BZERO(context->buffer, SHA384_BLOCK_LENGTH);
	context->bitcount[0] = context->bitcount[1] = 0;
}

void SHA384_Update(SHA384_CTX* context, const sha2_byte* data, size_t len) {
	SHA512_Update((SHA512_CTX*)context, data, len);
}

void SHA384_Final(sha2_byte digest[], SHA384_CTX* context) {
	sha2_word64	*d = (sha2_word64*)digest;

	/* Sanity check: */
	assert(context != (SHA384_CTX*)0);

	/* If no digest buffer is passed, we don't bother doing this: */
	if (digest != (sha2_byte*)0) {
		SHA512_Last((SHA512_CTX*)context);

		/* Save the hash data for output: */
#ifndef WORDS_BIGENDIAN
		{
			/* Convert TO host byte order */
			int	j;
			for (j = 0; j < 6; j++) {
				REVERSE64(context->state[j],context->state[j]);
				*d++ = context->state[j];
			}
		}
#else
		MEMCPY_BCOPY(d, context->state, SHA384_DIGEST_LENGTH);
#endif
	}

	/* Zero out state data */
	MEMSET_BZERO(context, sizeof(context));
}

char *SHA384_End(SHA384_CTX* context, char buffer[]) {
	sha2_byte	digest[SHA384_DIGEST_LENGTH], *d = digest;
	int		i;

	/* Sanity check: */
	assert(context != (SHA384_CTX*)0);

	if (buffer != (char*)0) {
		SHA384_Final(digest, context);

		for (i = 0; i < SHA384_DIGEST_LENGTH; i++) {
			*buffer++ = sha2_hex_digits[(*d & 0xf0) >> 4];
			*buffer++ = sha2_hex_digits[*d & 0x0f];
			d++;
		}
		*buffer = (char)0;
	} else {
		MEMSET_BZERO(context, sizeof(context));
	}
	MEMSET_BZERO(digest, SHA384_DIGEST_LENGTH);
	return buffer;
}

char* SHA384_Data(const sha2_byte* data, size_t len, char digest[SHA384_DIGEST_STRING_LENGTH]) {
	SHA384_CTX	context;

	SHA384_Init(&context);
	SHA384_Update(&context, data, len);
	return SHA384_End(&context, digest);
}
