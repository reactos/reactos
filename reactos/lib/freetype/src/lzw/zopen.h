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
 * Copyright (c) 2004
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

#include <stdio.h>

#define	BITS		16		/* Default bits. */
#define	HSIZE		69001		/* 95% occupancy */

/* A code_int must be able to hold 2**BITS values of type int, and also -1. */
typedef long code_int;
typedef long count_int;

typedef unsigned char char_type;

typedef enum {
	S_START, S_MIDDLE, S_EOF
} zs_enum;

typedef struct {
	unsigned char *next_in;
	unsigned int avail_in;
	unsigned long total_in;

	unsigned char *next_out;
	unsigned int avail_out;
	unsigned long total_out;

	zs_enum zs_state;		/* State of computation */
	int zs_n_bits;			/* Number of bits/code. */
	int zs_maxbits;			/* User settable max # bits/code. */
	code_int zs_maxcode;		/* Maximum code, given n_bits. */
	code_int zs_maxmaxcode;		/* Should NEVER generate this code. */
	count_int zs_htab [HSIZE];
	unsigned short zs_codetab [HSIZE];
	code_int zs_hsize;		/* For dynamic table sizing. */
	code_int zs_free_ent;		/* First unused entry. */
	/*
	 * Block compression parameters -- after all codes are used up,
	 * and compression rate changes, start over.
	 */
	int zs_block_compress;
	int zs_clear_flg;
	int zs_offset;
	long zs_in_count;		/* Remaining uncompressed bytes. */
	char_type zs_buf_len;
	char_type zs_buf[BITS];		/* Temporary buffer if we need
					   to read more to accumulate
					   n_bits. */
	union {
		struct {
			char_type *zs_stackp;
			int zs_finchar;
			code_int zs_code, zs_oldcode, zs_incode;
			int zs_roffset, zs_size;
			char_type zs_gbuf[BITS];
		} r;			/* Read parameters */
	} u;
} s_zstate_t;

static code_int getcode(s_zstate_t *);
#if 0
static int      zclose(s_zstate_t *);
#endif
static void     zinit(s_zstate_t *);
static int      zread(s_zstate_t *);
