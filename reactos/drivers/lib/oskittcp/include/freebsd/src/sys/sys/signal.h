/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 *	@(#)signal.h	8.2 (Berkeley) 1/21/94
 */

#ifndef	_SYS_SIGNAL_H_
#define	_SYS_SIGNAL_H_

#define NSIG	32		/* counting 0; could be 33 (mask is 1-32) */

#ifndef _ANSI_SOURCE
#include <machine/signal.h>	/* sigcontext; codes for SIGILL, SIGFPE */
#endif

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#ifndef _POSIX_SOURCE
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#endif
#define	SIGABRT	6	/* abort() */
#ifndef _POSIX_SOURCE
#define	SIGIOT	SIGABRT	/* compatibility */
#define	SIGEMT	7	/* EMT instruction */
#endif
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#ifndef _POSIX_SOURCE
#define	SIGBUS	10	/* bus error */
#endif
#define	SIGSEGV	11	/* segmentation violation */
#ifndef _POSIX_SOURCE
#define	SIGSYS	12	/* bad argument to system call */
#endif
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#ifndef _POSIX_SOURCE
#define	SIGURG	16	/* urgent condition on IO channel */
#endif
#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#ifndef _POSIX_SOURCE
#define	SIGIO	23	/* input/output possible signal */
#define	SIGXCPU	24	/* exceeded CPU time limit */
#define	SIGXFSZ	25	/* exceeded file size limit */
#define	SIGVTALRM 26	/* virtual time alarm */
#define	SIGPROF	27	/* profiling time alarm */
#define SIGWINCH 28	/* window size changes */
#define SIGINFO	29	/* information request */
#endif
#define SIGUSR1 30	/* user defined signal 1 */
#define SIGUSR2 31	/* user defined signal 2 */

#if defined(_ANSI_SOURCE) || defined(__cplusplus)
/*
 * Language spec sez we must list exactly one parameter, even though we
 * actually supply three.  Ugh!
 */
#define	SIG_DFL		(void (*)(int))0
#define	SIG_IGN		(void (*)(int))1
#define	SIG_ERR		(void (*)(int))-1
#else
#define	SIG_DFL		(void (*)())0
#define	SIG_IGN		(void (*)())1
#define	SIG_ERR		(void (*)())-1
#endif

#ifndef _ANSI_SOURCE
typedef unsigned int sigset_t;

/*
 * Signal vector "template" used in sigaction call.
 */
struct	sigaction {
	void	(*sa_handler)();	/* signal handler */
	sigset_t sa_mask;		/* signal mask to apply */
	int	sa_flags;		/* see signal options below */
};
#ifndef _POSIX_SOURCE
#define SA_ONSTACK	0x0001	/* take signal on signal stack */
#define SA_RESTART	0x0002	/* restart system on signal return */
#define	SA_DISABLE	0x0004	/* disable taking signals on alternate stack */
#ifdef COMPAT_SUNOS
#define	SA_USERTRAMP	0x0100	/* do not bounce off kernel's sigtramp */
#endif
#endif
#define SA_NOCLDSTOP	0x0008	/* do not generate SIGCHLD on child stop */

/*
 * Flags for sigprocmask:
 */
#define	SIG_BLOCK	1	/* block specified signal set */
#define	SIG_UNBLOCK	2	/* unblock specified signal set */
#define	SIG_SETMASK	3	/* set specified signal set */

#ifndef _POSIX_SOURCE
#ifndef KERNEL
#include <sys/cdefs.h>
#endif
typedef	void (*sig_t) __P((int));	/* type of signal function */

/*
 * Structure used in sigaltstack call.
 */
struct	sigaltstack {
	char	*ss_sp;			/* signal stack base */
	int	ss_size;		/* signal stack length */
	int	ss_flags;		/* SA_DISABLE and/or SA_ONSTACK */
};
#define	MINSIGSTKSZ	8192			/* minimum allowable stack */
#define	SIGSTKSZ	(MINSIGSTKSZ + 32768)	/* recommended stack size */

/*
 * 4.3 compatibility:
 * Signal vector "template" used in sigvec call.
 */
struct	sigvec {
	void	(*sv_handler)();	/* signal handler */
	int	sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};

#define SV_ONSTACK	SA_ONSTACK
#define SV_INTERRUPT	SA_RESTART	/* same bit, opposite sense */
#define sv_onstack sv_flags	/* isn't compatibility wonderful! */

/*
 * Structure used in sigstack call.
 */
struct	sigstack {
	char	*ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

/*
 * Macro for converting signal number to a mask suitable for
 * sigblock().
 */
#define sigmask(m)	(1 << ((m)-1))

#define	BADSIG		SIG_ERR

#endif	/* !_POSIX_SOURCE */
#endif	/* !_ANSI_SOURCE */

/*
 * For historical reasons; programs expect signal's return value to be
 * defined by <sys/signal.h>.
 */
__BEGIN_DECLS
void	(*signal __P((int, void (*) __P((int))))) __P((int));
__END_DECLS
#endif	/* !_SYS_SIGNAL_H_ */
