/*
 * dlls/rsaenh/tomcrypt.h
 * Function prototypes, type definitions and constant definitions
 * for LibTomCrypt code.
 *
 * Copyright 2004 Michael Jung
 * Based on public domain code by Tom St Denis (tomstdenis@iahu.ca)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * This file contains code from the LibTomCrypt cryptographic 
 * library written by Tom St Denis (tomstdenis@iahu.ca). LibTomCrypt
 * is in the public domain. The code in this file is tailored to
 * special requirements. Take a look at http://libtomcrypt.org for the
 * original version. 
 */

#ifndef __WINE_TOMCRYPT_H_
#define __WINE_TOMCRYPT_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "basetsd.h"

/* error codes [will be expanded in future releases] */
enum {
   CRYPT_OK=0,             /* Result OK */
   CRYPT_ERROR,            /* Generic Error */
   CRYPT_NOP,              /* Not a failure but no operation was performed */

   CRYPT_INVALID_KEYSIZE,  /* Invalid key size given */
   CRYPT_INVALID_ROUNDS,   /* Invalid number of rounds */
   CRYPT_FAIL_TESTVECTOR,  /* Algorithm failed test vectors */

   CRYPT_BUFFER_OVERFLOW,  /* Not enough space for output */
   CRYPT_INVALID_PACKET,   /* Invalid input packet given */

   CRYPT_INVALID_PRNGSIZE, /* Invalid number of bits for a PRNG */
   CRYPT_ERROR_READPRNG,   /* Could not read enough from PRNG */

   CRYPT_INVALID_CIPHER,   /* Invalid cipher specified */
   CRYPT_INVALID_HASH,     /* Invalid hash specified */
   CRYPT_INVALID_PRNG,     /* Invalid PRNG specified */

   CRYPT_MEM,              /* Out of memory */

   CRYPT_PK_TYPE_MISMATCH, /* Not equivalent types of PK keys */
   CRYPT_PK_NOT_PRIVATE,   /* Requires a private PK key */

   CRYPT_INVALID_ARG,      /* Generic invalid argument */
   CRYPT_FILE_NOTFOUND,    /* File Not Found */

   CRYPT_PK_INVALID_TYPE,  /* Invalid type of PK key */
   CRYPT_PK_INVALID_SYSTEM,/* Invalid PK system specified */
   CRYPT_PK_DUP,           /* Duplicate key already in key ring */
   CRYPT_PK_NOT_FOUND,     /* Key not found in keyring */
   CRYPT_PK_INVALID_SIZE,  /* Invalid size input for PK parameters */

   CRYPT_INVALID_PRIME_SIZE/* Invalid size of prime requested */
};

#define CONST64(a,b) ((((ULONG64)(a)) << 32) | (b))
typedef ULONG64 ulong64;

/* this is the "32-bit at least" data type 
 * Re-define it to suit your platform but it must be at least 32-bits 
 */
typedef ULONG32 ulong32;

/* ---- HELPER MACROS ---- */
#define STORE32H(x, y)                                                                     \
     { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned char)(((x)>>16)&255);   \
       (y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned char)((x)&255); }

#define LOAD32H(x, y)                            \
     { x = ((unsigned long)((y)[0] & 255)<<24) | \
           ((unsigned long)((y)[1] & 255)<<16) | \
           ((unsigned long)((y)[2] & 255)<<8)  | \
           ((unsigned long)((y)[3] & 255)); }

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)) && !defined(INTEL_CC)

static inline unsigned ROR(unsigned word, int i)
{
   __asm__("rorl %%cl,%0"
      :"=r" (word)
      :"0" (word),"c" (i));
   return word;
}

#else

/* rotates the hard way */
#define ROR(x, y) ( ((((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)((y)&31)) | \
                    ((unsigned long)(x)<<(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)

#endif

#undef MIN
#define MIN(x, y) ( ((x)<(y))?(x):(y) )

#define byte(x, n) (((x) >> (8 * (n))) & 255)

typedef struct tag_rc2_key { 
	unsigned xkey[64]; 
} rc2_key;

typedef struct tag_des_key {
    ulong32 ek[32], dk[32];
} des_key;

typedef struct tag_des3_key {
    ulong32 ek[3][32], dk[3][32];
} des3_key;

typedef struct tag_aes_key {
   ulong32 eK[64], dK[64];
   int Nr;
} aes_key;

int rc2_setup(const unsigned char *key, int keylen, int bits, int num_rounds, rc2_key *skey);
void rc2_ecb_encrypt(const unsigned char *pt, unsigned char *ct, rc2_key *key);
void rc2_ecb_decrypt(const unsigned char *ct, unsigned char *pt, rc2_key *key);

int des_setup(const unsigned char *key, int keylen, int num_rounds, des_key *skey);
void des_ecb_encrypt(const unsigned char *pt, unsigned char *ct, const des_key *key);
void des_ecb_decrypt(const unsigned char *ct, unsigned char *pt, const des_key *key);

int des3_setup(const unsigned char *key, int keylen, int num_rounds, des3_key *skey);
void des3_ecb_encrypt(const unsigned char *pt, unsigned char *ct, const des3_key *key);
void des3_ecb_decrypt(const unsigned char *ct, unsigned char *pt, const des3_key *key);

int aes_setup(const unsigned char *key, int keylen, int rounds, aes_key *skey);
void aes_ecb_encrypt(const unsigned char *pt, unsigned char *ct, aes_key *skey);
void aes_ecb_decrypt(const unsigned char *ct, unsigned char *pt, aes_key *skey);

typedef struct tag_md2_state {
    unsigned char chksum[16], X[48], buf[16];
    unsigned long curlen;
} md2_state;

int md2_init(md2_state * md);
int md2_process(md2_state * md, const unsigned char *buf, unsigned long len);
int md2_done(md2_state * md, unsigned char *hash);

struct rc4_prng {
    int x, y;
    unsigned char buf[256];
};

typedef union Prng_state {
    struct rc4_prng       rc4;
} prng_state;

int rc4_start(prng_state *prng);
int rc4_add_entropy(const unsigned char *buf, unsigned long len, prng_state *prng);
int rc4_ready(prng_state *prng);
unsigned long rc4_read(unsigned char *buf, unsigned long len, prng_state *prng);

/* some default configurations.
 *
 * A "mp_digit" must be able to hold DIGIT_BIT + 1 bits
 * A "mp_word" must be able to hold 2*DIGIT_BIT + 1 bits
 *
 * At the very least a mp_digit must be able to hold 7 bits
 * [any size beyond that is ok provided it doesn't overflow the data type]
 */
typedef unsigned long      mp_digit;
typedef ulong64            mp_word;
#define DIGIT_BIT 28
   
#define MP_DIGIT_BIT     DIGIT_BIT
#define MP_MASK          ((((mp_digit)1)<<((mp_digit)DIGIT_BIT))-((mp_digit)1))
#define MP_DIGIT_MAX     MP_MASK

/* equalities */
#define MP_LT        -1   /* less than */
#define MP_EQ         0   /* equal to */
#define MP_GT         1   /* greater than */

#define MP_ZPOS       0   /* positive integer */
#define MP_NEG        1   /* negative */

#define MP_OKAY       0   /* ok result */
#define MP_MEM        -2  /* out of mem */
#define MP_VAL        -3  /* invalid input */
#define MP_RANGE      MP_VAL

#define MP_YES        1   /* yes response */
#define MP_NO         0   /* no response */

/* Primality generation flags */
#define LTM_PRIME_BBS      0x0001 /* BBS style prime */
#define LTM_PRIME_SAFE     0x0002 /* Safe prime (p-1)/2 == prime */
#define LTM_PRIME_2MSB_OFF 0x0004 /* force 2nd MSB to 0 */
#define LTM_PRIME_2MSB_ON  0x0008 /* force 2nd MSB to 1 */

typedef int           mp_err;

/* define this to use lower memory usage routines (exptmods mostly) */
/* #define MP_LOW_MEM */

#define MP_PREC                 64     /* default digits of precision */

/* size of comba arrays, should be at least 2 * 2**(BITS_PER_WORD - BITS_PER_DIGIT*2) */
#define MP_WARRAY               (1 << (sizeof(mp_word) * CHAR_BIT - 2 * DIGIT_BIT + 1))

/* the infamous mp_int structure */
typedef struct  {
    int used, alloc, sign;
    mp_digit *dp;
} mp_int;

/* callback for mp_prime_random, should fill dst with random bytes and return how many read [up to len] */
typedef int ltm_prime_callback(unsigned char *dst, int len, void *dat);

#define DIGIT(m,k) ((m)->dp[(k)])

/* error code to char* string */
char *mp_error_to_string(int code);

/* init a null terminated series of arguments */
int mp_init_multi(mp_int *mp, ...);

/* clear a null terminated series of arguments */
void mp_clear_multi(mp_int *mp, ...);

/* shrink ram required for a bignum */
int mp_shrink(mp_int *a);

/* ---> Basic Manipulations <--- */
#define mp_iszero(a) (((a)->used == 0) ? MP_YES : MP_NO)
#define mp_iseven(a) (((a)->used > 0 && (((a)->dp[0] & 1) == 0)) ? MP_YES : MP_NO)
#define mp_isodd(a)  (((a)->used > 0 && (((a)->dp[0] & 1) == 1)) ? MP_YES : MP_NO)

/* set to a digit */
void mp_set(mp_int *a, mp_digit b);

/* set a 32-bit const */
int mp_set_int(mp_int *a, unsigned long b);

/* get a 32-bit value */
unsigned long mp_get_int(const mp_int * a);

/* initialize and set a digit */
int mp_init_set (mp_int * a, mp_digit b);

/* initialize and set 32-bit value */
int mp_init_set_int (mp_int * a, unsigned long b);

/* copy, b = a */
int mp_copy(const mp_int *a, mp_int *b);

/* inits and copies, a = b */
int mp_init_copy(mp_int *a, const mp_int *b);

/* trim unused digits */
void mp_clamp(mp_int *a);

/* ---> digit manipulation <--- */

/* Counts the number of lsbs which are zero before the first zero bit */
int mp_cnt_lsb(const mp_int *a);

/* I Love Earth! */

/* makes a pseudo-random int of a given size */
int mp_rand(mp_int *a, int digits);

/* ---> binary operations <--- */
/* c = a XOR b  */
int mp_xor(mp_int *a, mp_int *b, mp_int *c);

/* c = a OR b */
int mp_or(mp_int *a, mp_int *b, mp_int *c);

/* c = a AND b */
int mp_and(mp_int *a, mp_int *b, mp_int *c);

/* ---> Basic arithmetic <--- */

/* b = -a */
int mp_neg(mp_int *a, mp_int *b);

/* compare a to b */
int mp_cmp(const mp_int *a, const mp_int *b);

/* compare |a| to |b| */
int mp_cmp_mag(const mp_int *a, const mp_int *b);

/* c = a + b */
int mp_add(mp_int *a, mp_int *b, mp_int *c);

/* c = a - b */
int mp_sub(mp_int *a, mp_int *b, mp_int *c);

/* c = a * b */
int mp_mul(const mp_int *a, const mp_int *b, mp_int *c);

/* b = a*a  */
int mp_sqr(const mp_int *a, mp_int *b);

/* c = a mod b, 0 <= c < b  */
int mp_mod(const mp_int *a, mp_int *b, mp_int *c);

/* ---> single digit functions <--- */

/* compare against a single digit */
int mp_cmp_d(const mp_int *a, mp_digit b);

/* c = a - b */
int mp_sub_d(mp_int *a, mp_digit b, mp_int *c);

/* a/3 => 3c + d == a */
int mp_div_3(mp_int *a, mp_int *c, mp_digit *d);

/* c = a**b */
int mp_expt_d(mp_int *a, mp_digit b, mp_int *c);

/* ---> number theory <--- */

/* d = a + b (mod c) */
int mp_addmod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);

/* d = a - b (mod c) */
int mp_submod(mp_int *a, mp_int *b, mp_int *c, mp_int *d);

/* d = a * b (mod c) */
int mp_mulmod(const mp_int *a, const mp_int *b, mp_int *c, mp_int *d);

/* c = a * a (mod b) */
int mp_sqrmod(const mp_int *a, mp_int *b, mp_int *c);

/* c = 1/a (mod b) */
int mp_invmod(const mp_int *a, mp_int *b, mp_int *c);

/* c = (a, b) */
int mp_gcd(const mp_int *a, const mp_int *b, mp_int *c);

/* produces value such that U1*a + U2*b = U3 */
int mp_exteuclid(mp_int *a, mp_int *b, mp_int *U1, mp_int *U2, mp_int *U3);

/* c = [a, b] or (a*b)/(a, b) */
int mp_lcm(const mp_int *a, const mp_int *b, mp_int *c);

/* finds one of the b'th root of a, such that |c|**b <= |a|
 *
 * returns error if a < 0 and b is even
 */
int mp_n_root(mp_int *a, mp_digit b, mp_int *c);

/* special sqrt algo */
int mp_sqrt(mp_int *arg, mp_int *ret);

/* is number a square? */
int mp_is_square(mp_int *arg, int *ret);

/* computes the jacobi c = (a | n) (or Legendre if b is prime)  */
int mp_jacobi(mp_int *a, mp_int *n, int *c);

/* used to setup the Barrett reduction for a given modulus b */
int mp_reduce_setup(mp_int *a, const mp_int *b);

/* Barrett Reduction, computes a (mod b) with a precomputed value c
 *
 * Assumes that 0 < a <= b*b, note if 0 > a > -(b*b) then you can merely
 * compute the reduction as -1 * mp_reduce(mp_abs(a)) [pseudo code].
 */
int mp_reduce(mp_int *a, const mp_int *b, const mp_int *c);

/* setups the montgomery reduction */
int mp_montgomery_setup(const mp_int *a, mp_digit *mp);

/* computes a = B**n mod b without division or multiplication useful for
 * normalizing numbers in a Montgomery system.
 */
int mp_montgomery_calc_normalization(mp_int *a, const mp_int *b);

/* computes x/R == x (mod N) via Montgomery Reduction */
int mp_montgomery_reduce(mp_int *a, const mp_int *m, mp_digit mp);

/* returns 1 if a is a valid DR modulus */
int mp_dr_is_modulus(mp_int *a);

/* returns true if a can be reduced with mp_reduce_2k */
int mp_reduce_is_2k(mp_int *a);

/* determines k value for 2k reduction */
int mp_reduce_2k_setup(const mp_int *a, mp_digit *d);

/* reduces a modulo b where b is of the form 2**p - k [0 <= a] */
int mp_reduce_2k(mp_int *a, const mp_int *n, mp_digit d);

/* d = a**b (mod c) */
int mp_exptmod(const mp_int *a, const mp_int *b, mp_int *c, mp_int *d);

/* ---> Primes <--- */

/* number of primes */
#define PRIME_SIZE      256

/* performs one Fermat test of "a" using base "b".
 * Sets result to 0 if composite or 1 if probable prime
 */
int mp_prime_fermat(mp_int *a, mp_int *b, int *result);

/* This gives [for a given bit size] the number of trials required
 * such that Miller-Rabin gives a prob of failure lower than 2^-96 
 */
int mp_prime_rabin_miller_trials(int size);

/* finds the next prime after the number "a" using "t" trials
 * of Miller-Rabin.
 *
 * bbs_style = 1 means the prime must be congruent to 3 mod 4
 */
int mp_prime_next_prime(mp_int *a, int t, int bbs_style);

/* makes a truly random prime of a given size (bytes),
 * call with bbs = 1 if you want it to be congruent to 3 mod 4 
 *
 * You have to supply a callback which fills in a buffer with random bytes.  "dat" is a parameter you can
 * have passed to the callback (e.g. a state or something).  This function doesn't use "dat" itself
 * so it can be NULL
 *
 * The prime generated will be larger than 2^(8*size).
 */
#define mp_prime_random(a, t, size, bbs, cb, dat) mp_prime_random_ex(a, t, ((size) * 8) + 1, (bbs==1)?LTM_PRIME_BBS:0, cb, dat)

/* makes a truly random prime of a given size (bits),
 *
 * Flags are as follows:
 * 
 *   LTM_PRIME_BBS      - make prime congruent to 3 mod 4
 *   LTM_PRIME_SAFE     - make sure (p-1)/2 is prime as well (implies LTM_PRIME_BBS)
 *   LTM_PRIME_2MSB_OFF - make the 2nd highest bit zero
 *   LTM_PRIME_2MSB_ON  - make the 2nd highest bit one
 *
 * You have to supply a callback which fills in a buffer with random bytes.  "dat" is a parameter you can
 * have passed to the callback (e.g. a state or something).  This function doesn't use "dat" itself
 * so it can be NULL
 *
 */
int mp_prime_random_ex(mp_int *a, int t, int size, int flags, ltm_prime_callback cb, void *dat);

/* ---> radix conversion <--- */
int mp_count_bits(const mp_int *a);

int mp_unsigned_bin_size(const mp_int *a);
int mp_read_unsigned_bin(mp_int *a, const unsigned char *b, int c);
int mp_to_unsigned_bin(const mp_int *a, unsigned char *b);

int mp_signed_bin_size(const mp_int *a);
int mp_read_signed_bin(mp_int *a, unsigned char *b, int c);
int mp_to_signed_bin(mp_int *a, unsigned char *b);

int mp_read_radix(mp_int *a, char *str, int radix);
int mp_toradix(mp_int *a, char *str, int radix);
int mp_toradix_n(mp_int * a, char *str, int radix, int maxlen);
int mp_radix_size(mp_int *a, int radix, int *size);

int mp_fread(mp_int *a, int radix, FILE *stream);
int mp_fwrite(mp_int *a, int radix, FILE *stream);

#define mp_read_raw(mp, str, len) mp_read_signed_bin((mp), (str), (len))
#define mp_raw_size(mp)           mp_signed_bin_size(mp)
#define mp_toraw(mp, str)         mp_to_signed_bin((mp), (str))
#define mp_read_mag(mp, str, len) mp_read_unsigned_bin((mp), (str), (len))
#define mp_mag_size(mp)           mp_unsigned_bin_size(mp)
#define mp_tomag(mp, str)         mp_to_unsigned_bin((mp), (str))

#define mp_tobinary(M, S)  mp_toradix((M), (S), 2)
#define mp_tooctal(M, S)   mp_toradix((M), (S), 8)
#define mp_todecimal(M, S) mp_toradix((M), (S), 10)
#define mp_tohex(M, S)     mp_toradix((M), (S), 16)

extern const char *mp_s_rmap;

#define PK_PRIVATE            0        /* PK private keys */
#define PK_PUBLIC             1        /* PK public keys */

/* Min and Max RSA key sizes (in bits) */
#define MIN_RSA_SIZE 384
#define MAX_RSA_SIZE 16384

typedef struct Rsa_key {
    int type;
    mp_int e, d, N, p, q, qP, dP, dQ;
} rsa_key;

int rsa_make_key(int size, long e, rsa_key *key);

int rsa_exptmod(const unsigned char *in,   unsigned long inlen,
                      unsigned char *out,  unsigned long *outlen, int which,
                      rsa_key *key);

void rsa_free(rsa_key *key);

#endif /* __WINE_TOMCRYPT_H_ */
