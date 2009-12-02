/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)buf.h	8.7 (Berkeley) 1/21/94
 */

#ifndef _SYS_BUF_H_
#define	_SYS_BUF_H_
#include <sys/queue.h>

#define NOLIST ((struct buf *)0x87654321)

struct buf;

struct iodone_chain {
	long	ic_prev_flags;
	void	(*ic_prev_iodone) __P((struct buf *));
	void	*ic_prev_iodone_chain;
	struct {
		long	ia_long;
		void	*ia_ptr;
	}	ic_args[5];
};

/*
 * The buffer header describes an I/O operation in the kernel.
 */
struct buf {
	LIST_ENTRY(buf) b_hash;		/* Hash chain. */
	LIST_ENTRY(buf) b_vnbufs;	/* Buffer's associated vnode. */
	TAILQ_ENTRY(buf) b_freelist;	/* Free list position if not active. */
	struct	buf *b_actf, **b_actb;	/* Device driver queue when active. */
	struct  proc *b_proc;		/* Associated proc; NULL if kernel. */
	volatile long	b_flags;	/* B_* flags. */
	int	b_qindex;		/* buffer queue index */
	int	b_error;		/* Errno value. */
	long	b_bufsize;		/* Allocated buffer size. */
	long	b_bcount;		/* Valid bytes in buffer. */
	long	b_resid;		/* Remaining I/O. */
	dev_t	b_dev;			/* Device associated with buffer. */
	struct {
		caddr_t	b_addr;		/* Memory, superblocks, indirect etc. */
	} b_un;
	void	*b_saveaddr;		/* Original b_addr for physio. */
	daddr_t	b_lblkno;		/* Logical block number. */
	daddr_t	b_blkno;		/* Underlying physical block number. */
					/* Function to call upon completion. */
	void	(*b_iodone) __P((struct buf *));
					/* For nested b_iodone's. */
	struct	iodone_chain *b_iodone_chain;
	struct	vnode *b_vp;		/* Device vnode. */
	int	b_pfcent;		/* Center page when swapping cluster. */
	int	b_dirtyoff;		/* Offset in buffer of dirty region. */
	int	b_dirtyend;		/* Offset of end of dirty region. */
	struct	ucred *b_rcred;		/* Read credentials reference. */
	struct	ucred *b_wcred;		/* Write credentials reference. */
	int	b_validoff;		/* Offset in buffer of valid region. */
	int	b_validend;		/* Offset of end of valid region. */
	daddr_t	b_pblkno;               /* physical block number */
	caddr_t	b_savekva;              /* saved kva for transfer while bouncing */
	void	*b_driver1;		/* for private use by the driver */
	void	*b_driver2;		/* for private use by the driver */
	void	*b_spc;
	struct	vm_page *b_pages[(MAXPHYS + PAGE_SIZE - 1)/PAGE_SIZE];
	int		b_npages;
};

/* Device driver compatibility definitions. */
#define	b_active b_bcount		/* Driver queue head: drive active. */
#define	b_data	 b_un.b_addr		/* b_un.b_addr is not changeable. */
#define	b_errcnt b_resid		/* Retry count while I/O in progress. */
#define	iodone	 biodone		/* Old name for biodone. */
#define	iowait	 biowait		/* Old name for biowait. */

/*
 * These flags are kept in b_flags.
 */
#define	B_AGE		0x00000001	/* Move to age queue when I/O done. */
#define	B_APPENDWRITE	0x00000002	/* Append-write in progress. */
#define	B_ASYNC		0x00000004	/* Start I/O, do not wait. */
#define	B_BAD		0x00000008	/* Bad block revectoring in progress. */
#define	B_BUSY		0x00000010	/* I/O in progress. */
#define	B_CACHE		0x00000020	/* Bread found us in the cache. */
#define	B_CALL		0x00000040	/* Call b_iodone from biodone. */
#define	B_DELWRI	0x00000080	/* Delay I/O until buffer reused. */
#define	B_DIRTY		0x00000100	/* Dirty page to be pushed out async. */
#define	B_DONE		0x00000200	/* I/O completed. */
#define	B_EINTR		0x00000400	/* I/O was interrupted */
#define	B_ERROR		0x00000800	/* I/O error occurred. */
#define	B_GATHERED	0x00001000	/* LFS: already in a segment. */
#define	B_INVAL		0x00002000	/* Does not contain valid info. */
#define	B_LOCKED	0x00004000	/* Locked in core (not reusable). */
#define	B_NOCACHE	0x00008000	/* Do not cache block after use. */
#define	B_MALLOC	0x00010000	/* malloced b_data */
#define	B_CLUSTEROK		0x00020000	/* Pagein op, so swap() can count it. */
#define	B_PHYS		0x00040000	/* I/O to user memory. */
#define	B_RAW		0x00080000	/* Set by physio for raw transfers. */
#define	B_READ		0x00100000	/* Read buffer. */
#define	B_TAPE		0x00200000	/* Magnetic tape I/O. */
#define	B_RELBUF	0x00400000	/* Release VMIO buffer. */
#define	B_WANTED	0x00800000	/* Process wants this buffer. */
#define	B_WRITE		0x00000000	/* Write buffer (pseudo flag). */
#define	B_WRITEINPROG	0x01000000	/* Write in progress. */
#define	B_XXX		0x02000000	/* Debugging flag. */
#define	B_PAGING	0x04000000	/* volatile paging I/O -- bypass VMIO */
#define B_VMIO		0x20000000	/* VMIO flag */
#define B_CLUSTER	0x40000000	/* pagein op, so swap() can count it */
#define B_BOUNCE	0x80000000	/* bounce buffer flag */

/*
 * This structure describes a clustered I/O.  It is stored in the b_saveaddr
 * field of the buffer on which I/O is done.  At I/O completion, cluster
 * callback uses the structure to parcel I/O's to individual buffers, and
 * then free's this structure.
 */
struct cluster_save {
	long	bs_bcount;		/* Saved b_bcount. */
	long	bs_bufsize;		/* Saved b_bufsize. */
	void	*bs_saveaddr;		/* Saved b_addr. */
	int	bs_nchildren;		/* Number of associated buffers. */
	struct buf **bs_children;	/* List of associated buffers. */
};

/*
 * number of buffer hash entries
 */
#define BUFHSZ 512

/*
 * buffer hash table calculation, originally by David Greenman
 */
#define BUFHASH(vnp, bn)        \
	(&bufhashtbl[(((unsigned long)(vnp) >> 7)+(int)(bn)) % BUFHSZ])

/*
 * Definitions for the buffer free lists.
 */
#define BUFFER_QUEUES	6	/* number of free buffer queues */

LIST_HEAD(bufhashhdr, buf) bufhashtbl[BUFHSZ], invalhash;
TAILQ_HEAD(bqueues, buf) bufqueues[BUFFER_QUEUES];

#define QUEUE_NONE	0	/* on no queue */
#define QUEUE_LOCKED	1	/* locked buffers */
#define QUEUE_LRU	2	/* useful buffers */
#define QUEUE_VMIO	3	/* VMIO buffers */
#define QUEUE_AGE	4	/* not-useful buffers */
#define QUEUE_EMPTY	5	/* empty buffer headers*/

/*
 * Zero out the buffer's data area.
 */
#define	clrbuf(bp) {							\
	blkclr((bp)->b_data, (u_int)(bp)->b_bcount);			\
	(bp)->b_resid = 0;						\
}

/* Flags to low-level allocation routines. */
#define B_CLRBUF	0x01	/* Request allocated buffer be cleared. */
#define B_SYNC		0x02	/* Do all allocations synchronously. */

#ifdef KERNEL
extern int	nbuf;			/* The number of buffer headers */
extern struct	buf *buf;		/* The buffer headers. */
extern char	*buffers;		/* The buffer contents. */
extern int	bufpages;		/* Number of memory pages in the buffer pool. */
extern struct	buf *swbuf;		/* Swap I/O buffer headers. */
extern int	nswbuf;			/* Number of swap I/O buffer headers. */
extern TAILQ_HEAD(swqueue, buf) bswlist;

__BEGIN_DECLS
void	bufinit __P((void));
void	bremfree __P((struct buf *));
int	bread __P((struct vnode *, daddr_t, int,
	    struct ucred *, struct buf **));
int	breadn __P((struct vnode *, daddr_t, int, daddr_t *, int *, int,
	    struct ucred *, struct buf **));
int	bwrite __P((struct buf *));
void	bdwrite __P((struct buf *));
void	bawrite __P((struct buf *));
void	brelse __P((struct buf *));
void	vfs_bio_awrite __P((struct buf *));
struct buf *     getpbuf __P((void));
struct buf *incore __P((struct vnode *, daddr_t));
int	inmem __P((struct vnode *, daddr_t));
struct buf *getblk __P((struct vnode *, daddr_t, int, int, int));
struct buf *geteblk __P((int));
int allocbuf __P((struct buf *, int));
int	biowait __P((struct buf *));
void	biodone __P((struct buf *));

void	cluster_callback __P((struct buf *));
int	cluster_read __P((struct vnode *, u_quad_t, daddr_t, long,
	    struct ucred *, struct buf **));
void	cluster_wbuild __P((struct vnode *, struct buf *, long, daddr_t, int,
	    daddr_t));
void	cluster_write __P((struct buf *, u_quad_t));
int	physio __P((void (*)(), struct buf *, dev_t, int, u_int (*)(),
	    struct uio *));
u_int	minphys __P((struct buf *));
void	vfs_bio_clrbuf __P((struct buf *));
void	vfs_busy_pages __P((struct buf *, int clear_modify));
void	vfs_unbusy_pages(struct buf *);
void	vwakeup __P((struct buf *));
void	vmapbuf __P((struct buf *));
void	vunmapbuf __P((struct buf *));
void	relpbuf __P((struct buf *));
void	brelvp __P((struct buf *));
void	bgetvp __P((struct vnode *, struct buf *));
void	pbgetvp __P((struct vnode *, struct buf *));
void	pbrelvp __P((struct buf *));
void	reassignbuf __P((struct buf *, struct vnode *));
struct	buf *trypbuf __P((void));
void	vm_bounce_alloc __P((struct buf *));
void	vm_bounce_free __P((struct buf *));
vm_offset_t	vm_bounce_kva_alloc __P((int));
void	vm_bounce_kva_alloc_free __P((vm_offset_t, int));
__END_DECLS
#endif
#endif /* !_SYS_BUF_H_ */
