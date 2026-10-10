/*
 * cabinet.h
 *
 * Copyright 2002 Greg Turner
 * Copyright 2005 Gerold Jens Wucherpfennig
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
#ifndef __WINE_CABINET_H
#define __WINE_CABINET_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "fdi.h"
#include "fci.h"

#define CAB_SPLITMAX (10)

#define CAB_SEARCH_SIZE (32*1024)

typedef unsigned char cab_UBYTE; /* 8 bits  */
typedef UINT16        cab_UWORD; /* 16 bits */
typedef UINT32        cab_ULONG; /* 32 bits */
typedef INT32         cab_LONG;  /* 32 bits */

typedef UINT32        cab_off_t;

/* number of bits in a ULONG */
#ifndef CHAR_BIT
# define CHAR_BIT (8)
#endif
#define CAB_ULONG_BITS (sizeof(cab_ULONG) * CHAR_BIT)

/* structure offsets */
#define cfhead_Signature         (0x00)
#define cfhead_CabinetSize       (0x08)
#define cfhead_FileOffset        (0x10)
#define cfhead_MinorVersion      (0x18)
#define cfhead_MajorVersion      (0x19)
#define cfhead_NumFolders        (0x1A)
#define cfhead_NumFiles          (0x1C)
#define cfhead_Flags             (0x1E)
#define cfhead_SetID             (0x20)
#define cfhead_CabinetIndex      (0x22)
#define cfhead_SIZEOF            (0x24)
#define cfheadext_HeaderReserved (0x00)
#define cfheadext_FolderReserved (0x02)
#define cfheadext_DataReserved   (0x03)
#define cfheadext_SIZEOF         (0x04)
#define cffold_DataOffset        (0x00)
#define cffold_NumBlocks         (0x04)
#define cffold_CompType          (0x06)
#define cffold_SIZEOF            (0x08)
#define cffile_UncompressedSize  (0x00)
#define cffile_FolderOffset      (0x04)
#define cffile_FolderIndex       (0x08)
#define cffile_Date              (0x0A)
#define cffile_Time              (0x0C)
#define cffile_Attribs           (0x0E)
#define cffile_SIZEOF            (0x10)
#define cfdata_CheckSum          (0x00)
#define cfdata_CompressedSize    (0x04)
#define cfdata_UncompressedSize  (0x06)
#define cfdata_SIZEOF            (0x08)

/* flags */
#define cffoldCOMPTYPE_MASK            (0x000f)
#define cffoldCOMPTYPE_NONE            (0x0000)
#define cffoldCOMPTYPE_MSZIP           (0x0001)
#define cffoldCOMPTYPE_QUANTUM         (0x0002)
#define cffoldCOMPTYPE_LZX             (0x0003)
#define cfheadPREV_CABINET             (0x0001)
#define cfheadNEXT_CABINET             (0x0002)
#define cfheadRESERVE_PRESENT          (0x0004)
#define cffileCONTINUED_FROM_PREV      (0xFFFD)
#define cffileCONTINUED_TO_NEXT        (0xFFFE)
#define cffileCONTINUED_PREV_AND_NEXT  (0xFFFF)
#define cffile_A_RDONLY                (0x01)
#define cffile_A_HIDDEN                (0x02)
#define cffile_A_SYSTEM                (0x04)
#define cffile_A_ARCH                  (0x20)
#define cffile_A_EXEC                  (0x40)
#define cffile_A_NAME_IS_UTF           (0x80)

/****************************************************************************/
/* our archiver information / state */

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
  
/* Quantum stuff */

struct QTMmodelsym {
  cab_UWORD sym, cumfreq;
};

struct QTMmodel {
  int shiftsleft, entries; 
  struct QTMmodelsym *syms;
  cab_UWORD tabloc[256];
};

struct QTMstate {
    cab_UBYTE *window;         /* the actual decoding window              */
    cab_ULONG window_size;     /* window size (1Kb through 2Mb)           */
    cab_ULONG actual_size;     /* window size when it was first allocated */
    cab_ULONG window_posn;     /* current offset within the window        */

    struct QTMmodel model7;
    struct QTMmodelsym m7sym[7+1];

    struct QTMmodel model4, model5, model6pos, model6len;
    struct QTMmodelsym m4sym[0x18 + 1];
    struct QTMmodelsym m5sym[0x24 + 1];
    struct QTMmodelsym m6psym[0x2a + 1], m6lsym[0x1b + 1];

    struct QTMmodel model00, model40, model80, modelC0;
    struct QTMmodelsym m00sym[0x40 + 1], m40sym[0x40 + 1];
    struct QTMmodelsym m80sym[0x40 + 1], mC0sym[0x40 + 1];
};

/* LZX stuff */

/* some constants defined by the LZX specification */
#define LZX_MIN_MATCH                (2)
#define LZX_MAX_MATCH                (257)
#define LZX_NUM_CHARS                (256)
#define LZX_BLOCKTYPE_INVALID        (0)   /* also blocktypes 4-7 invalid */
#define LZX_BLOCKTYPE_VERBATIM       (1)
#define LZX_BLOCKTYPE_ALIGNED        (2)
#define LZX_BLOCKTYPE_UNCOMPRESSED   (3)
#define LZX_PRETREE_NUM_ELEMENTS     (20)
#define LZX_ALIGNED_NUM_ELEMENTS     (8)   /* aligned offset tree #elements */
#define LZX_NUM_PRIMARY_LENGTHS      (7)   /* this one missing from spec! */
#define LZX_NUM_SECONDARY_LENGTHS    (249) /* length tree #elements */

/* LZX huffman defines: tweak tablebits as desired */
#define LZX_PRETREE_MAXSYMBOLS  (LZX_PRETREE_NUM_ELEMENTS)
#define LZX_PRETREE_TABLEBITS   (6)
#define LZX_MAINTREE_MAXSYMBOLS (LZX_NUM_CHARS + 50*8)
#define LZX_MAINTREE_TABLEBITS  (12)
#define LZX_LENGTH_MAXSYMBOLS   (LZX_NUM_SECONDARY_LENGTHS+1)
#define LZX_LENGTH_TABLEBITS    (12)
#define LZX_ALIGNED_MAXSYMBOLS  (LZX_ALIGNED_NUM_ELEMENTS)
#define LZX_ALIGNED_TABLEBITS   (7)

#define LZX_LENTABLE_SAFETY (64) /* we allow length table decoding overruns */

#define LZX_DECLARE_TABLE(tbl) \
  cab_UWORD tbl##_table[(1<<LZX_##tbl##_TABLEBITS) + (LZX_##tbl##_MAXSYMBOLS<<1)];\
  cab_UBYTE tbl##_len  [LZX_##tbl##_MAXSYMBOLS + LZX_LENTABLE_SAFETY]

struct LZXstate {
    cab_UBYTE *window;         /* the actual decoding window              */
    cab_ULONG window_size;     /* window size (32Kb through 2Mb)          */
    cab_ULONG actual_size;     /* window size when it was first allocated */
    cab_ULONG window_posn;     /* current offset within the window        */
    cab_ULONG R0, R1, R2;      /* for the LRU offset system               */
    cab_UWORD main_elements;   /* number of main tree elements            */
    int   header_read;         /* have we started decoding at all yet?    */
    cab_UWORD block_type;      /* type of this block                      */
    cab_ULONG block_length;    /* uncompressed length of this block       */
    cab_ULONG block_remaining; /* uncompressed bytes still left to decode */
    cab_ULONG frames_read;     /* the number of CFDATA blocks processed   */
    cab_LONG  intel_filesize;  /* magic header value used for transform   */
    cab_LONG  intel_curpos;    /* current offset in transform space       */
    int   intel_started;       /* have we seen any translatable data yet? */

    LZX_DECLARE_TABLE(PRETREE);
    LZX_DECLARE_TABLE(MAINTREE);
    LZX_DECLARE_TABLE(LENGTH);
    LZX_DECLARE_TABLE(ALIGNED);
};

struct lzx_bits {
  cab_ULONG bb;
  int bl;
  cab_UBYTE *ip;
};

/* CAB data blocks are <= 32768 bytes in uncompressed form. Uncompressed
 * blocks have zero growth. MSZIP guarantees that it won't grow above
 * uncompressed size by more than 12 bytes. LZX guarantees it won't grow
 * more than 6144 bytes.
 */
#define CAB_BLOCKMAX (32768)
#define CAB_INPUTMAX (CAB_BLOCKMAX+6144)

struct cab_file {
  struct cab_file *next;               /* next file in sequence          */
  struct cab_folder *folder;           /* folder that contains this file */
  LPCSTR filename;                     /* output name of file            */
  HANDLE fh;                           /* open file handle or NULL       */
  cab_ULONG length;                    /* uncompressed length of file    */
  cab_ULONG offset;                    /* uncompressed offset in folder  */
  cab_UWORD index;                     /* magic index number of folder   */
  cab_UWORD time, date, attribs;       /* MS-DOS time/date/attributes    */
};


struct cab_folder {
  struct cab_folder *next;
  struct cabinet *cab[CAB_SPLITMAX];   /* cabinet(s) this folder spans   */
  cab_off_t offset[CAB_SPLITMAX];      /* offset to data blocks          */
  cab_UWORD comp_type;                 /* compression format/window size */
  cab_ULONG comp_size;                 /* compressed size of folder      */
  cab_UBYTE num_splits;                /* number of split blocks + 1     */
  cab_UWORD num_blocks;                /* total number of blocks         */
  struct cab_file *contfile;           /* the first split file           */
};

struct cabinet {
  struct cabinet *next;                /* for making a list of cabinets  */
  LPCSTR filename;                     /* input name of cabinet          */
  HANDLE *fh;                          /* open file handle or NULL       */
  cab_off_t filelen;                   /* length of cabinet file         */
  cab_off_t blocks_off;                /* offset to data blocks in file  */
  struct cabinet *prevcab, *nextcab;   /* multipart cabinet chains       */
  char *prevname, *nextname;           /* and their filenames            */
  char *previnfo, *nextinfo;           /* and their visible names        */
  struct cab_folder *folders;          /* first folder in this cabinet   */
  struct cab_file *files;              /* first file in this cabinet     */
  cab_UBYTE block_resv;                /* reserved space in datablocks   */
  cab_UBYTE flags;                     /* header flags                   */
};

typedef struct cds_forward {
  struct cab_folder *current;      /* current folder we're extracting from  */
  cab_ULONG offset;                /* uncompressed offset within folder     */
  cab_UBYTE *outpos;               /* (high level) start of data to use up  */
  cab_UWORD outlen;                /* (high level) amount of data to use up */
  cab_UWORD split;                 /* at which split in current folder?     */
  int (*decompress)(int, int, struct cds_forward *); /* chosen compress fn  */
  cab_UBYTE inbuf[CAB_INPUTMAX+2]; /* +2 for lzx bitbuffer overflows!       */
  cab_UBYTE outbuf[CAB_BLOCKMAX];
  cab_UBYTE q_length_base[27], q_length_extra[27], q_extra_bits[42];
  cab_ULONG q_position_base[42];
  cab_ULONG lzx_position_base[51];
  cab_UBYTE extra_bits[51];
  union {
    struct ZIPstate zip;
    struct QTMstate qtm;
    struct LZXstate lzx;
  } methods;
} cab_decomp_state;

/*
 * the rest of these are somewhat kludgy macros which are shared between fdi.c
 * and cabextract.c.
 */

/* Bitstream reading macros (Quantum / normal byte order)
 *
 * Q_INIT_BITSTREAM    should be used first to set up the system
 * Q_READ_BITS(var,n)  takes N bits from the buffer and puts them in var.
 *                     unlike LZX, this can loop several times to get the
 *                     requisite number of bits.
 * Q_FILL_BUFFER       adds more data to the bit buffer, if there is room
 *                     for another 16 bits.
 * Q_PEEK_BITS(n)      extracts (without removing) N bits from the bit
 *                     buffer
 * Q_REMOVE_BITS(n)    removes N bits from the bit buffer
 *
 * These bit access routines work by using the area beyond the MSB and the
 * LSB as a free source of zeroes. This avoids having to mask any bits.
 * So we have to know the bit width of the bitbuffer variable. This is
 * defined as ULONG_BITS.
 *
 * ULONG_BITS should be at least 16 bits. Unlike LZX's Huffman decoding,
 * Quantum's arithmetic decoding only needs 1 bit at a time, it doesn't
 * need an assured number. Retrieving larger bitstrings can be done with
 * multiple reads and fills of the bitbuffer. The code should work fine
 * for machines where ULONG >= 32 bits.
 *
 * Also note that Quantum reads bytes in normal order; LZX is in
 * little-endian order.
 */

#define Q_INIT_BITSTREAM do { bitsleft = 0; bitbuf = 0; } while (0)

#define Q_FILL_BUFFER do {                                                  \
  if (bitsleft <= (CAB_ULONG_BITS - 16)) {                                  \
    bitbuf |= ((inpos[0]<<8)|inpos[1]) << (CAB_ULONG_BITS-16 - bitsleft);   \
    bitsleft += 16; inpos += 2;                                             \
  }                                                                         \
} while (0)

#define Q_PEEK_BITS(n)   (bitbuf >> (CAB_ULONG_BITS - (n)))
#define Q_REMOVE_BITS(n) ((bitbuf <<= (n)), (bitsleft -= (n)))

#define Q_READ_BITS(v,n) do {                                           \
  (v) = 0;                                                              \
  for (bitsneed = (n); bitsneed; bitsneed -= bitrun) {                  \
    Q_FILL_BUFFER;                                                      \
    bitrun = (bitsneed > bitsleft) ? bitsleft : bitsneed;               \
    (v) = ((v) << bitrun) | Q_PEEK_BITS(bitrun);                        \
    Q_REMOVE_BITS(bitrun);                                              \
  }                                                                     \
} while (0)

#define Q_MENTRIES(model) (QTM(model).entries)
#define Q_MSYM(model,symidx) (QTM(model).syms[(symidx)].sym)
#define Q_MSYMFREQ(model,symidx) (QTM(model).syms[(symidx)].cumfreq)

/* GET_SYMBOL(model, var) fetches the next symbol from the stated model
 * and puts it in var. it may need to read the bitstream to do this.
 */
#define GET_SYMBOL(m, var) do {                                         \
  range =  ((H - L) & 0xFFFF) + 1;                                      \
  symf = ((((C - L + 1) * Q_MSYMFREQ(m,0)) - 1) / range) & 0xFFFF;      \
                                                                        \
  for (i=1; i < Q_MENTRIES(m); i++) {                                   \
    if (Q_MSYMFREQ(m,i) <= symf) break;                                 \
  }                                                                     \
  (var) = Q_MSYM(m,i-1);                                                \
                                                                        \
  range = (H - L) + 1;                                                  \
  H = L + ((Q_MSYMFREQ(m,i-1) * range) / Q_MSYMFREQ(m,0)) - 1;          \
  L = L + ((Q_MSYMFREQ(m,i)   * range) / Q_MSYMFREQ(m,0));              \
  while (1) {                                                           \
    if ((L & 0x8000) != (H & 0x8000)) {                                 \
      if ((L & 0x4000) && !(H & 0x4000)) {                              \
        /* underflow case */                                            \
        C ^= 0x4000; L &= 0x3FFF; H |= 0x4000;                          \
      }                                                                 \
      else break;                                                       \
    }                                                                   \
    L <<= 1; H = (H << 1) | 1;                                          \
    Q_FILL_BUFFER;                                                      \
    C  = (C << 1) | Q_PEEK_BITS(1);                                     \
    Q_REMOVE_BITS(1);                                                   \
  }                                                                     \
                                                                        \
  QTMupdatemodel(&(QTM(m)), i);                                         \
} while (0)

/* Bitstream reading macros (LZX / intel little-endian byte order)
 *
 * INIT_BITSTREAM    should be used first to set up the system
 * READ_BITS(var,n)  takes N bits from the buffer and puts them in var
 *
 * ENSURE_BITS(n)    ensures there are at least N bits in the bit buffer.
 *                   it can guarantee up to 17 bits (i.e. it can read in
 *                   16 new bits when there is down to 1 bit in the buffer,
 *                   and it can read 32 bits when there are 0 bits in the
 *                   buffer).
 * PEEK_BITS(n)      extracts (without removing) N bits from the bit buffer
 * REMOVE_BITS(n)    removes N bits from the bit buffer
 *
 * These bit access routines work by using the area beyond the MSB and the
 * LSB as a free source of zeroes. This avoids having to mask any bits.
 * So we have to know the bit width of the bitbuffer variable.
 */

#define INIT_BITSTREAM do { bitsleft = 0; bitbuf = 0; } while (0)

/* Quantum reads bytes in normal order; LZX is little-endian order */
#define ENSURE_BITS(n)                                                    \
  while (bitsleft < (n)) {                                                \
    bitbuf |= ((inpos[1]<<8)|inpos[0]) << (CAB_ULONG_BITS-16 - bitsleft); \
    bitsleft += 16; inpos+=2;                                             \
  }

#define PEEK_BITS(n)   (bitbuf >> (CAB_ULONG_BITS - (n)))
#define REMOVE_BITS(n) ((bitbuf <<= (n)), (bitsleft -= (n)))

#define READ_BITS(v,n) do {                                             \
  if (n) {                                                              \
    ENSURE_BITS(n);                                                     \
    (v) = PEEK_BITS(n);                                                 \
    REMOVE_BITS(n);                                                     \
  }                                                                     \
  else {                                                                \
    (v) = 0;                                                            \
  }                                                                     \
} while (0)

/* Huffman macros */

#define TABLEBITS(tbl)   (LZX_##tbl##_TABLEBITS)
#define MAXSYMBOLS(tbl)  (LZX_##tbl##_MAXSYMBOLS)
#define SYMTABLE(tbl)    (LZX(tbl##_table))
#define LENTABLE(tbl)    (LZX(tbl##_len))

/* BUILD_TABLE(tablename) builds a huffman lookup table from code lengths.
 * In reality, it just calls make_decode_table() with the appropriate
 * values - they're all fixed by some #defines anyway, so there's no point
 * writing each call out in full by hand.
 */
#define BUILD_TABLE(tbl)                                                \
  if (make_decode_table(                                                \
    MAXSYMBOLS(tbl), TABLEBITS(tbl), LENTABLE(tbl), SYMTABLE(tbl)       \
  )) { return DECR_ILLEGALDATA; }

/* READ_HUFFSYM(tablename, var) decodes one huffman symbol from the
 * bitstream using the stated table and puts it in var.
 */
#define READ_HUFFSYM(tbl,var) do {                                      \
  ENSURE_BITS(16);                                                      \
  hufftbl = SYMTABLE(tbl);                                              \
  if ((i = hufftbl[PEEK_BITS(TABLEBITS(tbl))]) >= MAXSYMBOLS(tbl)) {    \
    j = 1 << (CAB_ULONG_BITS - TABLEBITS(tbl));                         \
    do {                                                                \
      j >>= 1; i <<= 1; i |= (bitbuf & j) ? 1 : 0;                      \
      if (!j) { return DECR_ILLEGALDATA; }                              \
    } while ((i = hufftbl[i]) >= MAXSYMBOLS(tbl));                      \
  }                                                                     \
  j = LENTABLE(tbl)[(var) = i];                                         \
  REMOVE_BITS(j);                                                       \
} while (0)

/* READ_LENGTHS(tablename, first, last) reads in code lengths for symbols
 * first to last in the given table. The code lengths are stored in their
 * own special LZX way.
 */
#define READ_LENGTHS(tbl,first,last,fn) do { \
  lb.bb = bitbuf; lb.bl = bitsleft; lb.ip = inpos; \
  if (fn(LENTABLE(tbl),(first),(last),&lb,decomp_state)) { \
    return DECR_ILLEGALDATA; \
  } \
  bitbuf = lb.bb; bitsleft = lb.bl; inpos = lb.ip; \
} while (0)

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

/* SESSION Operation */
#define EXTRACT_FILLFILELIST  0x00000001
#define EXTRACT_EXTRACTFILES  0x00000002

struct FILELIST{
    LPSTR FileName;
    struct FILELIST *next;
    BOOL DoExtract;
};

typedef struct {
    INT FileSize;
    ERF Error;
    struct FILELIST *FileList;
    INT FileCount;
    INT Operation;
    CHAR Destination[MAX_PATH];
    CHAR CurrentFile[MAX_PATH];
    CHAR Reserved[MAX_PATH];
    struct FILELIST *FilterList;
} SESSION;

#endif /* __WINE_CABINET_H */
