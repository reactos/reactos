/*	$NetBSD: zopen.c,v 1.8 2003/08/07 11:13:29 agc Exp $	*/

/*-
 * Copyright (c) 1985, 1986, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Diomidis Spinellis and James A. Woods, derived from original
 * work by Spencer Thomas and Joseph Orost.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 *
 * Copyright (c) 2004, 2005
 *	Albert Chin-A-Young.
 *
 * Modified to work with FreeType's PCF driver.
 *
 */

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)zopen.c	8.1 (Berkeley) 6/27/93";
#else
static char rcsid[] = "$NetBSD: zopen.c,v 1.8 2003/08/07 11:13:29 agc Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

/*-
 * fcompress.c - File compression ala IEEE Computer, June 1984.
 *
 * Compress authors:
 *		Spencer W. Thomas	(decvax!utah-cs!thomas)
 *		Jim McKie		(decvax!mcvax!jim)
 *		Steve Davies		(decvax!vax135!petsd!peora!srd)
 *		Ken Turkowski		(decvax!decwrl!turtlevax!ken)
 *		James A. Woods		(decvax!ihnp4!ames!jaw)
 *		Joe Orost		(decvax!vax135!petsd!joe)
 *
 * Cleaned up and converted to library returning I/O streams by
 * Diomidis Spinellis <dds@doc.ic.ac.uk>.
 */

#include <ctype.h>
#if 0
#include <signal.h>
#endif
#include <stdlib.h>
#include <string.h>
#if 0
#include <unistd.h>
#endif

#if 0
static char_type magic_header[] =
	{ 0x1f, 0x9d };		/* 1F 9D */
#endif

#define	BIT_MASK	0x1f		/* Defines for third byte of header. */
#define	BLOCK_MASK	0x80

/*
 * Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
 * a fourth header byte (for expansion).
 */
#define	INIT_BITS 9			/* Initial number of bits/code. */

#define	MAXCODE(n_bits)	((1 << (n_bits)) - 1)

/* Definitions to retain old variable names */
#define	fp		zs->zs_fp
#define	state		zs->zs_state
#define	n_bits		zs->zs_n_bits
#define	maxbits		zs->zs_maxbits
#define	maxcode		zs->zs_maxcode
#define	maxmaxcode	zs->zs_maxmaxcode
#define	htab		zs->zs_htab
#define	codetab		zs->zs_codetab
#define	hsize		zs->zs_hsize
#define	free_ent	zs->zs_free_ent
#define	block_compress	zs->zs_block_compress
#define	clear_flg	zs->zs_clear_flg
#define	offset		zs->zs_offset
#define	in_count	zs->zs_in_count
#define	buf_len		zs->zs_buf_len
#define	buf		zs->zs_buf
#define	stackp		zs->u.r.zs_stackp
#define	finchar		zs->u.r.zs_finchar
#define	code		zs->u.r.zs_code
#define	oldcode		zs->u.r.zs_oldcode
#define	incode		zs->u.r.zs_incode
#define	roffset		zs->u.r.zs_roffset
#define	size		zs->u.r.zs_size
#define	gbuf		zs->u.r.zs_gbuf

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type as
 * the codetab.  The tab_suffix table needs 2**BITS characters.  We get this
 * from the beginning of htab.  The output stack uses the rest of htab, and
 * contains characters.  There is plenty of room for any possible stack
 * (stack used to be 8000 characters).
 */

#define	htabof(i)	htab[i]
#define	codetabof(i)	codetab[i]

#define	tab_prefixof(i)	codetabof(i)
#define	tab_suffixof(i)	((char_type *)(htab))[i]
#define	de_stack	((char_type *)&tab_suffixof(1 << BITS))

#define	CHECK_GAP 10000		/* Ratio check interval. */

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */
#define	FIRST	257		/* First free entry. */
#define	CLEAR	256		/* Table clear output code. */

/*-
 * Algorithm from "A Technique for High Performance Data Compression",
 * Terry A. Welch, IEEE Computer Vol 17, No 6 (June 1984), pp 8-19.
 *
 * Algorithm:
 * 	Modified Lempel-Ziv method (LZW).  Basically finds common
 * substrings and replaces them with a variable size code.  This is
 * deterministic, and can be done on the fly.  Thus, the decompression
 * procedure needs no input table, but tracks the way the table was built.
 */

#if 0
static int
zclose(s_zstate_t *zs)
{
	free(zs);
	return (0);
}
#endif

/*-
 * Output the given code.
 * Inputs:
 * 	code:	A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *		that n_bits =< (long)wordsize - 1.
 * Outputs:
 * 	Outputs code to the file.
 * Assumptions:
 *	Chars are 8 bits long.
 * Algorithm:
 * 	Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static const char_type rmask[9] =
	{0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

/*
 * Decompress read.  This routine adapts to the codes in the file building
 * the "string" table on-the-fly; requiring no table to be stored in the
 * compressed file.  The tables used herein are shared with those of the
 * compress() routine.  See the definitions above.
 */
static int
zread(s_zstate_t *zs)
{
	unsigned int count;

	if (in_count == 0)
		return -1;
	if (zs->avail_out == 0)
		return 0;

	count = zs->avail_out;
	switch (state) {
	case S_START:
		state = S_MIDDLE;
		break;
	case S_MIDDLE:
		goto middle;
	case S_EOF:
		goto eof;
	}

	maxbits = *(zs->next_in);	/* Set -b from file. */
	zs->avail_in--;
	zs->next_in++;
	zs->total_in++;
	in_count--;
	block_compress = maxbits & BLOCK_MASK;
	maxbits &= BIT_MASK;
	maxmaxcode = 1L << maxbits;
	if (maxbits > BITS) {
		return -1;
	}
	/* As above, initialize the first 256 entries in the table. */
	maxcode = MAXCODE(n_bits = INIT_BITS);
	for (code = 255; code >= 0; code--) {
		tab_prefixof(code) = 0;
		tab_suffixof(code) = (char_type) code;
	}
	free_ent = block_compress ? FIRST : 256;

	finchar = oldcode = getcode(zs);
	if (oldcode == -1)		/* EOF already? */
		return 0;		/* Get out of here */

	/* First code must be 8 bits = char. */
	*(zs->next_out)++ = (unsigned char)finchar;
	zs->total_out++;
	count--;
	stackp = de_stack;

	while ((code = getcode(zs)) > -1) {
		if ((code == CLEAR) && block_compress) {
			for (code = 255; code >= 0; code--)
				tab_prefixof(code) = 0;
			clear_flg = 1;
			free_ent = FIRST - 1;
			if ((code = getcode(zs)) == -1)
				/* O, untimely death! */
				break;
		}
		incode = code;

		/* Special case for KwKwK string. */
		if (code >= free_ent) {
			*stackp++ = (unsigned char)finchar;
			code = oldcode;
		}

		/* Generate output characters in reverse order. */
		while (code >= 256) {
			*stackp++ = tab_suffixof(code);
			code = tab_prefixof(code);
		}
		*stackp++ = (unsigned char)(finchar = tab_suffixof(code));

		/* And put them out in forward order.  */
middle:
		if (stackp == de_stack)
			continue;

		do {
			if (count-- == 0) {
				return zs->avail_out;
			}
			*(zs->next_out)++ = *--stackp;
			zs->total_out++;
		} while (stackp > de_stack);

		/* Generate the new entry. */
		if ((code = free_ent) < maxmaxcode) {
			tab_prefixof(code) = (unsigned short)oldcode;
			tab_suffixof(code) = (unsigned char)finchar;
			free_ent = code + 1;
		}

		/* Remember previous code. */
		oldcode = incode;
	}
	/* state = S_EOF; */
eof:	return (zs->avail_out - count);
}

/*-
 * Read one code from the standard input.  If EOF, return -1.
 * Inputs:
 * 	stdin
 * Outputs:
 * 	code or -1 is returned.
 */
static code_int
getcode(s_zstate_t *zs)
{
	code_int gcode;
	int r_off, bits;
	char_type *bp;

	bp = gbuf;
	if (clear_flg > 0 || roffset >= size || free_ent > maxcode) {
		/*
		 * If the next entry will be too big for the current gcode
		 * size, then we must increase the size.  This implies reading
		 * a new buffer full, too.
		 */
		if (free_ent > maxcode) {
			n_bits++;
			if (n_bits == maxbits)	/* Won't get any bigger now. */
				maxcode = maxmaxcode;
			else
				maxcode = MAXCODE(n_bits);
		}
		if (clear_flg > 0) {
			maxcode = MAXCODE(n_bits = INIT_BITS);
			clear_flg = 0;
		}
		if ( zs->avail_in < (unsigned int)n_bits && in_count > (long)n_bits ) {
			memcpy (buf, zs->next_in, zs->avail_in);
			buf_len = (unsigned char)zs->avail_in;
			zs->avail_in = 0;
			return -1;
		}
		if (buf_len) {
			memcpy (gbuf, buf, buf_len);
			memcpy (gbuf + buf_len, zs->next_in,
				n_bits - buf_len);
			zs->next_in += n_bits - buf_len;
			zs->avail_in -= n_bits - buf_len;
			buf_len = 0;
			zs->total_in += n_bits;
			size = n_bits;
			in_count -= n_bits;
		} else {
			if (in_count > n_bits) {
				memcpy (gbuf, zs->next_in, n_bits);
				zs->next_in += n_bits;
				zs->avail_in -= n_bits;
				zs->total_in += n_bits;
				size = n_bits;
				in_count -= n_bits;
			} else {
				memcpy (gbuf, zs->next_in, in_count);
				zs->next_in += in_count;
				zs->avail_in -= in_count;
				zs->total_in += in_count;
				size = in_count;
				in_count = 0;
			}
		}
		roffset = 0;
		/* Round size down to integral number of codes. */
		size = (size << 3) - (n_bits - 1);
	}
	r_off = roffset;
	bits = n_bits;

	/* Get to the first byte. */
	bp += (r_off >> 3);
	r_off &= 7;

	/* Get first part (low order bits). */
	gcode = (*bp++ >> r_off);
	bits -= (8 - r_off);
	r_off = 8 - r_off;	/* Now, roffset into gcode word. */

	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if (bits >= 8) {
		gcode |= *bp++ << r_off;
		r_off += 8;
		bits -= 8;
	}

	/* High order bits. */
	gcode |= (*bp & rmask[bits]) << r_off;
	roffset += n_bits;

	return (gcode);
}

static void
zinit(s_zstate_t *zs)
{
	memset(zs, 0, sizeof (s_zstate_t));

	maxbits = BITS;			/* User settable max # bits/code. */
	maxmaxcode = 1 << maxbits;	/* Should NEVER generate this code. */
	hsize = HSIZE;			/* For dynamic table sizing. */
	free_ent = 0;			/* First unused entry. */
	block_compress = BLOCK_MASK;
	clear_flg = 0;
	state = S_START;
	roffset = 0;
	size = 0;
}
