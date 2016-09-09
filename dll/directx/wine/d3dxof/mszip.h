/*
 * MSZIP decompression header (taken from cabinet.h of cabinet dll)
 *
 * Copyright 2002 Greg Turner
 * Copyright 2005 Gerold Jens Wucherpfennig
 * Copyright 2010 Christian Costa
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

#include "fdi.h"

typedef unsigned char cab_UBYTE; /* 8 bits  */
typedef UINT16        cab_UWORD; /* 16 bits */
typedef UINT32        cab_ULONG; /* 32 bits */
typedef INT32         cab_LONG;  /* 32 bits */

typedef struct {
  unsigned int FDI_Intmagic;
  PFNALLOC pfnalloc;
  PFNFREE  pfnfree;
  PFNOPEN  pfnopen;
  PFNREAD  pfnread;
  PFNWRITE pfnwrite;
  PFNCLOSE pfnclose;
  PFNSEEK  pfnseek;
  PERF     perf;
} FDI_Int, *PFDI_Int;

/* cast an HFDI into a PFDI_Int */
#define PFDI_INT(hfdi) ((PFDI_Int)(hfdi))

#define PFDI_ALLOC(hfdi, size)            ((*PFDI_INT(hfdi)->pfnalloc) (size))
#define PFDI_FREE(hfdi, ptr)              ((*PFDI_INT(hfdi)->pfnfree)  (ptr))

/* MSZIP stuff */
#define ZIPWSIZE 	0x8000  /* window size */
#define ZIPLBITS	9	/* bits in base literal/length lookup table */
#define ZIPDBITS	6	/* bits in base distance lookup table */
#define ZIPBMAX		16      /* maximum bit length of any code */
#define ZIPN_MAX	288     /* maximum number of codes in any set */

struct Ziphuft {
  cab_UBYTE e;                /* number of extra bits or operation */
  cab_UBYTE b;                /* number of bits in this code or subcode */
  union {
    cab_UWORD n;              /* literal, length base, or distance base */
    struct Ziphuft *t;        /* pointer to next level of table */
  } v;
};

struct ZIPstate {
    cab_ULONG window_posn;      /* current offset within the window        */
    cab_ULONG bb;               /* bit buffer */
    cab_ULONG bk;               /* bits in bit buffer */
    cab_ULONG ll[288+32];       /* literal/length and distance code lengths */
    cab_ULONG c[ZIPBMAX+1];     /* bit length count table */
    cab_LONG  lx[ZIPBMAX+1];    /* memory for l[-1..ZIPBMAX-1] */
    struct Ziphuft *u[ZIPBMAX];	/* table stack */
    cab_ULONG v[ZIPN_MAX];      /* values in order of bit length */
    cab_ULONG x[ZIPBMAX+1];     /* bit offsets, then code stack */
    cab_UBYTE *inpos;
};

#define CAB(x) (decomp_state->x)
#define ZIP(x) (decomp_state->methods.zip.x)
#define DECR_OK           (0)
#define DECR_DATAFORMAT   (1)
#define DECR_ILLEGALDATA  (2)
#define DECR_NOMEMORY     (3)
#define DECR_CHECKSUM     (4)
#define DECR_INPUT        (5)
#define DECR_OUTPUT       (6)
#define DECR_USERABORT    (7)

#define ZIPNEEDBITS(n) {while(k<(n)){cab_LONG c=*(ZIP(inpos)++);\
    b|=((cab_ULONG)c)<<k;k+=8;}}
#define ZIPDUMPBITS(n) {b>>=(n);k-=(n);}

/* CAB data blocks are <= 32768 bytes in uncompressed form. Uncompressed
 * blocks have zero growth. MSZIP guarantees that it won't grow above
 * uncompressed size by more than 12 bytes. LZX guarantees it won't grow
 * more than 6144 bytes.
 */
#define CAB_BLOCKMAX (32768)
#define CAB_INPUTMAX (CAB_BLOCKMAX+6144)

typedef struct fdi_cds_fwd {
  void *hfdi;                      /* the hfdi we are using                 */
  cab_UBYTE inbuf[CAB_INPUTMAX+2]; /* +2 for lzx bitbuffer overflows!       */
  cab_UBYTE outbuf[CAB_BLOCKMAX];
  union {
    struct ZIPstate zip;
  } methods;
} fdi_decomp_state;

/* Tables for deflate from PKZIP's appnote.txt. */

#define THOSE_ZIP_CONSTS                                                           \
static const cab_UBYTE Zipborder[] = /* Order of the bit length code lengths */    \
{ 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};               \
static const cab_UWORD Zipcplens[] = /* Copy lengths for literal codes 257..285 */ \
{ 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51,             \
 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};                              \
static const cab_UWORD Zipcplext[] = /* Extra bits for literal codes 257..285 */   \
{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,             \
  4, 5, 5, 5, 5, 0, 99, 99}; /* 99==invalid */                                     \
static const cab_UWORD Zipcpdist[] = /* Copy offsets for distance codes 0..29 */   \
{ 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,             \
513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};          \
static const cab_UWORD Zipcpdext[] = /* Extra bits for distance codes */           \
{ 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,            \
10, 11, 11, 12, 12, 13, 13};                                                       \
/* And'ing with Zipmask[n] masks the lower n bits */                               \
static const cab_UWORD Zipmask[17] = {                                             \
 0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,           \
 0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff                    \
}
