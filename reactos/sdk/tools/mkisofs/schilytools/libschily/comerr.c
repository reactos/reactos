/* @(#)comerr.c	1.44 17/03/13 Copyright 1985-1989, 1995-2017 J. Schilling */
/*
 *	Routines for printing command errors
 *
 *	Copyright (c) 1985-1989, 1995-2017 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/mconfig.h>
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdio.h>	/* Try again for size_t	*/
#include <schily/stdlib.h>	/* Try again for size_t	*/
#include <schily/standard.h>
#include <schily/varargs.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/errno.h>

EXPORT	int	on_comerr	__PR((void (*fun)(int, void *), void *arg));
EXPORT	void	comerr		__PR((const char *, ...));
EXPORT	void	xcomerr		__PR((int, const char *, ...));
EXPORT	void	comerrno	__PR((int, const char *, ...));
EXPORT	void	xcomerrno	__PR((int, int, const char *, ...));
EXPORT	int	errmsg		__PR((const char *, ...));
EXPORT	int	errmsgno	__PR((int, const char *, ...));
EXPORT	int	_comerr		__PR((FILE *, int, int, int,
						const char *, va_list));
LOCAL	int	_ex_clash	__PR((int));
EXPORT	void	comexit		__PR((int));
EXPORT	char	*errmsgstr	__PR((int));

typedef	struct ex {
	struct ex *next;
	void	(*func) __PR((int, void *));
	void	*arg;
} ex_t;

LOCAL	ex_t	*exfuncs;

/*
 * Set list of callback functions to call with *comerr() and comexit().
 * The function set up last with on_comerr() is called first on exit;
 * in other words: call order is the reverse order of registration.
 */
EXPORT	int
on_comerr(func, arg)
	void	(*func) __PR((int, void *));
	void	*arg;
{
	ex_t	*fp;

	fp = malloc(sizeof (*fp));
	if (fp == NULL)
		return (-1);

	fp->func = func;
	fp->arg  = arg;
	fp->next = exfuncs;
	exfuncs = fp;
	return (0);
}

/*
 * Fetch current errno, print a related message and exit(errno).
 */
/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
comerr(const char *msg, ...)
#else
EXPORT void
comerr(msg, va_alist)
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void) _comerr(stderr, COMERR_EXIT, 0, geterrno(), msg, args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Fetch current errno, print a related message and exit(exc).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
xcomerr(int exc, const char *msg, ...)
#else
EXPORT void
xcomerr(exc, msg, va_alist)
	int	exc;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void) _comerr(stderr, COMERR_EXCODE, exc, geterrno(), msg, args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Print a message related to err and exit(err).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
comerrno(int err, const char *msg, ...)
#else
EXPORT void
comerrno(err, msg, va_alist)
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void) _comerr(stderr, COMERR_EXIT, 0, err, msg, args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Print a message related to err and exit(exc).
 */
/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT void
xcomerrno(int exc, int err, const char *msg, ...)
#else
EXPORT void
xcomerrno(exc, err, msg, va_alist)
	int	exc;
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void) _comerr(stderr, COMERR_EXCODE, exc, err, msg, args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Fetch current errno, print a related message and return(errno).
 */
/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
errmsg(const char *msg, ...)
#else
EXPORT int
errmsg(msg, va_alist)
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _comerr(stderr, COMERR_RETURN, 0, geterrno(), msg, args);
	va_end(args);
	return (ret);
}

/*
 * Print a message related to err and return(err).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
errmsgno(int err, const char *msg, ...)
#else
EXPORT int
errmsgno(err, msg, va_alist)
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _comerr(stderr, COMERR_RETURN, 0, err, msg, args);
	va_end(args);
	return (ret);
}

#if defined(__BEOS__) || defined(__HAIKU__)
	/*
	 * On BeOS errno is a big negative number (0x80000000 + small number).
	 * We assume that small negative numbers are safe to be used as special
	 * values that prevent printing the errno text.
	 *
	 * We tried to use #if EIO < 0 but this does not work because EIO is
	 * defined to a enum. ENODEV may work as ENODEV is defined to a number
	 * directly.
	 */
#define	silent_error(e)		((e) < 0 && (e) >= -1024)
#else
	/*
	 * On UNIX errno is a small non-negative number, so we assume that
	 * negative values cannot be a valid errno and don't print the error
	 * string in this case. However the value may still be used as exit()
	 * code if 'exflg' is set.
	 */
#define	silent_error(e)		((e) < 0)
#endif
EXPORT int
_comerr(f, exflg, exc, err, msg, args)
	FILE		*f;	/* FILE * to print to */
	int		exflg;	/* COMERR_RETURN, COMERR_EXIT, COMERR_EXCODE */
	int		exc;	/* Use for exit() if exflg & COMERR_EXCODE */
	int		err;	/* Errno for text, exit(err) if !COMERR_EXIT*/
	const char	*msg;	/* printf() format */
	va_list		args;	/* printf() args for format */
{
	char	errbuf[20];
	char	*errnam;
	char	*prognam = get_progname();

	if (silent_error(err)) {
		js_fprintf(f, "%s: %r", prognam, msg, args);
	} else {
		errnam = errmsgstr(err);
		if (errnam == NULL) {
			(void) js_snprintf(errbuf, sizeof (errbuf),
						"Error %d", err);
			errnam = errbuf;
		}
		js_fprintf(f, "%s: %s. %r", prognam, errnam, msg, args);
	}
	if (exflg) {
		if (exflg & COMERR_EXCODE)
			err = exc;
		else
			err = _ex_clash(err);
		comexit(err);
		/* NOTREACHED */
	}
	return (err);
}

LOCAL int
_ex_clash(exc)
	int	exc;
{
	int	exmod = exc % 256;

	/*
	 * On a recent POSIX System that supports waitid(), siginfo.si_status
	 * holds the exit(2) value as an int. So if waitid() is used to wait
	 * for the process, we do not have problems from folded exit codes.
	 * All other wait*() functions fold the exit code by masking it
	 * with 0377.
	 *
	 * Exit codes used with comerr*()/comexit() are frequently errno values
	 * that have been in the range 0..31 with UNIX.V5 in the mid 1970s and
	 * that now are in the range 0..151 on Solaris. These values do not
	 * cause problems from folding to 8 bits, but "sysexits.h" contains
	 * definitions in the range 64..79 that cause (even unfolded) clashes
	 * with errno values.
	 *
	 * To avoid clashes with errno values, "schily/standard.h" defines
	 * EX_BAD (-1) as default error exit code and
	 * EX_CLASH (-64) as marker for clashes.
	 * Exit codes in the range -2..-63 (254..193 seen as unsigned two's
	 * complement) are available as software specific exit codes.
	 * We map all other negative exit codes to EX_CLASH if they would fold
	 * to -2..-63.
	 */
	if (exc != exmod && exmod <= 0 && exmod >= EX_CLASH)
		exc = EX_CLASH;
	return (exc);
}

/*
 * Do a program exit() with previously calling functions registered via
 * on_comerr().
 */
EXPORT void
comexit(err)
	int	err;
{
	while (exfuncs) {
		ex_t	*fp;

		(*exfuncs->func)(err, exfuncs->arg);
		fp = exfuncs;
		exfuncs = exfuncs->next;
		free(fp);
	}
	exit(err);
	/* NOTREACHED */
}

/*
 * Wrapper around the strange POSIX strerror().
 * If there is a problem with retrieving the correct error text,
 * return NULL.
 */
EXPORT char *
errmsgstr(err)
	int	err;
{
#ifdef	HAVE_STRERROR
	/*
	 * POSIX compliance may look strange...
	 */
	int	errsav = geterrno();
	char	*ret;

	seterrno(0);
	ret = strerror(err);
	err = geterrno();
	seterrno(errsav);

	if (ret == NULL || err)
		return (NULL);
	return (ret);
#else
	if (err < 0 || err >= sys_nerr) {
		return (NULL);
	} else {
		return (sys_errlist[err]);
	}
#endif
}
