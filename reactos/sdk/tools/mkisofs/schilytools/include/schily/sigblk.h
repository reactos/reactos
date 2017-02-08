/* @(#)sigblk.h	1.12 08/12/24 Copyright 1985, 1995-2008 J. Schilling */
/*
 *	software signal block definition
 *
 *	Copyright (c) 1985, 1995-2008 J. Schilling
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

#ifndef	_SCHILY_SIGBLK_H
#define	_SCHILY_SIGBLK_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct sigblk {
	long		**sb_savfp;	/* Real saved framepointer	*/
	struct sigblk	*sb_signext;	/* Next sw signal blk for this func */
	short		sb_siglen;	/* Strlen for sb_signame	*/
	const char	*sb_signame;	/* Name of software signal	*/

					/* sb_sigfun: function to call	*/
	int		(*sb_sigfun)	__PR((const char *, long, long));

	long		sb_sigarg;	/* Second arg for sb_sigfun	*/
} SIGBLK;

/*
 * The handler function is called with three arguments:
 *
 *	1)	The name of the software signal
 *	2)	The argument from the handlecond() call
 *	3)	The argument from the raisecond() call
 */
typedef	int	(*handlefunc_t)		__PR((const char *, long, long));

extern	void	handlecond		__PR((const char *, SIGBLK *,
					    int(*)(const char *, long, long),
									long));
extern	void	raisecond		__PR((const char *, long));
extern	void	starthandlecond		__PR((SIGBLK *));
extern	void	unhandlecond		__PR((SIGBLK *));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_SIGBLK_H */
