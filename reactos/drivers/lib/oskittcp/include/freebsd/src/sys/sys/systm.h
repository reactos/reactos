/*-
 * Copyright (c) 1982, 1988, 1991, 1993
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
 *	@(#)systm.h	8.4 (Berkeley) 2/23/94
 */

#ifndef _SYS_SYSTM_H_
#define	_SYS_SYSTM_H_

#include <machine/cpufunc.h>
#include <machine/stdarg.h>

/*
 * The `securelevel' variable controls the security level of the system.
 * It can only be decreased by process 1 (/sbin/init).
 *
 * Security levels are as follows:
 *   -1	permannently insecure mode - always run system in level 0 mode.
 *    0	insecure mode - immutable and append-only flags make be turned off.
 *	All devices may be read or written subject to permission modes.
 *    1	secure mode - immutable and append-only flags may not be changed;
 *	raw disks of mounted filesystems, /dev/mem, and /dev/kmem are
 *	read-only.
 *    2	highly secure mode - same as (1) plus raw disks are always
 *	read-only whether mounted or not. This level precludes tampering
 *	with filesystems by unmounting them, but also inhibits running
 *	newfs while the system is secured.
 *
 * In normal operation, the system runs in level 0 mode while single user
 * and in level 1 mode while multiuser. If level 2 mode is desired while
 * running multiuser, it can be set in the multiuser startup script
 * (/etc/rc.local) using sysctl(1). If it is desired to run the system
 * in level 0 mode while multiuser, initialize the variable securelevel
 * in /sys/kern/kern_sysctl.c to -1. Note that it is NOT initialized to
 * zero as that would allow the kernel binary to be patched to -1.
 * Without initialization, securelevel loads in the BSS area which only
 * comes into existence when the kernel is loaded and hence cannot be
 * patched by a stalking hacker.
 */
extern int securelevel;		/* system security level */

extern int cold;		/* nonzero if we are doing a cold boot */
extern const char *panicstr;	/* panic message */
extern char version[];		/* system version */
extern char copyright[];	/* system copyright */

extern int nblkdev;		/* number of entries in bdevsw */
extern int nchrdev;		/* number of entries in cdevsw */
extern struct swdevt *swdevt;	/* swap-device information */
extern int nswdev;		/* number of swap devices */
extern int nswap;		/* size of swap space */

extern int selwait;		/* select timeout address */

extern u_char curpriority;	/* priority of current process */

extern int physmem;		/* physical memory */

extern dev_t dumpdev;		/* dump device */
extern long dumplo;		/* offset into dumpdev */

extern dev_t rootdev;		/* root device */
extern struct vnode *rootvp;	/* vnode equivalent to above */

extern dev_t swapdev;		/* swapping device */
extern struct vnode *swapdev_vp;/* vnode equivalent to above */

extern int boothowto;		/* reboot flags, from console subsystem */

/*
 * General function declarations.
 */
int	nullop __P((void));
int	enodev __P((void));
int	enoioctl __P((void));
int	enxio __P((void));
int	eopnotsupp __P((void));
int	seltrue __P((dev_t dev, int which, struct proc *p));
int	ureadc __P((int, struct uio *));
void	*hashinit __P((int count, int type, u_long *hashmask));
void	*phashinit __P((int count, int type, u_long *nentries));

__dead void	panic __P((const char *, ...)) __dead2;
__dead void	boot __P((int)) __dead2;
void	tablefull __P((const char *));
void	addlog __P((const char *, ...));
#ifndef __REACTOS__
void	log __P((int, const char *, ...));
void	printf __P((const char *, ...));
#else
#include <oskitfreebsd.h>
#include <oskitdebug.h>
#define log(x,...) OS_DbgPrint(x,(__VA_ARGS__))
#endif
void	uprintf __P((const char *, ...));
int	sprintf __P((char *buf, const char *, ...));
void	ttyprintf __P((struct tty *, const char *, ...));
void	kprintf __P((const char *fmt, int flags, struct tty *tp, va_list ap));

#ifndef __REACTOS__
void	bcopy __P((const void *from, void *to, size_t len));
#endif
void	ovbcopy __P((const void *from, void *to, size_t len));
void	blkclr __P((void *buf, u_int len));
void	bzero __P((void *buf, size_t len));

#ifndef __REACTOS__
void	*memcpy __P((void *to, const void *from, size_t len));
#endif

int	copystr __P((void *kfaddr, void *kdaddr, u_int len, u_int *done));
int	copyinstr __P((void *udaddr, void *kaddr, u_int len, u_int *done));
int	copyoutstr __P((void *kaddr, void *udaddr, u_int len, u_int *done));
int	copyin __P((void *udaddr, void *kaddr, u_int len));
int	copyout __P((void *kaddr, void *udaddr, u_int len));

int	fubyte __P((void *base));
int	fuibyte __P((void *base));
int	subyte __P((void *base, int byte));
int	suibyte __P((void *base, int byte));
int	fuword __P((void *base));
int	fuiword __P((void *base));
int	suword __P((void *base, int word));
int	susword __P((void *base, int word));
int	suiword __P((void *base, int word));

int	hzto __P((struct timeval *tv));
void	realitexpire __P((void *));

struct clockframe;
void	hardclock __P((struct clockframe *frame));
void	softclock __P((void));
void	statclock __P((struct clockframe *frame));

void	initclocks __P((void));

void	startprofclock __P((struct proc *));
void	stopprofclock __P((struct proc *));
void	setstatclockrate __P((int hzrate));

void	hardupdate __P((long));
#include <sys/libkern.h>

/* Initialize the world */
extern void consinit(void);
extern void kmeminit(void);
extern void cpu_startup(void);
extern void usrinfoinit(void);
extern void rqinit(void);
extern void vfsinit(void);
extern void mbinit(void);
extern void clist_init(void);
extern void ifinit(void);
extern void domaininit(void);
extern void cpu_initclocks(void);
extern void vntblinit(void);
extern void nchinit(void);

/* Finalize the world. */
void	shutdown_nice __P((void));

extern __dead void vm_pageout(void) __dead2; /* pagedaemon, called in proc 2 */
extern __dead void vfs_update(void) __dead2; /* update, called in proc 3 */
extern __dead void scheduler(void) __dead2; /* sched, called in process 0 */

/*
 * Kernel to clock driver interface.
 */
void	inittodr __P((time_t base));
void	resettodr __P((void));
void	startrtclock __P((void));

/* Timeouts */
typedef void (timeout_t)(void *); /* actual timeout function type */
typedef timeout_t *timeout_func_t; /* a pointer to this type */

void timeout(timeout_func_t, void *, int);
void untimeout(timeout_func_t, void *);
void	logwakeup __P((void));

/* Syscalls that are called internally. */
struct close_args {
	int	fd;
};
int	close __P((struct proc *, struct close_args *uap, int *retval));
struct execve_args {
	char	*fname;
	char	**argv;
	char	**envv;
};
int	execve __P((struct proc *, struct execve_args *, int *retval));
struct fork_args {
	int	dummy;
};
int	fork __P((struct proc *, struct fork_args *, int retval[]));
struct sync_args {
	int	dummy;
};
int	sync __P((struct proc *, struct sync_args *, int *retval));
struct wait_args {
	int	pid;
	int	*status;
	int	options;
	struct	rusage *rusage;
#if defined(COMPAT_43) || defined(COMPAT_IBCS2)
	int	compat;		/* pseudo */
#endif
};
int	wait1 __P((struct proc *, struct wait_args *, int retval[]));

#endif /* !_SYS_SYSTM_H_ */
