/*
 * Copyright (c) 1996-1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*-
 * Copyright (c) 1986, 1989, 1991, 1993
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
 *	@(#)proc.h	8.8 (Berkeley) 1/21/94
 * $\Id: proc.h,v 1.17.4.1 1996/06/04 02:45:01 davidg Exp $
 */

#ifndef _SYS_PROC_H_
#define	_SYS_PROC_H_

#include <machine/proc.h>		/* Machine-dependent proc substruct. */
#include <sys/rtprio.h>			/* For struct rtprio. */
#include <sys/select.h>			/* For struct selinfo. */
#include <sys/time.h>			/* For structs itimerval, timeval. */
#include <sys/socketvar.h>

#ifdef OSKIT
#include <oskit/dev/dev.h>
#endif

/*
 * One structure allocated per session.
 */
struct	session {
	int	s_count;		/* Ref cnt; pgrps in session. */
	struct	proc *s_leader;		/* Session leader. */
	struct	vnode *s_ttyvp;		/* Vnode of controlling terminal. */
	struct	tty *s_ttyp;		/* Controlling terminal. */
	char	s_login[MAXLOGNAME];	/* Setlogin() name. */
};

/*
 * One structure allocated per process group.
 */
struct	pgrp {
	struct	pgrp *pg_hforw;		/* Forward link in hash bucket. */
	struct	proc *pg_mem;		/* Pointer to pgrp members. */
	struct	session *pg_session;	/* Pointer to session. */
	pid_t	pg_id;			/* Pgrp id. */
	int	pg_jobc;	/* # procs qualifying pgrp for job control */
};

/*
 * Description of a process.
 *
 * This structure contains the information needed to manage a thread of
 * control, known in UN*X as a process; it has references to substructures
 * containing descriptions of things that the process uses, but may share
 * with related processes.  The process structure and the substructures
 * are always addressible except for those marked "(PROC ONLY)" below,
 * which might be addressible only on a processor on which the process
 * is running.
 */
struct	proc {
#ifndef OSKIT
	struct	proc *p_forw;		/* Doubly-linked run/sleep queue. */
	struct	proc *p_back;
	struct	proc *p_next;		/* Linked list of active procs */
	struct	proc **p_prev;		/*    and zombies. */

	/* substructures: */
	struct	pcred *p_cred;		/* Process owner's identity. */
	struct	filedesc *p_fd;		/* Ptr to open files structure. */
	struct	pstats *p_stats;	/* Accounting/statistics (PROC ONLY). */
	struct	plimit *p_limit;	/* Process limits. */
	struct	vmspace *p_vmspace;	/* Address space. */
	struct	sigacts *p_sigacts;	/* Signal actions, state (PROC ONLY). */

#define	p_ucred		p_cred->pc_ucred
#define	p_rlimit	p_limit->pl_rlimit

	int	p_flag;			/* P_* flags. */
	char	p_stat;			/* S* process status. */

	char	p_pad1[3];

	pid_t	p_pid;			/* Process identifier. */
	struct	proc *p_hash;	 /* Hashed based on p_pid for kill+exit+... */
	struct	proc *p_pgrpnxt; /* Pointer to next process in process group. */
	struct	proc *p_pptr;	 /* Pointer to process structure of parent. */
	struct	proc *p_osptr;	 /* Pointer to older sibling processes. */

/* The following fields are all zeroed upon creation in fork. */
#define	p_startzero	p_ysptr
	struct	proc *p_ysptr;	 /* Pointer to younger siblings. */
	struct	proc *p_cptr;	 /* Pointer to youngest living child. */
	pid_t	p_oppid;	 /* Save parent pid during ptrace. XXX */
	int	p_dupfd;	 /* Sideways return value from fdopen. XXX */

	/* scheduling */
	u_int	p_estcpu;	 /* Time averaged value of p_cpticks. */
	int	p_cpticks;	 /* Ticks of cpu time. */
	fixpt_t	p_pctcpu;	 /* %cpu for this process during p_swtime */
	void	*p_wchan;	 /* Sleep address. */
	char	*p_wmesg;	 /* Reason for sleep. */
	u_int	p_swtime;	 /* Time swapped in or out. */
	u_int	p_slptime;	 /* Time since last blocked. */

	struct	itimerval p_realtimer;	/* Alarm timer. */
	struct	timeval p_rtime;	/* Real time. */
	u_quad_t p_uticks;		/* Statclock hits in user mode. */
	u_quad_t p_sticks;		/* Statclock hits in system mode. */
	u_quad_t p_iticks;		/* Statclock hits processing intr. */

	int	p_traceflag;		/* Kernel trace points. */
	struct	vnode *p_tracep;	/* Trace to vnode. */

	int	p_siglist;		/* Signals arrived but not delivered. */

	struct	vnode *p_textvp;	/* Vnode of executable. */

	char	p_lock;			/* Process lock count. */
	char	p_pad2[3];		/* alignment */
	long	p_spare[2];		/* Pad to 256, avoid shifting eproc. XXX */

/* End area that is zeroed on creation. */
#define	p_endzero	p_startcopy

/* The following fields are all copied upon creation in fork. */
#define	p_startcopy	p_sigmask

	sigset_t p_sigmask;	/* Current signal mask. */
	sigset_t p_sigignore;	/* Signals being ignored. */
	sigset_t p_sigcatch;	/* Signals being caught by user. */

	u_char	p_priority;	/* Process priority. */
	u_char	p_usrpri;	/* User-priority based on p_cpu and p_nice. */
	char	p_nice;		/* Process "nice" value. */
	char	p_comm[MAXCOMLEN+1];

	struct 	pgrp *p_pgrp;	/* Pointer to process group. */

	struct 	sysentvec *p_sysent; /* System call dispatch information. */

	struct	rtprio p_rtprio;	/* Realtime priority. */
/* End area that is copied on creation. */
#define	p_endcopy	p_thread
	int	p_thread;	/* Id for this "thread"; Mach glue. XXX */
	struct	user *p_addr;	/* Kernel virtual addr of u-area (PROC ONLY). */
	struct	mdproc p_md;	/* Any machine-dependent fields. */

	u_short	p_xstat;	/* Exit status for wait; also stop signal. */
	u_short	p_acflag;	/* Accounting flags. */
	struct	rusage *p_ru;	/* Exit information. XXX */
#else /* OSKIT */
	/*
	 * let's build our own rudimentary struct proc
	 */
	struct	proc *p_forw;		/* Doubly-linked run/sleep queue. */
	struct	pcred *p_cred;		/* Process owner's identity. */
	struct	pstats *p_stats;	/* Accounting/statistics (PROC ONLY). */
#define	p_ucred		p_cred->pc_ucred
	int	p_flag;			/* P_* flags. */
	char	p_stat;			/* S* process status. */
	pid_t	p_pid;			/* Process identifier. */
	void	*p_wchan;	 /* Sleep address. */
	char	*p_wmesg;	 /* Reason for sleep. */
	u_short	p_acflag;	/* Accounting flags. */

	osenv_sleeprec_t		p_sr;
	/*
	 * When selrecord is invoked, this points to an object used to
	 * manage a set of listeners 
	 */
	struct 	listener_mgr	*p_sel;
#endif /* !OSKIT */
};

#define	p_session	p_pgrp->pg_session
#define	p_pgid		p_pgrp->pg_id

/* Status values. */
#define	SIDL	1		/* Process being created by fork. */
#define	SRUN	2		/* Currently runnable. */
#define	SSLEEP	3		/* Sleeping on an address. */
#define	SSTOP	4		/* Process debugging or suspension. */
#define	SZOMB	5		/* Awaiting collection by parent. */

/* These flags are kept in p_flags. */
#define	P_ADVLOCK	0x00001	/* Process may hold a POSIX advisory lock. */
#define	P_CONTROLT	0x00002	/* Has a controlling terminal. */
#define	P_INMEM		0x00004	/* Loaded into memory. */
#define	P_NOCLDSTOP	0x00008	/* No SIGCHLD when children stop. */
#define	P_PPWAIT	0x00010	/* Parent is waiting for child to exec/exit. */
#define	P_PROFIL	0x00020	/* Has started profiling. */
#define	P_SELECT	0x00040	/* Selecting; wakeup/waiting danger. */
#define	P_SINTR		0x00080	/* Sleep is interruptible. */
#define	P_SUGID		0x00100	/* Had set id privileges since last exec. */
#define	P_SYSTEM	0x00200	/* System proc: no sigs, stats or swapping. */
#define	P_TIMEOUT	0x00400	/* Timing out during sleep. */
#define	P_TRACED	0x00800	/* Debugged process being traced. */
#define	P_WAITED	0x01000	/* Debugging process has waited for child. */
#define	P_WEXIT		0x02000	/* Working on exiting. */
#define P_EXEC		0x04000	/* Process called exec. */
#define	P_SWAPPING	0x40000	/* Process is being swapped. */

/* Should probably be changed into a hold count (They have. -DG). */
#define	P_NOSWAP	0x08000	/* Another flag to prevent swap out. */
#define	P_PHYSIO	0x10000	/* Doing physical I/O. */

/* Should be moved to machine-dependent areas. */
#define	P_OWEUPC	0x20000	/* Owe process an addupc() call at next ast. */

/*
 * MOVE TO ucred.h?
 *
 * Shareable process credentials (always resident).  This includes a reference
 * to the current user credentials as well as real and saved ids that may be
 * used to change ids.
 */
struct	pcred {
	struct	ucred *pc_ucred;	/* Current credentials. */
	uid_t	p_ruid;			/* Real user id. */
	uid_t	p_svuid;		/* Saved effective user id. */
	gid_t	p_rgid;			/* Real group id. */
	gid_t	p_svgid;		/* Saved effective group id. */
	int	p_refcnt;		/* Number of references. */
};

#ifdef KERNEL
/*
 * We use process IDs <= PID_MAX; PID_MAX + 1 must also fit in a pid_t,
 * as it is used to represent "no process group".
 */
#define	PID_MAX		30000
#define	NO_PID		30001
#define	PIDHASH(pid)	((pid) & pidhashmask)

#define SESS_LEADER(p)	((p)->p_session->s_leader == (p))
#define	SESSHOLD(s)	((s)->s_count++)
#define	SESSRELE(s) {							\
	if (--(s)->s_count == 0)					\
		FREE(s, M_SESSION);					\
}

extern struct proc *pidhash[];		/* In param.c. */
extern struct pgrp *pgrphash[];		/* In param.c. */
extern struct proc *curproc;		/* Current running proc. */
extern struct proc proc0;		/* Process slot for swapper. */
extern int nprocs, maxproc;		/* Current and max number of procs. */
extern int maxprocperuid;		/* Max procs per uid. */
extern int pidhashmask;			/* In param.c. */

extern volatile struct proc *allproc;	/* List of active procs. */
extern struct proc *zombproc;		/* List of zombie procs. */
extern struct proc *initproc, *pageproc; /* Process slots for init, pager. */

#define	NQS	32			/* 32 run queues. */
extern struct prochd qs[];
extern struct prochd rtqs[];
extern struct prochd idqs[];
extern int	whichqs;	/* Bit mask summary of non-empty Q's. */
struct	prochd {
	struct	proc *ph_link;		/* Linked list of running processes. */
	struct	proc *ph_rlink;
};

int	chgproccnt __P((uid_t, int));
struct proc *pfind __P((pid_t));	/* Find process by id. */
struct proc *zpfind __P((pid_t));	/* Find zombie process by id. */
struct pgrp *pgfind __P((pid_t));	/* Find process group by id. */
void	mi_switch __P((void));
void	resetpriority __P((struct proc *));
void	roundrobin __P((void *));
void	schedcpu __P((void *));
void	setrunnable __P((struct proc *));
void	setrunqueue __P((struct proc *));
void	remrq __P((struct proc *));
void	cpu_switch __P((struct proc *));
void	sleep __P((void *chan, int pri));
int	tsleep __P((void *chan, int pri, char *wmesg, int timo));
void	unsleep __P((struct proc *));
void	wakeup __P((struct socket *so, struct selinfo *si, void *chan));

__dead void cpu_exit __P((struct proc *)) __dead2;
__dead void exit1 __P((struct proc *, int)) __dead2;
void	fixjobc __P((struct proc *, struct pgrp *, int));
int	acct_process __P((struct proc *));
int	leavepgrp __P((struct proc *));
int	enterpgrp __P((struct proc *, pid_t, int));
int	trace_req __P((struct proc *));
void	cpu_wait __P((struct proc *));
int	cpu_coredump __P((struct proc *, struct vnode *, struct ucred *));
int	inferior __P((struct proc *));
#endif	/* KERNEL */

#endif	/* !_SYS_PROC_H_ */
