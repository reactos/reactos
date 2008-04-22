/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 * Copyright (c) 1994 John S. Dyson
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
 *	from: @(#)vmparam.h	5.9 (Berkeley) 5/12/91
 */


#ifndef _MACHINE_VMPARAM_H_
#define _MACHINE_VMPARAM_H_ 1

/*
 * Machine dependent constants for 386.
 */

/*
 * Virtual memory related constants, all in bytes
 */
#define	MAXTSIZ		(16UL*1024*1024)	/* max text size */
#ifndef DFLDSIZ
#define	DFLDSIZ		(64UL*1024*1024)	/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(128UL*1024*1024)	/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(8UL*1024*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(64UL*1024*1024)	/* max stack size */
#endif
#ifndef SGROWSIZ
#define SGROWSIZ	(128UL*1024)		/* amount to grow stack */
#endif

#define USRTEXT		(1*NBPG)		/* base of user text XXX bogus */

/*
 * Default sizes of swap allocation chunks (see dmap.h).
 * The actual values may be changed in vminit() based on MAXDSIZ.
 * With MAXDSIZ of 16Mb and NDMAP of 38, dmmax will be 1024.
 */
#define	DMMIN	32			/* smallest swap allocation */
#define	DMMAX	4096			/* largest potential swap allocation */
#define	DMTEXT	1024			/* swap allocation for text */

/*
 * Sizes of the system and user portions of the system page table.
 */
#define	SYSPTSIZE 	(2*NPTEPG)
#define	USRPTSIZE 	(2*NPTEPG)

/*
 * Size of the Shared Memory Pages page table.
 */
#ifndef	SHMMAXPGS
#define	SHMMAXPGS	1024		/* XXX until we have more kmap space */
#endif

/*
 * Size of User Raw I/O map
 */
#define	USRIOSIZE 	1024

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * Mach derived constants
 */

/* user/kernel map constants */
#ifdef OSKIT
#define KERNBASE 0	/* XXX */
#else
#define	KERNBASE ((0x400-1-NKPDE)*(NBPG*NPTEPG))
#endif

#define VM_MIN_ADDRESS		((vm_offset_t)0)
#define VM_MAXUSER_ADDRESS	((vm_offset_t)KERNBASE - (NBPG*(NPTEPG+UPAGES)))
#define USRSTACK VM_MAXUSER_ADDRESS
#define UPT_MIN_ADDRESS		((vm_offset_t)KERNBASE - (NBPG*NPTEPG))
#define UPT_MAX_ADDRESS		((vm_offset_t)KERNBASE - (NBPG*(NKPDE+2)))
#define VM_MAX_ADDRESS		UPT_MAX_ADDRESS
#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t)KERNBASE - (NBPG*(NKPDE+2)))
#define UPDT			VM_MIN_KERNEL_ADDRESS
#define KPT_MIN_ADDRESS		((vm_offset_t)KERNBASE - NBPG*(NKPDE+1))
#define KPT_MAX_ADDRESS		((vm_offset_t)KERNBASE - NBPG)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t)KERNBASE + NKPDE*NBPG*NPTEPG)

/* virtual sizes (bytes) for various kernel submaps */
#define VM_KMEM_SIZE		(32 * 1024 * 1024)
#define VM_PHYS_SIZE		(USRIOSIZE*CLBYTES)

#endif /* _MACHINE_VMPARAM_H_ */
