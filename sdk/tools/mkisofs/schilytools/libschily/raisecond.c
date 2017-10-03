/* @(#)raisecond.c	1.22 09/07/10 Copyright 1985, 1989, 1995-2004 J. Schilling */
/*
 *	raise a condition (software signal)
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */
/*
 *	Check for installed condition handlers.
 *	If a handler is found, the function is called with the appropriate args.
 *	If no handler is found or no handler signals success,
 *	the program will be aborted.
 *
 *	Copyright (c) 1985, 1989, 1995-2004 J. Schilling
 */
#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/sigblk.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/avoffset.h>
#include <schily/schily.h>

#if	!defined(AV_OFFSET) || !defined(FP_INDIR)
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif

/*
 * Macros to print to stderr without stdio, to avoid screwing up.
 */
#ifndef	STDERR_FILENO
#define	STDERR_FILENO	2
#endif
#define	eprints(a)	(void)write(STDERR_FILENO, (a), sizeof (a)-1)
#define	eprintl(a)	(void)write(STDERR_FILENO, (a), strlen(a))

#define	is_even(p)	((((long)(p)) & 1) == 0)
#define	even(p)		(((long)(p)) & ~1L)
#ifdef	__future__
#define	even(p)		(((long)(p)) - 1) /* will this work with 64 bit ?? */
#endif


LOCAL	void raiseabort  __PR((const char *));

#ifdef	HAVE_SCANSTACK
#include <schily/stkframe.h>
#define	next_frame(vp)	do {						    \
				if (((struct frame *)(vp))->fr_savfp == 0) { \
					vp = (void *)0;			    \
					break;				    \
				}					    \
				if (((struct frame *)(vp))->fr_savpc == 0) { \
					vp = (void *)0;			    \
					break;				    \
				}					    \
				vp =					    \
				    (void *)((struct frame *)(vp))->fr_savfp; \
			} while (vp != NULL && is_even(vp));		    \
			vp = (struct frame *)even(vp);
#else
#if	defined(IS_MACOS_X)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * Make __roothandle a "data" variable.
 */
EXPORT	SIGBLK	*__roothandle = 0;
#else
EXPORT	SIGBLK	*__roothandle;
#endif

#define	next_frame(vp)	vp = (((SIGBLK *)(vp))->sb_savfp);
#endif

LOCAL	BOOL framehandle __PR((SIGBLK *, const char *, const char *, long));

/*
 *	Loop through the chain of procedure frames on the stack.
 *
 *	Frame pointers normally have even values.
 *	Frame pointers of procedures with an installed handler are marked odd.
 *	The even base value, in this case actually points to a SIGBLK which
 *	holds the saved "real" frame pointer.
 *	The SIGBLK mentioned above may me the start of a chain of SIGBLK's,
 *	containing different handlers.
 */
EXPORT void
raisecond(signame, arg2)
	const char	*signame;
	long		arg2;
{
	register void	*vp = NULL;

#ifdef	HAVE_SCANSTACK
	/*
	 * As the SCO OpenServer C-Compiler has a bug that may cause
	 * the first function call to getfp() been done before the
	 * new stack frame is created, we call getfp() twice.
	 */
	(void) getfp();
	vp = getfp();
	next_frame(vp);
#else
	vp = __roothandle;
#endif

	while (vp) {
		if (framehandle((SIGBLK *)vp, signame, signame, arg2))
			return;
		else if (framehandle((SIGBLK *)vp, "any_other", signame, arg2))
			return;
#ifdef	HAVE_SCANSTACK
		vp = (struct frame *)((SIGBLK *)vp)->sb_savfp;
#endif
		next_frame(vp);
	}
	/*
	 * No matching handler that signals success found.
	 * Print error message and abort.
	 */
	raiseabort(signame);
	/* NOTREACHED */
}

/*
 *	Loop through the handler chain for a procedure frame.
 *
 *	If no handler with matching name is found, return FALSE,
 *	otherwise the first handler with matching name is called.
 *	The return value in the latter case depends on the called function.
 */
LOCAL BOOL
framehandle(sp, handlename, signame, arg2)
	register SIGBLK *sp;
	const char	*handlename;
	const char	*signame;
	long		arg2;
{
	for (; sp; sp = sp->sb_signext) {
		if (sp->sb_signame != NULL &&
		    streql(sp->sb_signame, handlename)) {
			if (sp->sb_sigfun == NULL) {	/* deactivated */
				return (FALSE);
			} else {
				return (*sp->sb_sigfun)(signame,
							sp->sb_sigarg, arg2);
			}
		}
	}
	return (FALSE);
}

LOCAL void
raiseabort(signame)
	const	char	*signame;
{
	eprints("Condition not caught: "); eprintl(signame); eprints(".\n");
	abort();
	/* NOTREACHED */
}
