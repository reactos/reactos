/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	from: @(#)param.h	5.8 (Berkeley) 6/28/91
 */

#ifndef _MACHINE_PARAM_H_
#define	_MACHINE_PARAM_H_

/*
 * Machine dependent constants for Intel 386.
 */

#define MACHINE		"i386"
#define MID_MACHINE	MID_I386

/*
 * Round p (pointer or byte index) up to a correctly-aligned value
 * for all data types (int, long, ...).   The result is unsigned int
 * and must be cast to any desired pointer type.
 */
#define ALIGNBYTES	(sizeof(int) - 1)
#define ALIGN(p)	(((unsigned)(p) + ALIGNBYTES) & ~ALIGNBYTES)

#ifndef PAGE_SIZE
/* XXX PGSHIFT and PG_SHIFT are two names for the same thing */
#define PGSHIFT		12		/* LOG2(NBPG) */
#define PAGE_SHIFT	12
#define NBPG		(1 << PAGE_SHIFT)	/* bytes/page */
#define PAGE_SIZE	(1 << PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE-1)
#define PGOFSET		(NBPG-1)	/* byte offset into page */
#define NPTEPG		(NBPG/(sizeof (pt_entry_t)))
#endif

/* XXX PDRSHIFT and PD_SHIFT are two names for the same thing */
#define PDRSHIFT	22		/* LOG2(NBPDR) */
#define NBPDR		(1 << PDRSHIFT)	/* bytes/page dir */
#define PDROFSET	(NBPDR-1)	/* byte offset into page dir */

/*
 * XXX This should really be KPTDPTDI << PDRSHIFT, but since KPTDPTDI is
 * defined in pmap.h which is included after this we can't do that
 * (YET!)
 */
#define BTOPKERNBASE	(KERNBASE >> PGSHIFT)

#define DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define DEV_BSIZE	(1 << DEV_BSHIFT)

#define BLKDEV_IOSIZE	2048
#define MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

#define CLSIZELOG2	0
#define CLSIZE		(1 << CLSIZELOG2)

/* NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE */
#define SSIZE	1		/* initial stack size/NBPG */
#define SINCR	1		/* increment of stack/NBPG */

#define UPAGES	2		/* pages of u-area */

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than CLBYTES (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#ifndef	MSIZE
#define MSIZE		128		/* size of an mbuf */
#endif	/* MSIZE */

#ifndef	MCLSHIFT
#define MCLSHIFT	11		/* convert bytes to m_buf clusters */
#endif	/* MCLSHIFT */
#define MCLBYTES	(1 << MCLSHIFT)	/* size of an m_buf cluster */
#define MCLOFSET	(MCLBYTES - 1)	/* offset within an m_buf cluster */

/*
 * Some macros for units conversion
 */
/* Core clicks (4096 bytes) to segments and vice versa */
#define ctos(x)	(x)
#define stoc(x)	(x)

/* Core clicks (4096 bytes) to disk blocks */
#define ctod(x)	((x)<<(PGSHIFT-DEV_BSHIFT))
#define dtoc(x)	((x)>>(PGSHIFT-DEV_BSHIFT))
#define dtob(x)	((x)<<DEV_BSHIFT)

/* clicks to bytes */
#define ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */
#define btoc(x)	(((unsigned)(x)+(NBPG-1))>>PGSHIFT)

/*
 * This is messy and perhaps slow because `bytes' may be an off_t.  We
 * have to shift an unsigned type to avoid sign extension and we don't
 * want to widen `bytes' unnecessarily.  Assume that off_t is long long
 * and daddr_t is unsigned long.
 */
#define btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	(sizeof (bytes) > sizeof(long) \
	 ? (unsigned long)((unsigned long long)(bytes) >> DEV_BSHIFT) \
	 : (unsigned long)((unsigned long)(bytes) >> DEV_BSHIFT))
#define dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned long long)(db) << DEV_BSHIFT)

/*
 * Mach derived conversion macros
 */
#define trunc_page(x)		((unsigned)(x) & ~(NBPG-1))
#define round_page(x)		((((unsigned)(x)) + NBPG - 1) & ~(NBPG-1))

#define atop(x)			((unsigned)(x) >> PG_SHIFT)
#define ptoa(x)			((unsigned)(x) << PG_SHIFT)

#define i386_round_pdr(x)	((((unsigned)(x)) + NBPDR - 1) & ~(NBPDR-1))
#define i386_trunc_pdr(x)	((unsigned)(x) & ~(NBPDR-1))
#define i386_round_page(x)	((((unsigned)(x)) + NBPG - 1) & ~(NBPG-1))
#define i386_trunc_page(x)	((unsigned)(x) & ~(NBPG-1))
#define i386_btod(x)		((unsigned)(x) >> PDRSHIFT)
#define i386_dtob(x)		((unsigned)(x) << PDRSHIFT)
#define i386_btop(x)		((unsigned)(x) >> PGSHIFT)
#define i386_ptob(x)		((unsigned)(x) << PGSHIFT)

#endif /* !_MACHINE_PARAM_H_ */
