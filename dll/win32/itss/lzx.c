/***************************************************************************
 *                        lzx.c - LZX decompression routines               *
 *                           -------------------                           *
 *                                                                         *
 *  maintainer: Jed Wing <jedwin@ugcs.caltech.edu>                         *
 *  source:     modified lzx.c from cabextract v0.5                        *
 *  notes:      This file was taken from cabextract v0.5, which was,       *
 *              itself, a modified version of the lzx decompression code   *
 *              from unlzx.                                                *
 *                                                                         *
 *  platforms:  In its current incarnation, this file has been tested on   *
 *              two different Linux platforms (one, redhat-based, with a   *
 *              2.1.2 glibc and gcc 2.95.x, and the other, Debian, with    *
 *              2.2.4 glibc and both gcc 2.95.4 and gcc 3.0.2).  Both were *
 *              Intel x86 compatible machines.                             *
 ***************************************************************************/

/***************************************************************************
 *
 *   Copyright(C) Stuart Caie
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
 *
 ***************************************************************************/

#include "lzx.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"

/* sized types */
typedef unsigned char  UBYTE; /* 8 bits exactly    */
typedef unsigned short UWORD; /* 16 bits (or more) */

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
  UWORD tbl##_table[(1<<LZX_##tbl##_TABLEBITS) + (LZX_##tbl##_MAXSYMBOLS<<1)];\
  UBYTE tbl##_len  [LZX_##tbl##_MAXSYMBOLS + LZX_LENTABLE_SAFETY]

struct LZXstate
{
    UBYTE *window;         /* the actual decoding window              */
    ULONG window_size;     /* window size (32Kb through 2Mb)          */
    ULONG actual_size;     /* window size when it was first allocated */
    ULONG window_posn;     /* current offset within the window        */
    ULONG R0, R1, R2;      /* for the LRU offset system               */
    UWORD main_elements;   /* number of main tree elements            */
    int   header_read;     /* have we started decoding at all yet?    */
    UWORD block_type;      /* type of this block                      */
    ULONG block_length;    /* uncompressed length of this block       */
    ULONG block_remaining; /* uncompressed bytes still left to decode */
    ULONG frames_read;     /* the number of CFDATA blocks processed   */
    LONG  intel_filesize;  /* magic header value used for transform   */
    LONG  intel_curpos;    /* current offset in transform space       */
    int   intel_started;   /* have we seen any translatable data yet? */

    LZX_DECLARE_TABLE(PRETREE);
    LZX_DECLARE_TABLE(MAINTREE);
    LZX_DECLARE_TABLE(LENGTH);
    LZX_DECLARE_TABLE(ALIGNED);
};

/* LZX decruncher */

/* Microsoft's LZX document and their implementation of the
 * com.ms.util.cab Java package do not concur.
 *
 * In the LZX document, there is a table showing the correlation between
 * window size and the number of position slots. It states that the 1MB
 * window = 40 slots and the 2MB window = 42 slots. In the implementation,
 * 1MB = 42 slots, 2MB = 50 slots. The actual calculation is 'find the
 * first slot whose position base is equal to or more than the required
 * window size'. This would explain why other tables in the document refer
 * to 50 slots rather than 42.
 *
 * The constant NUM_PRIMARY_LENGTHS used in the decompression pseudocode
 * is not defined in the specification.
 *
 * The LZX document does not state the uncompressed block has an
 * uncompressed length field. Where does this length field come from, so
 * we can know how large the block is? The implementation has it as the 24
 * bits following after the 3 blocktype bits, before the alignment
 * padding.
 *
 * The LZX document states that aligned offset blocks have their aligned
 * offset huffman tree AFTER the main and length trees. The implementation
 * suggests that the aligned offset tree is BEFORE the main and length
 * trees.
 *
 * The LZX document decoding algorithm states that, in an aligned offset
 * block, if an extra_bits value is 1, 2 or 3, then that number of bits
 * should be read and the result added to the match offset. This is
 * correct for 1 and 2, but not 3, where just a huffman symbol (using the
 * aligned tree) should be read.
 *
 * Regarding the E8 preprocessing, the LZX document states 'No translation
 * may be performed on the last 6 bytes of the input block'. This is
 * correct.  However, the pseudocode provided checks for the *E8 leader*
 * up to the last 6 bytes. If the leader appears between -10 and -7 bytes
 * from the end, this would cause the next four bytes to be modified, at
 * least one of which would be in the last 6 bytes, which is not allowed
 * according to the spec.
 *
 * The specification states that the huffman trees must always contain at
 * least one element. However, many CAB files contain blocks where the
 * length tree is completely empty (because there are no matches), and
 * this is expected to succeed.
 */


/* LZX uses what it calls 'position slots' to represent match offsets.
 * What this means is that a small 'position slot' number and a small
 * offset from that slot are encoded instead of one large offset for
 * every match.
 * - position_base is an index to the position slot bases
 * - extra_bits states how many bits of offset-from-base data is needed.
 */
static const UBYTE extra_bits[51] = {
     0,  0,  0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,
     7,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14,
    15, 15, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17
};

static const ULONG position_base[51] = {
          0,       1,       2,      3,      4,      6,      8,     12,     16,     24,     32,       48,      64,      96,     128,     192,
        256,     384,     512,    768,   1024,   1536,   2048,   3072,   4096,   6144,   8192,    12288,   16384,   24576,   32768,   49152,
      65536,   98304,  131072, 196608, 262144, 393216, 524288, 655360, 786432, 917504, 1048576, 1179648, 1310720, 1441792, 1572864, 1703936,
    1835008, 1966080, 2097152
};

struct LZXstate *LZXinit(int wndsize)
{
    struct LZXstate *pState=NULL;
    int i, posn_slots;

    /* allocate state and associated window */
    pState = HeapAlloc(GetProcessHeap(), 0, sizeof(struct LZXstate));
    if (!(pState->window = HeapAlloc(GetProcessHeap(), 0, wndsize)))
    {
        HeapFree(GetProcessHeap(), 0, pState);
        return NULL;
    }
    pState->actual_size = wndsize;
    pState->window_size = wndsize;

    /* calculate required position slots */
    posn_slots = i = 0;
    while (i < wndsize) i += 1 << extra_bits[posn_slots++];

    /* initialize other state */
    pState->R0  =  pState->R1  = pState->R2 = 1;
    pState->main_elements   = LZX_NUM_CHARS + (posn_slots << 3);
    pState->header_read     = 0;
    pState->frames_read     = 0;
    pState->block_remaining = 0;
    pState->block_type      = LZX_BLOCKTYPE_INVALID;
    pState->intel_curpos    = 0;
    pState->intel_started   = 0;
    pState->window_posn     = 0;

    /* initialise tables to 0 (because deltas will be applied to them) */
    for (i = 0; i < LZX_MAINTREE_MAXSYMBOLS; i++) pState->MAINTREE_len[i] = 0;
    for (i = 0; i < LZX_LENGTH_MAXSYMBOLS; i++)   pState->LENGTH_len[i]   = 0;

    return pState;
}

void LZXteardown(struct LZXstate *pState)
{
    if (pState)
    {
        HeapFree(GetProcessHeap(), 0, pState->window);
        HeapFree(GetProcessHeap(), 0, pState);
    }
}

int LZXreset(struct LZXstate *pState)
{
    int i;

    pState->R0  =  pState->R1  = pState->R2 = 1;
    pState->header_read     = 0;
    pState->frames_read     = 0;
    pState->block_remaining = 0;
    pState->block_type      = LZX_BLOCKTYPE_INVALID;
    pState->intel_curpos    = 0;
    pState->intel_started   = 0;
    pState->window_posn     = 0;

    for (i = 0; i < LZX_MAINTREE_MAXSYMBOLS + LZX_LENTABLE_SAFETY; i++) pState->MAINTREE_len[i] = 0;
    for (i = 0; i < LZX_LENGTH_MAXSYMBOLS + LZX_LENTABLE_SAFETY; i++)   pState->LENGTH_len[i]   = 0;

    return DECR_OK;
}


/* Bitstream reading macros:
 *
 * INIT_BITSTREAM    should be used first to set up the system
 * READ_BITS(var,n)  takes N bits from the buffer and puts them in var
 *
 * ENSURE_BITS(n)    ensures there are at least N bits in the bit buffer
 * PEEK_BITS(n)      extracts (without removing) N bits from the bit buffer
 * REMOVE_BITS(n)    removes N bits from the bit buffer
 *
 * These bit access routines work by using the area beyond the MSB and the
 * LSB as a free source of zeroes. This avoids having to mask any bits.
 * So we have to know the bit width of the bitbuffer variable. This is
 * sizeof(ULONG) * 8, also defined as ULONG_BITS
 */

/* number of bits in ULONG. Note: This must be at multiple of 16, and at
 * least 32 for the bitbuffer code to work (ie, it must be able to ensure
 * up to 17 bits - that's adding 16 bits when there's one bit left, or
 * adding 32 bits when there are no bits left. The code should work fine
 * for machines where ULONG >= 32 bits.
 */
#define ULONG_BITS (sizeof(ULONG)<<3)

#define INIT_BITSTREAM do { bitsleft = 0; bitbuf = 0; } while (0)

#define ENSURE_BITS(n)							\
  while (bitsleft < (n)) {						\
    bitbuf |= ((inpos[1]<<8)|inpos[0]) << (ULONG_BITS-16 - bitsleft);	\
    bitsleft += 16; inpos+=2;						\
  }

#define PEEK_BITS(n)   (bitbuf >> (ULONG_BITS - (n)))
#define REMOVE_BITS(n) ((bitbuf <<= (n)), (bitsleft -= (n)))

#define READ_BITS(v,n) do {						\
  ENSURE_BITS(n);							\
  (v) = PEEK_BITS(n);							\
  REMOVE_BITS(n);							\
} while (0)


/* Huffman macros */

#define TABLEBITS(tbl)   (LZX_##tbl##_TABLEBITS)
#define MAXSYMBOLS(tbl)  (LZX_##tbl##_MAXSYMBOLS)
#define SYMTABLE(tbl)    (pState->tbl##_table)
#define LENTABLE(tbl)    (pState->tbl##_len)

/* BUILD_TABLE(tablename) builds a huffman lookup table from code lengths.
 * In reality, it just calls make_decode_table() with the appropriate
 * values - they're all fixed by some #defines anyway, so there's no point
 * writing each call out in full by hand.
 */
#define BUILD_TABLE(tbl)						\
  if (make_decode_table(						\
    MAXSYMBOLS(tbl), TABLEBITS(tbl), LENTABLE(tbl), SYMTABLE(tbl)	\
  )) { return DECR_ILLEGALDATA; }


/* READ_HUFFSYM(tablename, var) decodes one huffman symbol from the
 * bitstream using the stated table and puts it in var.
 */
#define READ_HUFFSYM(tbl,var) do {					\
  ENSURE_BITS(16);							\
  hufftbl = SYMTABLE(tbl);						\
  if ((i = hufftbl[PEEK_BITS(TABLEBITS(tbl))]) >= MAXSYMBOLS(tbl)) {	\
    j = 1 << (ULONG_BITS - TABLEBITS(tbl));				\
    do {								\
      j >>= 1; i <<= 1; i |= (bitbuf & j) ? 1 : 0;			\
      if (!j) { return DECR_ILLEGALDATA; }	                        \
    } while ((i = hufftbl[i]) >= MAXSYMBOLS(tbl));			\
  }									\
  j = LENTABLE(tbl)[(var) = i];						\
  REMOVE_BITS(j);							\
} while (0)


/* READ_LENGTHS(tablename, first, last) reads in code lengths for symbols
 * first to last in the given table. The code lengths are stored in their
 * own special LZX way.
 */
#define READ_LENGTHS(tbl,first,last) do { \
  lb.bb = bitbuf; lb.bl = bitsleft; lb.ip = inpos; \
  if (lzx_read_lens(pState, LENTABLE(tbl),(first),(last),&lb)) { \
    return DECR_ILLEGALDATA; \
  } \
  bitbuf = lb.bb; bitsleft = lb.bl; inpos = lb.ip; \
} while (0)


/* make_decode_table(nsyms, nbits, length[], table[])
 *
 * This function was coded by David Tritscher. It builds a fast huffman
 * decoding table out of just a canonical huffman code lengths table.
 *
 * nsyms  = total number of symbols in this huffman tree.
 * nbits  = any symbols with a code length of nbits or less can be decoded
 *          in one lookup of the table.
 * length = A table to get code lengths from [0 to syms-1]
 * table  = The table to fill up with decoded symbols and pointers.
 *
 * Returns 0 for OK or 1 for error
 */

static int make_decode_table(ULONG nsyms, ULONG nbits, UBYTE *length, UWORD *table) {
    register UWORD sym;
    register ULONG leaf;
    register UBYTE bit_num = 1;
    ULONG fill;
    ULONG pos         = 0; /* the current position in the decode table */
    ULONG table_mask  = 1 << nbits;
    ULONG bit_mask    = table_mask >> 1; /* don't do 0 length codes */
    ULONG next_symbol = bit_mask; /* base of allocation for long codes */

    /* fill entries for codes short enough for a direct mapping */
    while (bit_num <= nbits) {
        for (sym = 0; sym < nsyms; sym++) {
            if (length[sym] == bit_num) {
                leaf = pos;

                if((pos += bit_mask) > table_mask) return 1; /* table overrun */

                /* fill all possible lookups of this symbol with the symbol itself */
                fill = bit_mask;
                while (fill-- > 0) table[leaf++] = sym;
            }
        }
        bit_mask >>= 1;
        bit_num++;
    }

    /* if there are any codes longer than nbits */
    if (pos != table_mask) {
        /* clear the remainder of the table */
        for (sym = pos; sym < table_mask; sym++) table[sym] = 0;

        /* give ourselves room for codes to grow by up to 16 more bits */
        pos <<= 16;
        table_mask <<= 16;
        bit_mask = 1 << 15;

        while (bit_num <= 16) {
            for (sym = 0; sym < nsyms; sym++) {
                if (length[sym] == bit_num) {
                    leaf = pos >> 16;
                    for (fill = 0; fill < bit_num - nbits; fill++) {
                        /* if this path hasn't been taken yet, 'allocate' two entries */
                        if (table[leaf] == 0) {
                            table[(next_symbol << 1)] = 0;
                            table[(next_symbol << 1) + 1] = 0;
                            table[leaf] = next_symbol++;
                        }
                        /* follow the path and select either left or right for next bit */
                        leaf = table[leaf] << 1;
                        if ((pos >> (15-fill)) & 1) leaf++;
                    }
                    table[leaf] = sym;

                    if ((pos += bit_mask) > table_mask) return 1; /* table overflow */
                }
            }
            bit_mask >>= 1;
            bit_num++;
        }
    }

    /* full table? */
    if (pos == table_mask) return 0;

    /* either erroneous table, or all elements are 0 - let's find out. */
    for (sym = 0; sym < nsyms; sym++) if (length[sym]) return 1;
    return 0;
}

struct lzx_bits {
  ULONG bb;
  int bl;
  UBYTE *ip;
};

static int lzx_read_lens(struct LZXstate *pState, UBYTE *lens, ULONG first, ULONG last, struct lzx_bits *lb) {
    ULONG i,j, x,y;
    int z;

    register ULONG bitbuf = lb->bb;
    register int bitsleft = lb->bl;
    UBYTE *inpos = lb->ip;
    UWORD *hufftbl;

    for (x = 0; x < 20; x++) {
        READ_BITS(y, 4);
        LENTABLE(PRETREE)[x] = y;
    }
    BUILD_TABLE(PRETREE);

    for (x = first; x < last; ) {
        READ_HUFFSYM(PRETREE, z);
        if (z == 17) {
            READ_BITS(y, 4); y += 4;
            while (y--) lens[x++] = 0;
        }
        else if (z == 18) {
            READ_BITS(y, 5); y += 20;
            while (y--) lens[x++] = 0;
        }
        else if (z == 19) {
            READ_BITS(y, 1); y += 4;
            READ_HUFFSYM(PRETREE, z);
            z = lens[x] - z; if (z < 0) z += 17;
            while (y--) lens[x++] = z;
        }
        else {
            z = lens[x] - z; if (z < 0) z += 17;
            lens[x++] = z;
        }
    }

    lb->bb = bitbuf;
    lb->bl = bitsleft;
    lb->ip = inpos;
    return 0;
}

int LZXdecompress(struct LZXstate *pState, unsigned char *inpos, unsigned char *outpos, int inlen, int outlen) {
    UBYTE *endinp = inpos + inlen;
    UBYTE *window = pState->window;
    UBYTE *runsrc, *rundest;
    UWORD *hufftbl; /* used in READ_HUFFSYM macro as chosen decoding table */

    ULONG window_posn = pState->window_posn;
    ULONG window_size = pState->window_size;
    ULONG R0 = pState->R0;
    ULONG R1 = pState->R1;
    ULONG R2 = pState->R2;

    register ULONG bitbuf;
    register int bitsleft;
    ULONG match_offset, i,j,k; /* ijk used in READ_HUFFSYM macro */
    struct lzx_bits lb; /* used in READ_LENGTHS macro */

    int togo = outlen, this_run, main_element, aligned_bits;
    int match_length, length_footer, extra, verbatim_bits;
    int copy_length;

    INIT_BITSTREAM;

    /* read header if necessary */
    if (!pState->header_read) {
        i = j = 0;
        READ_BITS(k, 1); if (k) { READ_BITS(i,16); READ_BITS(j,16); }
        pState->intel_filesize = (i << 16) | j; /* or 0 if not encoded */
        pState->header_read = 1;
    }

    /* main decoding loop */
    while (togo > 0) {
        /* last block finished, new block expected */
        if (pState->block_remaining == 0) {
            if (pState->block_type == LZX_BLOCKTYPE_UNCOMPRESSED) {
                if (pState->block_length & 1) inpos++; /* realign bitstream to word */
                INIT_BITSTREAM;
            }

            READ_BITS(pState->block_type, 3);
            READ_BITS(i, 16);
            READ_BITS(j, 8);
            pState->block_remaining = pState->block_length = (i << 8) | j;

            switch (pState->block_type) {
                case LZX_BLOCKTYPE_ALIGNED:
                    for (i = 0; i < 8; i++) { READ_BITS(j, 3); LENTABLE(ALIGNED)[i] = j; }
                    BUILD_TABLE(ALIGNED);
                    /* rest of aligned header is same as verbatim */

                case LZX_BLOCKTYPE_VERBATIM:
                    READ_LENGTHS(MAINTREE, 0, 256);
                    READ_LENGTHS(MAINTREE, 256, pState->main_elements);
                    BUILD_TABLE(MAINTREE);
                    if (LENTABLE(MAINTREE)[0xE8] != 0) pState->intel_started = 1;

                    READ_LENGTHS(LENGTH, 0, LZX_NUM_SECONDARY_LENGTHS);
                    BUILD_TABLE(LENGTH);
                    break;

                case LZX_BLOCKTYPE_UNCOMPRESSED:
                    pState->intel_started = 1; /* because we can't assume otherwise */
                    ENSURE_BITS(16); /* get up to 16 pad bits into the buffer */
                    if (bitsleft > 16) inpos -= 2; /* and align the bitstream! */
                    R0 = inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
                    R1 = inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
                    R2 = inpos[0]|(inpos[1]<<8)|(inpos[2]<<16)|(inpos[3]<<24);inpos+=4;
                    break;

                default:
                    return DECR_ILLEGALDATA;
            }
        }

        /* buffer exhaustion check */
        if (inpos > endinp) {
            /* it's possible to have a file where the next run is less than
             * 16 bits in size. In this case, the READ_HUFFSYM() macro used
             * in building the tables will exhaust the buffer, so we should
             * allow for this, but not allow those accidentally read bits to
             * be used (so we check that there are at least 16 bits
             * remaining - in this boundary case they aren't really part of
             * the compressed data)
             */
            if (inpos > (endinp+2) || bitsleft < 16) return DECR_ILLEGALDATA;
        }

        while ((this_run = pState->block_remaining) > 0 && togo > 0) {
            if (this_run > togo) this_run = togo;
            togo -= this_run;
            pState->block_remaining -= this_run;

            /* apply 2^x-1 mask */
            window_posn &= window_size - 1;
            /* runs can't straddle the window wraparound */
            if ((window_posn + this_run) > window_size)
                return DECR_DATAFORMAT;

            switch (pState->block_type) {

                case LZX_BLOCKTYPE_VERBATIM:
                    while (this_run > 0) {
                        READ_HUFFSYM(MAINTREE, main_element);

                        if (main_element < LZX_NUM_CHARS) {
                            /* literal: 0 to LZX_NUM_CHARS-1 */
                            window[window_posn++] = main_element;
                            this_run--;
                        }
                        else {
                            /* match: LZX_NUM_CHARS + ((slot<<3) | length_header (3 bits)) */
                            main_element -= LZX_NUM_CHARS;

                            match_length = main_element & LZX_NUM_PRIMARY_LENGTHS;
                            if (match_length == LZX_NUM_PRIMARY_LENGTHS) {
                                READ_HUFFSYM(LENGTH, length_footer);
                                match_length += length_footer;
                            }
                            match_length += LZX_MIN_MATCH;

                            match_offset = main_element >> 3;

                            if (match_offset > 2) {
                                /* not repeated offset */
                                if (match_offset != 3) {
                                    extra = extra_bits[match_offset];
                                    READ_BITS(verbatim_bits, extra);
                                    match_offset = position_base[match_offset] - 2 + verbatim_bits;
                                }
                                else {
                                    match_offset = 1;
                                }

                                /* update repeated offset LRU queue */
                                R2 = R1; R1 = R0; R0 = match_offset;
                            }
                            else if (match_offset == 0) {
                                match_offset = R0;
                            }
                            else if (match_offset == 1) {
                                match_offset = R1;
                                R1 = R0; R0 = match_offset;
                            }
                            else /* match_offset == 2 */ {
                                match_offset = R2;
                                R2 = R0; R0 = match_offset;
                            }

                            rundest = window + window_posn;
                            this_run -= match_length;

                            /* copy any wrapped around source data */
                            if (window_posn >= match_offset) {
                                /* no wrap */
                                 runsrc = rundest - match_offset;
                            } else {
                                runsrc = rundest + (window_size - match_offset);
                                copy_length = match_offset - window_posn;
                                if (copy_length < match_length) {
                                     match_length -= copy_length;
                                     window_posn += copy_length;
                                     while (copy_length-- > 0) *rundest++ = *runsrc++;
                                     runsrc = window;
                                }
                            }
                            window_posn += match_length;
 
                            /* copy match data - no worries about destination wraps */
                            while (match_length-- > 0) *rundest++ = *runsrc++;

                        }
                    }
                    break;

                case LZX_BLOCKTYPE_ALIGNED:
                    while (this_run > 0) {
                        READ_HUFFSYM(MAINTREE, main_element);

                        if (main_element < LZX_NUM_CHARS) {
                            /* literal: 0 to LZX_NUM_CHARS-1 */
                            window[window_posn++] = main_element;
                            this_run--;
                        }
                        else {
                            /* match: LZX_NUM_CHARS + ((slot<<3) | length_header (3 bits)) */
                            main_element -= LZX_NUM_CHARS;

                            match_length = main_element & LZX_NUM_PRIMARY_LENGTHS;
                            if (match_length == LZX_NUM_PRIMARY_LENGTHS) {
                                READ_HUFFSYM(LENGTH, length_footer);
                                match_length += length_footer;
                            }
                            match_length += LZX_MIN_MATCH;

                            match_offset = main_element >> 3;

                            if (match_offset > 2) {
                                /* not repeated offset */
                                extra = extra_bits[match_offset];
                                match_offset = position_base[match_offset] - 2;
                                if (extra > 3) {
                                    /* verbatim and aligned bits */
                                    extra -= 3;
                                    READ_BITS(verbatim_bits, extra);
                                    match_offset += (verbatim_bits << 3);
                                    READ_HUFFSYM(ALIGNED, aligned_bits);
                                    match_offset += aligned_bits;
                                }
                                else if (extra == 3) {
                                    /* aligned bits only */
                                    READ_HUFFSYM(ALIGNED, aligned_bits);
                                    match_offset += aligned_bits;
                                }
                                else if (extra > 0) { /* extra==1, extra==2 */
                                    /* verbatim bits only */
                                    READ_BITS(verbatim_bits, extra);
                                    match_offset += verbatim_bits;
                                }
                                else /* extra == 0 */ {
                                    /* ??? */
                                    match_offset = 1;
                                }

                                /* update repeated offset LRU queue */
                                R2 = R1; R1 = R0; R0 = match_offset;
                            }
                            else if (match_offset == 0) {
                                match_offset = R0;
                            }
                            else if (match_offset == 1) {
                                match_offset = R1;
                                R1 = R0; R0 = match_offset;
                            }
                            else /* match_offset == 2 */ {
                                match_offset = R2;
                                R2 = R0; R0 = match_offset;
                            }

                            rundest = window + window_posn;
                            this_run -= match_length;

                            /* copy any wrapped around source data */
                            if (window_posn >= match_offset) {
                                /* no wrap */
                                 runsrc = rundest - match_offset;
                            } else {
                                runsrc = rundest + (window_size - match_offset);
                                copy_length = match_offset - window_posn;
                                if (copy_length < match_length) {
                                     match_length -= copy_length;
                                     window_posn += copy_length;
                                     while (copy_length-- > 0) *rundest++ = *runsrc++;
                                     runsrc = window;
                                }
                            }
                            window_posn += match_length;
 
                            /* copy match data - no worries about destination wraps */
                            while (match_length-- > 0) *rundest++ = *runsrc++;

                        }
                    }
                    break;

                case LZX_BLOCKTYPE_UNCOMPRESSED:
                    if ((inpos + this_run) > endinp) return DECR_ILLEGALDATA;
                    memcpy(window + window_posn, inpos, (size_t) this_run);
                    inpos += this_run; window_posn += this_run;
                    break;

                default:
                    return DECR_ILLEGALDATA; /* might as well */
            }

        }
    }

    if (togo != 0) return DECR_ILLEGALDATA;
    memcpy(outpos, window + ((!window_posn) ? window_size : window_posn) - outlen, (size_t) outlen);

    pState->window_posn = window_posn;
    pState->R0 = R0;
    pState->R1 = R1;
    pState->R2 = R2;

    /* intel E8 decoding */
    if ((pState->frames_read++ < 32768) && pState->intel_filesize != 0) {
        if (outlen <= 6 || !pState->intel_started) {
            pState->intel_curpos += outlen;
        }
        else {
            UBYTE *data    = outpos;
            UBYTE *dataend = data + outlen - 10;
            LONG curpos    = pState->intel_curpos;
            LONG filesize  = pState->intel_filesize;
            LONG abs_off, rel_off;

            pState->intel_curpos = curpos + outlen;

            while (data < dataend) {
                if (*data++ != 0xE8) { curpos++; continue; }
                abs_off = data[0] | (data[1]<<8) | (data[2]<<16) | (data[3]<<24);
                if ((abs_off >= -curpos) && (abs_off < filesize)) {
                    rel_off = (abs_off >= 0) ? abs_off - curpos : abs_off + filesize;
                    data[0] = (UBYTE) rel_off;
                    data[1] = (UBYTE) (rel_off >> 8);
                    data[2] = (UBYTE) (rel_off >> 16);
                    data[3] = (UBYTE) (rel_off >> 24);
                }
                data += 4;
                curpos += 5;
            }
        }
    }
    return DECR_OK;
}

#ifdef LZX_CHM_TESTDRIVER
int main(int c, char **v)
{
    FILE *fin, *fout;
    struct LZXstate state;
    UBYTE ibuf[16384];
    UBYTE obuf[32768];
    int ilen, olen;
    int status;
    int i;
    int count=0;
    int w = atoi(v[1]);
    LZXinit(&state, 1 << w);
    fout = fopen(v[2], "wb");
    for (i=3; i<c; i++)
    {
        fin = fopen(v[i], "rb");
        ilen = fread(ibuf, 1, 16384, fin);
        status = LZXdecompress(&state, ibuf, obuf, ilen, 32768);
        switch (status)
        {
            case DECR_OK:
                printf("ok\n");
                fwrite(obuf, 1, 32768, fout);
                break;
            case DECR_DATAFORMAT:
                printf("bad format\n");
                break;
            case DECR_ILLEGALDATA:
                printf("illegal data\n");
                break;
            case DECR_NOMEMORY:
                printf("no memory\n");
                break;
            default:
                break;
        }
        fclose(fin);
        if (++count == 2)
        {
            count = 0;
            LZXreset(&state);
        }
    }
    fclose(fout);
}
#endif
