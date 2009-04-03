/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)malloc.h	8.3 (Berkeley) 1/12/94
 */

#ifndef _SYS_MALLOC_H_
#define	_SYS_MALLOC_H_

#ifndef OSKIT
#define KMEMSTATS
#endif

/*
 * flags to malloc
 */
#define	M_WAITOK	0x0000
#define	M_NOWAIT	0x0001
#define M_KERNEL	0x0002

/*
 * Types of memory to be allocated
 */
#define	M_FREE		0	/* should be on free list */
#define	M_MBUF		1	/* mbuf */
#define	M_DEVBUF	2	/* device driver memory */
#define	M_SOCKET	3	/* socket structure */
#define	M_PCB		4	/* protocol control block */
#define	M_RTABLE	5	/* routing tables */
#define	M_HTABLE	6	/* IMP host tables */
#define	M_FTABLE	7	/* fragment reassembly header */
#define	M_ZOMBIE	8	/* zombie proc status */
#define	M_IFADDR	9	/* interface address */
#define	M_SOOPTS	10	/* socket options */
#define	M_SONAME	11	/* socket name */
#define	M_NAMEI		12	/* namei path name buffer */
#define	M_GPROF		13	/* kernel profiling buffer */
#define	M_IOCTLOPS	14	/* ioctl data buffer */
#define	M_MAPMEM	15	/* mapped memory descriptors */
#define	M_CRED		16	/* credentials */
#define	M_PGRP		17	/* process group header */
#define	M_SESSION	18	/* session header */
#define	M_IOV		19	/* large iov's */
#define	M_MOUNT		20	/* vfs mount struct */
#define	M_FHANDLE	21	/* network file handle */
#define	M_NFSREQ	22	/* NFS request header */
#define	M_NFSMNT	23	/* NFS mount structure */
#define	M_NFSNODE	24	/* NFS vnode private part */
#define	M_VNODE		25	/* Dynamically allocated vnodes */
#define	M_CACHE		26	/* Dynamically allocated cache entries */
#define	M_DQUOT		27	/* UFS quota entries */
#define	M_UFSMNT	28	/* UFS mount structure */
#define	M_SHM		29	/* SVID compatible shared memory segments */
#define	M_VMMAP		30	/* VM map structures */
#define	M_VMMAPENT	31	/* VM map entry structures */
#define	M_VMOBJ		32	/* VM object structure */
#define	M_VMOBJHASH	33	/* VM object hash structure */
#define	M_VMPMAP	34	/* VM pmap */
#define	M_VMPVENT	35	/* VM phys-virt mapping entry */
#define	M_VMPAGER	36	/* XXX: VM pager struct */
#define	M_VMPGDATA	37	/* XXX: VM pager private data */
#define	M_FILE		38	/* Open file structure */
#define	M_FILEDESC	39	/* Open file descriptor table */
#define	M_LOCKF		40	/* Byte-range locking structures */
#define	M_PROC		41	/* Proc structures */
#define	M_SUBPROC	42	/* Proc sub-structures */
#define	M_SEGMENT	43	/* Segment for LFS */
#define	M_LFSNODE	44	/* LFS vnode private part */
#define	M_FFSNODE	45	/* FFS vnode private part */
#define	M_MFSNODE	46	/* MFS vnode private part */
#define	M_NQLEASE	47	/* Nqnfs lease */
#define	M_NQMHOST	48	/* Nqnfs host address table */
#define	M_NETADDR	49	/* Export host address structure */
#define	M_NFSSVC	50	/* Nfs server structure */
#define	M_NFSUID	51	/* Nfs uid mapping structure */
#define	M_NFSD		52	/* Nfs server daemon structure */
#define	M_IPMOPTS	53	/* internet multicast options */
#define	M_IPMADDR	54	/* internet multicast address */
#define	M_IFMADDR	55	/* link-level multicast address */
#define	M_MRTABLE	56	/* multicast routing tables */
#define M_ISOFSMNT	57	/* ISOFS mount structure */
#define M_ISOFSNODE	58	/* ISOFS vnode private part */
#define M_MSDOSFSMNT	59	/* MSDOSFS mount structure */
#define M_MSDOSFSNODE	60	/* MSDOSFS vnode private part */
#define M_MSDOSFSFAT	61	/* MSDOSFS file allocation table */
#define M_DEVFSMNT	62	/* DEVFS mount structure */
#define M_DEVFSBACK	63	/* DEVFS Back node */
#define M_DEVFSFRONT	64	/* DEVFS Front node */
#define M_DEVFSNODE	65	/* DEVFS node */
#define	M_TEMP		74	/* misc temporary data buffers */
#define M_TTYS		75	/* tty data structures */
#define M_GZIP		76	/* Gzip trees */
#define M_IPFW		77	/* IpFw/IpAcct chain's */
#define M_DEVL		78	/* isa_device lists in userconfig() */
#define	M_LAST		79	/* Must be last type + 1 */

#define INITKMEMNAMES { \
	"free",		/* 0 M_FREE */ \
	"mbuf",		/* 1 M_MBUF */ \
	"devbuf",	/* 2 M_DEVBUF */ \
	"socket",	/* 3 M_SOCKET */ \
	"pcb",		/* 4 M_PCB */ \
	"routetbl",	/* 5 M_RTABLE */ \
	"hosttbl",	/* 6 M_HTABLE */ \
	"fragtbl",	/* 7 M_FTABLE */ \
	"zombie",	/* 8 M_ZOMBIE */ \
	"ifaddr",	/* 9 M_IFADDR */ \
	"soopts",	/* 10 M_SOOPTS */ \
	"soname",	/* 11 M_SONAME */ \
	"namei",	/* 12 M_NAMEI */ \
	"gprof",	/* 13 M_GPROF */ \
	"ioctlops",	/* 14 M_IOCTLOPS */ \
	"mapmem",	/* 15 M_MAPMEM */ \
	"cred",		/* 16 M_CRED */ \
	"pgrp",		/* 17 M_PGRP */ \
	"session",	/* 18 M_SESSION */ \
	"iov",		/* 19 M_IOV */ \
	"mount",	/* 20 M_MOUNT */ \
	"fhandle",	/* 21 M_FHANDLE */ \
	"NFS req",	/* 22 M_NFSREQ */ \
	"NFS mount",	/* 23 M_NFSMNT */ \
	"NFS node",	/* 24 M_NFSNODE */ \
	"vnodes",	/* 25 M_VNODE */ \
	"namecache",	/* 26 M_CACHE */ \
	"UFS quota",	/* 27 M_DQUOT */ \
	"UFS mount",	/* 28 M_UFSMNT */ \
	"shm",		/* 29 M_SHM */ \
	"VM map",	/* 30 M_VMMAP */ \
	"VM mapent",	/* 31 M_VMMAPENT */ \
	"VM object",	/* 32 M_VMOBJ */ \
	"VM objhash",	/* 33 M_VMOBJHASH */ \
	"VM pmap",	/* 34 M_VMPMAP */ \
	"VM pvmap",	/* 35 M_VMPVENT */ \
	"VM pager",	/* 36 M_VMPAGER */ \
	"VM pgdata",	/* 37 M_VMPGDATA */ \
	"file",		/* 38 M_FILE */ \
	"file desc",	/* 39 M_FILEDESC */ \
	"lockf",	/* 40 M_LOCKF */ \
	"proc",		/* 41 M_PROC */ \
	"subproc",	/* 42 M_SUBPROC */ \
	"LFS segment",	/* 43 M_SEGMENT */ \
	"LFS node",	/* 44 M_LFSNODE */ \
	"FFS node",	/* 45 M_FFSNODE */ \
	"MFS node",	/* 46 M_MFSNODE */ \
	"NQNFS Lease",	/* 47 M_NQLEASE */ \
	"NQNFS Host",	/* 48 M_NQMHOST */ \
	"Export Host",	/* 49 M_NETADDR */ \
	"NFS srvsock",	/* 50 M_NFSSVC */ \
	"NFS uid",	/* 51 M_NFSUID */ \
	"NFS daemon",	/* 52 M_NFSD */ \
	"ip_moptions",	/* 53 M_IPMOPTS */ \
	"in_multi",	/* 54 M_IPMADDR */ \
	"ether_multi",	/* 55 M_IFMADDR */ \
	"mrt",		/* 56 M_MRTABLE */ \
	"ISOFS mount",	/* 57 M_ISOFSMNT */ \
	"ISOFS node",	/* 58 M_ISOFSNODE */ \
	"MSDOSFS mount",/* 59 M_MSDOSFSMNT */ \
	"MSDOSFS node",	/* 60 M_MSDOSFSNODE */ \
	"MSDOSFS FAT",  /* 61 M_MSDOSFSFAR */ \
	"DEVFS mount",	/* 62 M_DEVFSMNT */ \
	"DEVFS back",	/* 63 M_DEVFSBACK */ \
	"DEVFS front",	/* 64 M_DEVFSFRONT */ \
	"DEVFS node",	/* 65 M_DEVFSNODE */ \
	NULL, \
	NULL, NULL, NULL, NULL, NULL, \
	NULL, NULL, \
	"temp",		/* 74 M_TEMP */ \
	"ttys",		/* 75 M_TTYS */ \
	"Gzip trees",	/* 76 M_GZIP */ \
	"IpFw/IpAcct",	/* 77 M_IPFW */ \
	"isa_devlist",	/* 78 M_DEVL */ \
}

struct kmemstats {
	long	ks_inuse;	/* # of packets of this type currently in use */
	long	ks_calls;	/* total packets of this type ever allocated */
	long 	ks_memuse;	/* total memory held in bytes */
	u_short	ks_limblocks;	/* number of times blocked for hitting limit */
	u_short	ks_mapblocks;	/* number of times blocked for kernel map */
	long	ks_maxused;	/* maximum number ever used */
	long	ks_limit;	/* most that are allowed to exist */
	long	ks_size;	/* sizes of this thing that are allocated */
	long	ks_spare;
};

/*
 * Array of descriptors that describe the contents of each page
 */
struct kmemusage {
	short ku_indx;		/* bucket index */
	union {
		u_short freecnt;/* for small allocations, free pieces in page */
		u_short pagecnt;/* for large allocations, pages alloced */
	} ku_un;
};
#define ku_freecnt ku_un.freecnt
#define ku_pagecnt ku_un.pagecnt

/*
 * Set of buckets for each size of memory block that is retained
 */
struct kmembuckets {
	caddr_t kb_next;	/* list of free blocks */
	caddr_t kb_last;	/* last free block */
	long	kb_calls;	/* total calls to allocate this size */
	long	kb_total;	/* total number of blocks allocated */
	long	kb_totalfree;	/* # of free elements in this bucket */
	long	kb_elmpercl;	/* # of elements in this sized allocation */
	long	kb_highwat;	/* high water mark */
	long	kb_couldfree;	/* over high water mark and could free */
};

#ifdef KERNEL
#define	MINALLOCSIZE	(1 << MINBUCKET)
#define BUCKETINDX(size) \
	(size) <= (MINALLOCSIZE * 128) \
		? (size) <= (MINALLOCSIZE * 8) \
			? (size) <= (MINALLOCSIZE * 2) \
				? (size) <= (MINALLOCSIZE * 1) \
					? (MINBUCKET + 0) \
					: (MINBUCKET + 1) \
				: (size) <= (MINALLOCSIZE * 4) \
					? (MINBUCKET + 2) \
					: (MINBUCKET + 3) \
			: (size) <= (MINALLOCSIZE* 32) \
				? (size) <= (MINALLOCSIZE * 16) \
					? (MINBUCKET + 4) \
					: (MINBUCKET + 5) \
				: (size) <= (MINALLOCSIZE * 64) \
					? (MINBUCKET + 6) \
					: (MINBUCKET + 7) \
		: (size) <= (MINALLOCSIZE * 2048) \
			? (size) <= (MINALLOCSIZE * 512) \
				? (size) <= (MINALLOCSIZE * 256) \
					? (MINBUCKET + 8) \
					: (MINBUCKET + 9) \
				: (size) <= (MINALLOCSIZE * 1024) \
					? (MINBUCKET + 10) \
					: (MINBUCKET + 11) \
			: (size) <= (MINALLOCSIZE * 8192) \
				? (size) <= (MINALLOCSIZE * 4096) \
					? (MINBUCKET + 12) \
					: (MINBUCKET + 13) \
				: (size) <= (MINALLOCSIZE * 16384) \
					? (MINBUCKET + 14) \
					: (MINBUCKET + 15)

/*
 * Turn virtual addresses into kmem map indicies
 */
#define kmemxtob(alloc)	(kmembase + (alloc) * NBPG)
#define btokmemx(addr)	(((caddr_t)(addr) - kmembase) / NBPG)
#define btokup(addr)	(&kmemusage[((caddr_t)(addr) - kmembase) >> CLSHIFT])

/*
 * Macro versions for the usual cases of malloc/free
 */
#if defined(KMEMSTATS) || defined(DIAGNOSTIC)
#define	MALLOC(space, cast, size, type, flags) \
	(space) = (cast)fbsd_malloc((u_long)(size), __FILE__, __LINE__, type, flags)
#define FREE(addr, type) fbsd_free((caddr_t)(addr), __FILE__, __LINE__, type)

#else /* do not collect statistics */
#define	MALLOC(space, cast, size, type, flags) { \
	register struct kmembuckets *kbp = &bucket[BUCKETINDX(size)]; \
	long s = splimp(); \
	if (kbp->kb_next == NULL) { \
		(space) = (cast)fbsd_malloc((u_long)(size), __FILE__, __LINE__, type, flags); \
	} else { \
		(space) = (cast)kbp->kb_next; \
		kbp->kb_next = *(caddr_t *)(space); \
	} \
	splx(s); \
}

#define FREE(addr, type) { \
	register struct kmembuckets *kbp; \
	register struct kmemusage *kup = btokup(addr); \
	long s = splimp(); \
	if (1 << kup->ku_indx > MAXALLOCSAVE) { \
		fbsd_free((caddr_t)(addr), __FILE__, __LINE__, type); \
	} else { \
		kbp = &bucket[kup->ku_indx]; \
		if (kbp->kb_next == NULL) \
			kbp->kb_next = (caddr_t)(addr); \
		else \
			*(caddr_t *)(kbp->kb_last) = (caddr_t)(addr); \
		*(caddr_t *)(addr) = NULL; \
		kbp->kb_last = (caddr_t)(addr); \
	} \
	splx(s); \
}
#endif /* do not collect statistics */

extern struct kmemstats kmemstats[];
extern struct kmemusage *kmemusage;
extern char *kmembase;
extern struct kmembuckets bucket[];
#ifndef __REACTOS__
extern void *malloc __P((unsigned long size, ...));
extern void free __P((void *addr, ...));
#else
#define malloc(size, flags, id) fbsd_malloc(size, __FILE__, __LINE__)
#define free(area, flags)   fbsd_free(area, __FILE__, __LINE__)
#endif
#endif /* KERNEL */
#endif /* !_SYS_MALLOC_H_ */
