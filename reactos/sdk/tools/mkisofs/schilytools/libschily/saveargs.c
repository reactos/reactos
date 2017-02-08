/* @(#)saveargs.c	1.16 10/11/18 Copyright 1995-2010 J. Schilling */
/*
 *	save argc, argv for command error printing routines
 *
 *	Copyright (c) 1995-2010 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/avoffset.h>
#include <schily/schily.h>
#include <schily/dlfcn.h>

#if	!defined(AV_OFFSET) || !defined(FP_INDIR)
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif

#ifdef	HAVE_VAR___PROGNAME
extern	char	*__progname;
#ifdef	HAVE_VAR___PROGNAME_FULL
extern	char	*__progname_full;
#else
#define	__progname_full	__progname
#endif
#endif

static	int	ac_saved;
static	char	**av_saved;
static	char	*av0_saved;
static	char	*av0_name_saved;
static	char	*progpath_saved;
static	char	*progname_saved;

static	char	av0_sp[32];	/* av0 space, avoid malloc() in most cases */
static	char	prn_sp[32];	/* name space, avoid malloc() in most cases */
static	char	dfl_str[] = "?";

LOCAL	void	save_av0	__PR((char *av0));
LOCAL	void	init_progname	__PR((void));
LOCAL	void	init_arginfo	__PR((void));

EXPORT void
save_args(ac, av)
	int	ac;
	char	*av[];
{
	ac_saved = ac;
	av_saved = av;
	save_av0(av[0]);
}

LOCAL void
save_av0(av0)
	char	*av0;
{
	int	slen;
	char	*p;

	if (av0_saved && av0_saved != av0_sp)
		free(av0_saved);

	slen = strlen(av0) + 1;

	if (slen <= (int)sizeof (av0_sp))
		av0_saved = av0_sp;
	else
		av0_saved = malloc(slen);

	if (av0_saved) {
		strcpy(av0_saved, av0);
		av0 = av0_saved;
	}

	if ((p = strrchr(av0, '/')) == NULL)
		av0_name_saved = av0;
	else
		av0_name_saved = ++p;
}

EXPORT int
saved_ac()
{
	if (av_saved == NULL)
		init_arginfo();

	return (ac_saved);
}

EXPORT char **
saved_av()
{
	if (av_saved == NULL)
		init_arginfo();

	return (av_saved);
}

EXPORT char *
saved_av0()
{
	if (av0_saved == NULL)
		init_arginfo();

	return (av0_saved);
}

EXPORT void
set_progname(name)
	const char	*name;
{
	int	slen;
	char	*p;

	if (progpath_saved && progpath_saved != prn_sp)
		free(progpath_saved);

	slen = strlen(name) + 1;

	if (slen <= sizeof (prn_sp))
		progpath_saved = prn_sp;
	else
		progpath_saved = malloc(slen);

	if (progpath_saved) {
		strcpy(progpath_saved, name);
		name = progpath_saved;
	}

	if ((p = strrchr(name, '/')) == NULL)
		progname_saved = (char *)name;
	else
		progname_saved = ++p;
}

EXPORT char *
get_progname()
{
	if (progname_saved)
		return (progname_saved);
	if (av0_name_saved == NULL)
		init_progname();
	if (av0_name_saved)
		return (av0_name_saved);
	return (dfl_str);
}

EXPORT char *
get_progpath()
{
	if (progpath_saved)
		return (progpath_saved);
	if (av0_saved == NULL)
		init_progname();
	if (av0_saved)
		return (av0_saved);
	return (dfl_str);
}

LOCAL void
init_progname()
{
#if defined(HAVE_SCANSTACK) || defined(HAVE_GETPROGNAME)
	char	*progname;
#endif

	if (av0_name_saved == NULL)
		init_arginfo();
	if (av0_name_saved)
		return;
#ifdef	HAVE_GETPROGNAME
	progname = (char *)getprogname();
	if (progname) {
		save_av0(progname);
		return;
	}
#endif
#ifdef	HAVE_VAR___PROGNAME
	if (__progname_full) {
		save_av0(__progname_full);
		return;
	}
#endif
#ifdef	HAVE_SCANSTACK
	progname = getav0();		/* scan stack to get argv[0] */
	if (progname) {
		save_av0(progname);
		return;
	}
#endif
}

LOCAL void
init_arginfo()
{
#if defined(HAVE_DLINFO) && defined(HAVE_DLOPEN_IN_LIBC) && defined(RTLD_DI_ARGSINFO)
	Dl_argsinfo	args;

	if (dlinfo(RTLD_SELF, RTLD_DI_ARGSINFO, &args) < 0 ||
	    args.dla_argc <= 0 ||
	    args.dla_argv[0] == NULL)
		return;

	if (av_saved == NULL)
		save_args(args.dla_argc, args.dla_argv);
#endif
}
