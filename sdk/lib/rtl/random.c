/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Random number generator functions
 * FILE:              lib/rtl/random.c
 * PROGRAMMER:        Wine
 *                    Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

static ULONG RtlpRandomConstantVector[128] =
{
    0x4c8bc0aa, 0x4c022957, 0x2232827a, 0x2f1e7626,  /*   0 */
    0x7f8bdafb, 0x5c37d02a, 0x0ab48f72, 0x2f0c4ffa,  /*   4 */
    0x290e1954, 0x6b635f23, 0x5d3885c0, 0x74b49ff8,  /*   8 */
    0x5155fa54, 0x6214ad3f, 0x111e9c29, 0x242a3a09,  /*  12 */
    0x75932ae1, 0x40ac432e, 0x54f7ba7a, 0x585ccbd5,  /*  16 */
    0x6df5c727, 0x0374dad1, 0x7112b3f1, 0x735fc311,  /*  20 */
    0x404331a9, 0x74d97781, 0x64495118, 0x323e04be,  /*  24 */
    0x5974b425, 0x4862e393, 0x62389c1d, 0x28a68b82,  /*  28 */
    0x0f95da37, 0x7a50bbc6, 0x09b0091c, 0x22cdb7b4,  /*  32 */
    0x4faaed26, 0x66417ccd, 0x189e4bfa, 0x1ce4e8dd,  /*  36 */
    0x5274c742, 0x3bdcf4dc, 0x2d94e907, 0x32eac016,  /*  40 */
    0x26d33ca3, 0x60415a8a, 0x31f57880, 0x68c8aa52,  /*  44 */
    0x23eb16da, 0x6204f4a1, 0x373927c1, 0x0d24eb7c,  /*  48 */
    0x06dd7379, 0x2b3be507, 0x0f9c55b1, 0x2c7925eb,  /*  52 */
    0x36d67c9a, 0x42f831d9, 0x5e3961cb, 0x65d637a8,  /*  56 */
    0x24bb3820, 0x4d08e33d, 0x2188754f, 0x147e409e,  /*  60 */
    0x6a9620a0, 0x62e26657, 0x7bd8ce81, 0x11da0abb,  /*  64 */
    0x5f9e7b50, 0x23e444b6, 0x25920c78, 0x5fc894f0,  /*  68 */
    0x5e338cbb, 0x404237fd, 0x1d60f80f, 0x320a1743,  /*  72 */
    0x76013d2b, 0x070294ee, 0x695e243b, 0x56b177fd,  /*  76 */
    0x752492e1, 0x6decd52f, 0x125f5219, 0x139d2e78,  /*  80 */
    0x1898d11e, 0x2f7ee785, 0x4db405d8, 0x1a028a35,  /*  84 */
    0x63f6f323, 0x1f6d0078, 0x307cfd67, 0x3f32a78a,  /*  88 */
    0x6980796c, 0x462b3d83, 0x34b639f2, 0x53fce379,  /*  92 */
    0x74ba50f4, 0x1abc2c4b, 0x5eeaeb8d, 0x335a7a0d,  /*  96 */
    0x3973dd20, 0x0462d66b, 0x159813ff, 0x1e4643fd,  /* 100 */
    0x06bc5c62, 0x3115e3fc, 0x09101613, 0x47af2515,  /* 104 */
    0x4f11ec54, 0x78b99911, 0x3db8dd44, 0x1ec10b9b,  /* 108 */
    0x5b5506ca, 0x773ce092, 0x567be81a, 0x5475b975,  /* 112 */
    0x7a2cde1a, 0x494536f5, 0x34737bb4, 0x76d9750b,  /* 116 */
    0x2a1f6232, 0x2e49644d, 0x7dddcbe7, 0x500cebdb,  /* 120 */
    0x619dab9e, 0x48c626fe, 0x1cda3193, 0x52dabe9d   /* 124 */
};

static ULONG RtlpRandomExConstantVector[128] =
{
    0x4c8bc0aa, 0x51a0b326, 0x7112b3f1, 0x1b9ca4e1,  /*   0 */
    0x735fc311, 0x6fe48580, 0x320a1743, 0x494045ca,  /*   4 */
    0x103ad1c5, 0x4ba26e25, 0x62f1d304, 0x280d5677,  /*   8 */
    0x070294ee, 0x7acef21a, 0x62a407d5, 0x2dd36af5,  /*  12 */
    0x194f0f95, 0x1f21d7b4, 0x307cfd67, 0x66b9311e,  /*  16 */
    0x60415a8a, 0x5b264785, 0x3c28b0e4, 0x08faded7,  /*  20 */
    0x556175ce, 0x29c44179, 0x666f23c9, 0x65c057d8,  /*  24 */
    0x72b97abc, 0x7c3be3d0, 0x478e1753, 0x3074449b,  /*  28 */
    0x675ee842, 0x53f4c2de, 0x44d58949, 0x6426cf59,  /*  32 */
    0x111e9c29, 0x3aba68b9, 0x242a3a09, 0x50ddb118,  /*  36 */
    0x7f8bdafb, 0x089ebf23, 0x5c37d02a, 0x27db8ca6,  /*  40 */
    0x0ab48f72, 0x34995a4e, 0x189e4bfa, 0x2c405c36,  /*  44 */
    0x373927c1, 0x66c20c71, 0x5f991360, 0x67a38fa3,  /*  48 */
    0x4edc56aa, 0x25a59126, 0x34b639f2, 0x1679b2ce,  /*  52 */
    0x54f7ba7a, 0x319d28b5, 0x5155fa54, 0x769e6b87,  /*  56 */
    0x323e04be, 0x4565a5aa, 0x5974b425, 0x5c56a104,  /*  60 */
    0x25920c78, 0x362912dc, 0x7af3996f, 0x5feb9c87,  /*  64 */
    0x618361bf, 0x433fbe97, 0x0244da8e, 0x54e3c739,  /*  68 */
    0x33183689, 0x3533f398, 0x0d24eb7c, 0x06428590,  /*  72 */
    0x09101613, 0x53ce5c5a, 0x47af2515, 0x2e003f35,  /*  76 */
    0x15fb4ed5, 0x5e5925f4, 0x7f622ea7, 0x0bb6895f,  /*  80 */
    0x2173cdb6, 0x0467bb41, 0x2c4d19f1, 0x364712e1,  /*  84 */
    0x78b99911, 0x0a39a380, 0x3db8dd44, 0x6b4793b8,  /*  88 */
    0x09b0091c, 0x47ef52b0, 0x293cdcb3, 0x707b9e7b,  /*  92 */
    0x26d33ca3, 0x1e527faa, 0x3fe08625, 0x42560b04,  /*  96 */
    0x139d2e78, 0x0b558cdb, 0x28a68b82, 0x7ba3a51d,  /* 100 */
    0x52dabe9d, 0x59c3da1d, 0x5676cf9c, 0x152e972f,  /* 104 */
    0x6d8ac746, 0x5eb33591, 0x78b30601, 0x0ab68db0,  /* 108 */
    0x34737bb4, 0x1b6dd168, 0x76d9750b, 0x2ddc4ff2,  /* 112 */
    0x18a610cd, 0x2bacc08c, 0x422db55f, 0x169b89b6,  /* 116 */
    0x5274c742, 0x615535dd, 0x46ad005d, 0x4128f8dd,  /* 120 */
    0x29f5875c, 0x62c6f3ef, 0x2b3be507, 0x4a8e003f   /* 124 */
};

static ULONG RtlpRandomExAuxVarY = 0x7775fb16;

#define LCG_A 0x7fffffed
#define LCG_C 0x7fffffc3
#define LCG_M MAXLONG

/*************************************************************************
 * RtlRandom   [NTDLL.@]
 *
 * Generates a random number. This is based on a LCG.
 * This makes it not suitable for Monte Carlo simulations nor
 * cryptographic applications.
 * See LCG_A, LCG_C, LCG_M for parameters.
 *
 * PARAMS
 *  Seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number distributed over [0..MAXLONG-1].
 */
/*
* @implemented
*/
ULONG NTAPI
RtlRandom (IN OUT PULONG Seed)
{
   ULONG Rand;
   int Pos;
   ULONG Result;

   PAGED_CODE_RTL();

   Rand = (*Seed * LCG_A + LCG_C) % LCG_M;
   *Seed = (Rand * LCG_A + LCG_C) % LCG_M;
   Pos = *Seed & (sizeof(RtlpRandomConstantVector) / sizeof(RtlpRandomConstantVector[0]) - 1);
   Result = RtlpRandomConstantVector[Pos];
   RtlpRandomConstantVector[Pos] = Rand;

   return Result;
}

/*************************************************************************
 * RtlRandomEx   [NTDLL.@]
 *
 * Generates a random number. This is based on a LCG.
 * This makes it not suitable for Monte Carlo simulations nor
 * cryptographic applications.
 * This one is faster than RtlRandom.
 * See LCG_A, LCG_C, LCG_M for parameters.
 *
 * PARAMS
 *  Seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number distributed over [0..MAXLONG-1].
 */
/*
* @implemented
*/
ULONG
NTAPI
RtlRandomEx( IN OUT PULONG Seed
	)
{
   ULONG Rand;
   int Pos;

   PAGED_CODE_RTL();

   Pos = RtlpRandomExAuxVarY & (sizeof(RtlpRandomExConstantVector) / sizeof(RtlpRandomExConstantVector[0]) - 1);
   RtlpRandomExAuxVarY = RtlpRandomExConstantVector[Pos];
   Rand = (*Seed * LCG_A + LCG_C) % LCG_M;
   RtlpRandomExConstantVector[Pos] = Rand;
   *Seed = Rand;

   return Rand;
}



/*************************************************************************
 * RtlUniform   [NTDLL.@]
 *
 * Generates an uniform random number
 *
 * PARAMS
 *  Seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number uniformly distributed over [0..MAXLONG-1].
 *
 * NOTES
 *  Generates an uniform random number using LCG. In spite of what
 *  MSDN states, this is an LCG and not a Lehmer RNG since C is defined.
 *  This makes it not suitable for Monte Carlo simulations nor
 *  cryptographic applications.
 *  See LCG_A, LCG_C, LCG_M for parameters.
 *
 * DIFFERENCES
 *  The native documentation states that the random number is
 *  uniformly distributed over [0..MAXLONG]. In reality the native
 *  function and our function return a random number uniformly
 *  distributed over [0..MAXLONG-1].
 */
ULONG
NTAPI
RtlUniform(IN PULONG Seed)
{
    ULONG Result;

    /* Generate the random number */
    Result = (*Seed * LCG_A + LCG_C) % LCG_M;

    /* Return it */
    *Seed = Result;
    return Result;
}
