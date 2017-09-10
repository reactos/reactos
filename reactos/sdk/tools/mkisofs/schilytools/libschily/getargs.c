/* @(#)getargs.c	2.71 17/07/16 Copyright 1985, 1988, 1994-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getargs.c	2.71 17/07/16 Copyright 1985, 1988, 1994-2017 J. Schilling";
#endif
#define	NEW
/*
 *	Copyright (c) 1985, 1988, 1994-2017 J. Schilling
 *
 *	1.3.88	 Start implementation of release 2
 */
/*
 *	Parse arguments on a command line.
 *	Format string type specifier (appearing directly after flag name):
 *		''	BOOL size int set to TRUE (1)
 *		'%'	Extended format, next char determines the type:
 *			'%0' BOOL with size modifier set to FALSE (0)
 *			'%1' BOOL with size modifier set to TRUE(1)
 *		'*'	string
 *		'?'	char
 *		'#'	number
 *		'&'	call function for any type flag
 *		'~'	call function for BOOLEAN flag
 *		'+'	inctype			+++ NEU +++
 *
 *	The format string 'f* ' may be used to disallow -ffoo for f*
 *	The same behavior is implemented for 'f# ', 'f? ' and 'f& '.
 *	The ' ' needs to immediately follow the format type specifier.
 *
 *	The format string 'f*_' may be used to disallow -f foo for f*
 *	The same behavior is implemented for 'f#_', 'f?_' and 'f&_'.
 *	The '_' needs to immediately follow the format type specifier.
 *	This also allows to implement optional arguments to options.
 *	Note: 'f#_' is only implemented for orthogonality, -f will
 *	be converted to an integer value of 0.
 *
 *	The '#', '+' and '%[01]' types may have size modifiers added:
 *		'c'/'C'	char
 *		's'/'S'	short
 *		'i'/'I'	int	(default == no size modifier)
 *		'l'/'L'	long
 *		'll'/'LL' long long
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
/* LINTLIBRARY */
#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/getargs.h>
#include <schily/varargs.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/ctype.h>

/*
 * Various return values
 */
#define	RETMAX		  2	/* Max. value for getargerror()	*/
#define	FLAGDELIM	  2	/* "--" stopped flag processing	*/
#define	NOTAFLAG	  1	/* Not a flag type argument	*/
#define	NOARGS		  0	/* No more args			*/
#define	BADFLAG		(-1)	/* Not a valid flag argument	*/
#define	BADFMT		(-2)	/* Error in format string	*/
#define	NOTAFILE	(-3)	/* Seems to be a flag type arg	*/
#define	RETMIN		(-3)	/* Min. value for getargerror()	*/

LOCAL char	*retnames[] =  {
	"NOTAFILE",
	"BADFMT",
	"BADFLAG",
	"NOARGS",
	"NOTAFLAG",
	"FLAGDELIM",
};
#define	RNAME(a)	(retnames[(a)-RETMIN])

#define	SCANONLY	0	/* Do not try to set argument values	*/
#define	SETARGS		1	/* Set argument values from cmdline	*/
#define	ARGVECTOR	2	/* Use vector instead of list interface	*/
#define	NOEQUAL		4	/* -opt=val not allowed for -opt val	*/

LOCAL	struct ga_props *_getprops __PR((struct ga_props *));

	int	_getargs __PR((int *, char *const **, void *,
							int,
							struct ga_props *,
							va_list));
LOCAL	int	dofile __PR((int *, char *const **, const char **,
							struct ga_props *));
LOCAL	int	doflag __PR((int *, char *const **, const char *,
						void *, int, va_list));
LOCAL	int	dosflags __PR((const char *, void *,
						int *, char *const **,
						int, va_list));
LOCAL	int	checkfmt __PR((const char *));
LOCAL	int	checkeql __PR((const char *));

LOCAL	va_list	va_dummy;

LOCAL	char	fmtspecs[] = "#?*&~+%";

#define	isfmtspec(c)		(strchr(fmtspecs, c) != NULL)

LOCAL	struct ga_props	props_default = { 0, 0, sizeof (struct ga_props) };
LOCAL	struct ga_props	props_posix = { GAF_POSIX_DEFAULT, 0, sizeof (struct ga_props) };

EXPORT int
_getarginit(props, size, flags)
	struct ga_props	*props;
	size_t		size;
	UInt32_t	flags;
{
	if (size > sizeof (struct ga_props))
		return (-1);

	/*
	 * GAF_POSIX may be used as an alias for the flags that currently
	 * define POSIX behavior.
	 */
	if (flags == GAF_POSIX)
		flags = GAF_POSIX_DEFAULT;

	props->ga_flags = flags;
	props->ga_oflags = 0;
	props->ga_size = size;
	return (0);
}

LOCAL struct ga_props *
_getprops(props)
	struct ga_props	*props;
{
	if (props == GA_NO_PROPS)
		props = &props_default;
	else if (props == GA_POSIX_PROPS)
		props = &props_posix;
	props->ga_oflags = 0;
	/*
	 * GAF_POSIX may be used as an alias for the flags that currently
	 * define POSIX behavior.
	 */
	if (props->ga_flags == GAF_POSIX)
		props->ga_flags = GAF_POSIX_DEFAULT;

	return (props);
}

/*
 *	get flags until a non flag type argument is reached (old version)
 */
/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT int
getargs(int *pac, char *const **pav, const char *fmt, ...)
#else
EXPORT int
getargs(pac, pav, fmt, va_alist)
	int	*pac;
	char	**pav[];
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = _getargs(pac, pav, (void *)fmt, SETARGS, GA_NO_PROPS, args);
	va_end(args);
	return (ret);
}


/*
 *	get flags until a non flag type argument is reached (list version)
 */
/* VARARGS4 */
#ifdef	PROTOTYPES
EXPORT int
getlargs(int *pac, char *const **pav, struct ga_props *props, const char *fmt, ...)
#else
EXPORT int
getlargs(pac, pav, props, fmt, va_alist)
	int		*pac;
	char		**pav[];
	struct ga_props	*props;
	char		*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = _getargs(pac, pav, (void *)fmt, SETARGS, props, args);
	va_end(args);
	return (ret);
}


/*
 *	get flags until a non flag type argument is reached (vector version)
 */
EXPORT int
getvargs(pac, pav, props, vfmt)
	int	*pac;
	char	* const *pav[];
	struct ga_props	*props;
	struct ga_flags *vfmt;
{
	return (_getargs(pac, pav, vfmt, SETARGS | ARGVECTOR, props, va_dummy));
}


/*
 *	get all flags on the command line, do not stop on files (old version)
 */
/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT int
getallargs(int *pac, char *const **pav, const char *fmt, ...)
#else
EXPORT int
getallargs(pac, pav, fmt, va_alist)
	int	*pac;
	char	**pav[];
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	for (; ; (*pac)--, (*pav)++) {
		if ((ret = _getargs(pac, pav, (void *)fmt, SETARGS, GA_NO_PROPS, args)) < NOTAFLAG)
			break;
	}
	va_end(args);
	return (ret);
}


/*
 *	get all flags on the command line, do not stop on files (list version)
 */
/* VARARGS4 */
#ifdef	PROTOTYPES
EXPORT int
getlallargs(int *pac, char *const **pav, struct ga_props *props, const char *fmt, ...)
#else
EXPORT int
getlallargs(pac, pav, props, fmt, va_alist)
	int		*pac;
	char		**pav[];
	struct ga_props	*props;
	char		*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	props = _getprops(props);
	for (; ; (*pac)--, (*pav)++) {
		if ((ret = _getargs(pac, pav, (void *)fmt, SETARGS, props, args)) < NOTAFLAG)
			break;
		/*
		 * The default is to parse all options on the command line and
		 * to let "--" only make the next argument a non-option.
		 */
		if (ret == FLAGDELIM &&	(props->ga_flags & GAF_DELIM_DASHDASH))
			break;
	}
	va_end(args);
	return (ret);
}


/*
 *	get all flags on the command line, do not stop on files (vector version)
 */
EXPORT int
getvallargs(pac, pav, props, vfmt)
	int	*pac;
	char	* const *pav[];
	struct ga_props	*props;
	struct ga_flags *vfmt;
{
	int	ret;

	props = _getprops(props);
	for (; ; (*pac)--, (*pav)++) {
		if ((ret = _getargs(pac, pav, vfmt, SETARGS | ARGVECTOR, props, va_dummy)) < NOTAFLAG)
			break;
		/*
		 * The default is to parse all options on the command line and
		 * to let "--" only make the next argument a non-option.
		 */
		if (ret == FLAGDELIM &&	(props->ga_flags & GAF_DELIM_DASHDASH))
			break;
	}
	return (ret);
}


/*
 *	get next non flag type argument (i.e. a file) (old version)
 *	getfiles() is a dry run getargs()
 */
EXPORT int
getfiles(pac, pav, fmt)
	int		*pac;
	char *const	*pav[];
	const char	*fmt;
{
	return (_getargs(pac, pav, (void *)fmt, SCANONLY, GA_NO_PROPS, va_dummy));
}


/*
 *	get next non flag type argument (i.e. a file) (list version)
 *	getlfiles() is a dry run getlargs()
 */
EXPORT int
getlfiles(pac, pav, props, fmt)
	int		*pac;
	char *const	*pav[];
	struct ga_props	*props;
	const char	*fmt;
{
	return (_getargs(pac, pav, (void *)fmt, SCANONLY, props, va_dummy));
}


/*
 *	get next non flag type argument (i.e. a file) (vector version)
 *	getvfiles() is a dry run getvargs()
 */
EXPORT int
getvfiles(pac, pav, props, vfmt)
	int		*pac;
	char *const	*pav[];
	struct ga_props	*props;
	struct ga_flags	*vfmt;
{
	return (_getargs(pac, pav, vfmt, SCANONLY | ARGVECTOR, props, va_dummy));
}


/*
 *	check args until the next non flag type argmument is reached
 *	*pac is decremented, *pav is incremented so that the
 *	non flag type argument is at *pav[0]
 *
 *	return code:
 *		+2 FLAGDELIM	"--" stopped flag processing
 *		+1 NOTAFLAG	not a flag type argument (is a file)
 *		 0 NOARGS	no more args
 *		-1 BADFLAG	a non-matching flag type argument
 *		-2 BADFMT	bad syntax in format string
 */
/* LOCAL int */
EXPORT int
_getargs(pac, pav, vfmt, flags, props, args)
	register int		*pac;
	register char	*const	**pav;
		void		*vfmt;
		int		flags;
		struct ga_props	*props;
		va_list		args;
{
	const	char	*argp;
		int	ret;


	props = _getprops(props);

	if (props->ga_flags & GAF_NO_EQUAL)
		flags |= NOEQUAL;

	for (; *pac > 0; (*pac)--, (*pav)++) {
		argp = **pav;

		ret = dofile(pac, pav, &argp, props);

		if (ret != NOTAFILE)
			return (ret);

		ret = doflag(pac, pav, argp, vfmt, flags, args);

		if (ret != NOTAFLAG)
			return (ret);
	}
	return (NOARGS);
}


/*
 * check if *pargp is a file type argument
 */
LOCAL int
dofile(pac, pav, pargp, props)
	register int		*pac;
	register char *const	**pav;
		const char	**pargp;
		struct ga_props	*props;
{
	register const char	*argp = *pargp;


	if (argp[0] == '-') {
		/*
		 * "-"	is a special non flag type argument
		 *	that usually means take stdin instead of a named file
		 */
		if (argp[1] == '\0')
			return (NOTAFLAG);
		/*
		 * "--" is a prefix to take the next argument
		 *	as non flag type argument
		 * NOTE: POSIX requires "--" to indicate the end of the
		 *	 flags on the command line. Programs that like to be
		 *	 100% POSIX compliant call only get[lv]args() once
		 *	 and then process the list of files from cav[0].
		 */
		if (argp[1] == '-' && argp[2] == '\0') {
			if (--(*pac) > 0) {
				(*pav)++;
				return (FLAGDELIM);
			} else {
				return (NOARGS);
			}
		}
	}

	/*
	 * Now check if it may be flag type argument at all.
	 * Flag type arguments begin with a '-', a '+' or contain a '='
	 * i.e. -flag +flag or flag=
	 * The behavior here may be controlled by props->ga_flags to
	 * allow getargs() to e.g. behave fully POSIX compliant.
	 */
	if (argp[0] != '-') {
		if (argp[0] == '+' && (props->ga_flags & GAF_NO_PLUS) == 0)
			return (NOTAFILE);	/* This is a flag type arg */

		/*
		 * If 'flag=value' is not allowed at all, speed things up
		 * and do not call checkeql() to check for '='.
		 */
		if (props->ga_flags & GAF_NO_EQUAL)
			return (NOTAFLAG);
		if (checkeql(argp) && (props->ga_flags & GAF_NEED_DASH) == 0)
			return (NOTAFILE);	/* This is a flag type arg */
		return (NOTAFLAG);
	}
	return (NOTAFILE);			/* This is a flag type arg */
}


/*
 *	compare argp with the format string
 *	if a match is found store the result a la scanf in one of the
 *	arguments pointed to in the va_list
 *
 *	If (flags & SETARGS) == 0, only check arguments for getfiles()
 *	In case that (flags & SETARGS) == 0 or that (flags & ARGVECTOR) != 0,
 *	va_list may be a dummy argument.
 */
LOCAL int
doflag(pac, pav, argp, vfmt, flags, oargs)
		int		*pac;
		char	*const	**pav;
	register const char	*argp;
		void		*vfmt;
		int		flags;
		va_list		oargs;
{
	register const char	*fmt = (const char *)vfmt;
	struct ga_flags		*flagp = vfmt;
	const char	*fmtp;
	long	val;
	Llong	llval;
	int	singlecharflag	= 0;
	BOOL	isspec;
	BOOL	hasdash		= FALSE;
	BOOL	doubledash	= FALSE;
	BOOL	haseql		= checkeql(argp);
	const char	*sargp;
	const char	*sfmt;
	va_list	args;
	char	*const	*spav	= *pav;
	int		spac	= *pac;
	void		*curarg	= (void *)0;

	sfmt = fmt;
	/*
	 * flags beginning with '-' don't have to include the '-' in
	 * the format string.
	 * flags beginning with '+' have to include it in the format string.
	 */
	if (argp[0] == '-') {
		argp++;
		hasdash = TRUE;
		/*
		 * Implement legacy support for --longopt
		 * If we find a double dash, we do not look for combinations
		 * of boolean single char flags.
		 */
		if (argp[0] == '-') {
			argp++;
			doubledash = TRUE;
			/*
			 * Allow -- only for long options.
			 */
			if (argp[1] == '\0') {
				return (BADFLAG);
			}
		}
	}
	sargp = argp;

	/*
	 * Initialize 'args' to the start of the argument list.
	 * I don't know any portable way to copy an arbitrary
	 * C object so I use a system-specific routine
	 * (probably a macro) from stdarg.h.  (Remember that
	 * if va_list is an array, 'args' will be a pointer
	 * and '&args' won't be what I would need for memcpy.)
	 * It is a system requirement for SVr4 compatibility
	 * to be able to do this assgignement. If your system
	 * defines va_list to be an array but does not define
	 * va_copy() you are lost.
	 * This is needed to make sure, that 'oargs' will not
	 * be clobbered.
	 */
	va_copy(args, oargs);

	if (flags & ARGVECTOR) {
		sfmt = fmt = flagp->ga_format;
		if (fmt == NULL)
			sfmt = fmt = "";
		if (flags & SETARGS)
			curarg = flagp->ga_arg;
	} else if (flags & SETARGS) {
		curarg = va_arg(args, void *);
	}
	/*
	 * check if the first flag in format string is a singlechar flag
	 */
again:
	if (fmt[0] != '\0' &&
	    (fmt[1] == ',' || fmt[1] == '+' ||
	    fmt[1] == '~' || fmt[1] == '%' || fmt[1] == '\0'))
		singlecharflag++;
	/*
	 * check the whole format string for a match
	 */
	for (;;) {
		for (; *fmt; fmt++, argp++) {
			if (*fmt == '\\') {
				/*
				 * Allow "#?*&+" to appear inside a flag.
				 * NOTE: they must be escaped by '\\' only
				 *	 inside the the format string.
				 */
				fmt++;
				isspec = FALSE;
			} else {
				isspec = isfmtspec(*fmt);
			}
			/*
			 * If isspec is TRUE, the arg beeing checked starts
			 * like a valid flag. Argp now points to the rest.
			 */
			if (isspec) {
				/*
				 * If *argp is '+' and we are on the
				 * beginning of the arg that is currently
				 * checked, this cannot be an inc type flag.
				 */
				if (*argp == '+' && argp == sargp)
					continue;
				/*
				 * skip over to arg of flag
				 */
				if (*argp == '=') {
					if (flags & NOEQUAL)
						return (BADFLAG);
					argp++;
				} else if (*argp != '\0' && haseql) {
					/*
					 * Flag and arg are not separated by a
					 * space.
					 * Check here for:
					 * xxxxx=yyyyy	match on '&'
					 * Checked before:
					 * abc=yyyyy	match on 'abc&'
					 * 		or	 'abc*'
					 * 		or	 'abc#'
					 * We come here if 'argp' starts with
					 * the same sequence as a valid flag
					 * and contains an equal sign.
					 * We have tested before if the text
					 * before 'argp' matches exactly.
					 * At this point we have no exact match
					 * and we only allow to match
					 * the special pattern '&'.
					 * We need this e.g. for 'make'.
					 * We allow any flag type argument to
					 * match the format string "&" to set
					 * up a function that handles all odd
					 * stuff that getargs will not grok.
					 * In addition, to allow getargs to be
					 * used for CPP type flags we allow to
					 * match -Dabc=xyz on 'D&'. Note that
					 * Dabc=xyz will not match 'D&'.
					 */
					if ((!hasdash && argp != sargp) || *fmt != '&')
						goto nextarg;
				}

				/*
				 * The format string 'f* ' may be used
				 * to disallow -ffoo for f*
				 *
				 * The same behavior is implemented for
				 * 'f# '. 'f? ' and 'f& '.
				 */
				if (!haseql && *argp != '\0' &&
				    (fmt[0] == '*' || fmt[0] == '#' ||
				    fmt[0] == '?' || fmt[0] == '&') &&
							fmt[1] == ' ') {
					goto nextarg;
				}

				/*
				 * *arpp == '\0' || !haseql
				 * We come here if 'argp' starts with
				 * the same sequence as a valid flag.
				 * This will match on the following args:
				 * -farg	match on 'f*'
				 * -f12		match on 'f#'
				 * +12		match on '+#'
				 * -12		match on '#'
				 * and all args that are separated from
				 * their flags.
				 * In the switch statement below, we check
				 * if the text after 'argp' (if *argp != 0) or
				 * the next arg is a valid arg for this flag.
				 */
				break;
			} else if (*fmt == *argp) {
				if (argp[1] == '\0' &&
				    (fmt[1] == '\0' || fmt[1] == ',')) {

					if (flags & SETARGS)
						*((int *)curarg) = TRUE;


					return (checkfmt(fmt)); /* XXX */
				}
			} else {
				/*
				 * skip over to next format identifier
				 * & reset arg pointer
				 */
			nextarg:
				while (*fmt != ',' && *fmt != '\0') {
					/* function has extra arg on stack */
					if ((*fmt == '&' || *fmt == '~') &&
					    (flags & (SETARGS|ARGVECTOR)) == SETARGS) {
						curarg = va_arg(args, void *);
					}
					fmt++;
				}
				argp = sargp;
				break;
			}
		}
		switch (*fmt) {

		case '\0':
			/*
			 * Boolean type has been tested before.
			 */
			if (flags & ARGVECTOR) {
				if (flagp[1].ga_format != NULL) {
					flagp++;
					sfmt = fmt = flagp->ga_format;
					if (flags & SETARGS)
						curarg = flagp->ga_arg;
					argp = sargp;
					goto again;
				}
			}
			if (singlecharflag && !doubledash &&
			    (val = dosflags(sargp, vfmt, pac, pav,
							flags & ~SETARGS,
							va_dummy)) == BADFLAG) {
				return (val);
			}
			if (singlecharflag && !doubledash &&
			    (val = dosflags(sargp, vfmt, pac, pav,
							flags,
							oargs)) != BADFLAG) {
				return (val);
			}
			return (BADFLAG);

		case ',':
			fmt++;
			if (fmt[0] == '\0')	/* Should we allow "a,b,c,"? */
				return (BADFMT);
			if (fmt[1] == ',' || fmt[1] == '+' || fmt[1] == '\0')
				singlecharflag++;
			if ((flags & (SETARGS|ARGVECTOR)) == SETARGS)
				curarg = va_arg(args, void *);
			continue;

		case '*':
			if (*argp == '\0' && fmt[1] != '_') {
				if (*pac > 1) {
					(*pac)--;
					(*pav)++;
					argp = **pav;
				} else {
					return (BADFLAG);
				}
			}
			if (fmt[1] == '_')	/* To disallow -f foo for f* */
				fmt++;
			else if (fmt[1] == ' ')	/* To disallow -ffoo for f* */
				fmt++;

			if (flags & SETARGS)
				*((const char **)curarg) = argp;


			return (checkfmt(fmt));

		case '?':
			if (*argp == '\0' && fmt[1] != '_') {
				if (*pac > 1) {
					(*pac)--;
					(*pav)++;
					argp = **pav;
				} else {
					return (BADFLAG);
				}
			}
			if (fmt[1] == '_')	/* To disallow -f c for f? */
				fmt++;
			else if (fmt[1] == ' ')	/* To disallow -fc for f? */
				fmt++;

			/*
			 * Allow -f '' to specify a nul character.
			 * If more than one char arg, it
			 * cannot be a character argument.
			 */
			if (argp[0] != '\0' && argp[1] != '\0')
				goto nextchance;

			if (flags & SETARGS)
				*((char *)curarg) = *argp;


			return (checkfmt(fmt));

		case '+':
			/*
			 * inc type is similar to boolean,
			 * there is no arg in argp to convert.
			 */
			if (*argp != '\0')
				goto nextchance;
			/*
			 * If *fmt is '+' and we are on the beginning
			 * of the format desciptor that is currently
			 * checked, this cannot be an inc type flag.
			 */
			if (fmt == sfmt || fmt[-1] == ',')
				goto nextchance;

			fmtp = fmt;
			if (fmt[1] == 'l' || fmt[1] == 'L') {
				if (fmt[2] == 'l' || fmt[2] == 'L') {
					if (flags & SETARGS)
						*((Llong *)curarg) += 1;
					fmt += 2;
				} else {
					if (flags & SETARGS)
						*((long *)curarg) += 1;
					fmt++;
				}
			} else if (fmt[1] == 's' || fmt[1] == 'S') {
				if (flags & SETARGS)
					*((short *)curarg) += 1;
				fmt++;
			} else if (fmt[1] == 'c' || fmt[1] == 'C') {
				if (flags & SETARGS)
					*((char *)curarg) += 1;
				fmt++;
			} else {
				if (fmt[1] == 'i' || fmt[1] == 'I')
					fmt++;
				if (flags & SETARGS)
					*((int *)curarg) += 1;
			}


			return (checkfmt(fmt));

		case '%':
			/*
			 * inc type is similar to boolean,
			 * there is no arg in argp to convert.
			 */
			if (*argp != '\0')
				goto nextchance;

			fmt++;
			if (*fmt == '1')
				val = TRUE;
			else if (*fmt == '0')
				val = FALSE;
			else
				goto nextchance;

			fmtp = fmt;
			llval = (Llong)val;
			if (fmt[1] == 'l' || fmt[1] == 'L') {
				if (fmt[2] == 'l' || fmt[2] == 'L') {
					if (flags & SETARGS)
						*((Llong *)curarg) = llval;
					fmt += 2;
				} else {
					if (flags & SETARGS)
						*((long *)curarg) = val;
					fmt++;
				}
			} else if (fmt[1] == 's' || fmt[1] == 'S') {
				if (flags & SETARGS)
					*((short *)curarg) = val;
				fmt++;
			} else if (fmt[1] == 'c' || fmt[1] == 'C') {
				if (flags & SETARGS)
					*((char *)curarg) = val;
				fmt++;
			} else {
				if (fmt[1] == 'i' || fmt[1] == 'I')
					fmt++;
				if (flags & SETARGS)
					*((int *)curarg) = val;
			}


			return (checkfmt(fmt));

		case '#':
			if (*argp == '\0' && fmt[1] != '_') {
				if (*pac > 1) {
					(*pac)--;
					(*pav)++;
					argp = **pav;
				} else {
					return (BADFLAG);
				}
			}
			if (fmt[1] == '_')	/* To disallow -f 123 for f# */
				fmt++;
			else if (fmt[1] == ' ')	/* To disallow -f123 for f# */
				fmt++;

			if (*astoll(argp, &llval) != '\0') {
				/*
				 * arg is not a valid number!
				 * go to next format in the format string
				 * and check if arg matches any other type
				 * in the format specs.
				 */
			nextchance:
				while (*fmt != ',' && *fmt != '\0') {
					if ((*fmt == '&' || *fmt == '~') &&
					    (flags & (SETARGS|ARGVECTOR)) == SETARGS) {
						curarg = va_arg(args, void *);
					}
					fmt++;
				}
				argp = sargp;
				*pac = spac;
				*pav = spav;
				continue;
			}
			fmtp = fmt;
			val = (long)llval;
			if (fmt[1] == 'l' || fmt[1] == 'L') {
				if (fmt[2] == 'l' || fmt[2] == 'L') {
					if (flags & SETARGS)
						*((Llong *)curarg) = llval;
					fmt += 2;
				} else {
					if (flags & SETARGS)
						*((long *)curarg) = val;
					fmt++;
				}
			} else if (fmt[1] == 's' || fmt[1] == 'S') {
				if (flags & SETARGS)
					*((short *)curarg) = (short)val;
				fmt++;
			} else if (fmt[1] == 'c' || fmt[1] == 'C') {
				if (flags & SETARGS)
					*((char *)curarg) = (char)val;
				fmt++;
			} else {
				if (fmt[1] == 'i' || fmt[1] == 'I')
					fmt++;
				if (flags & SETARGS)
					*((int *)curarg) = (int)val;
			}

			return (checkfmt(fmt));

		case '~':
			if (*argp != '\0')
				goto nextchance;
			if (haseql) {
				return (BADFLAG);
			}
			goto callfunc;

		case '&':
			if (*argp == '\0' && fmt[1] != '_') {
				if (*pac > 1) {
					(*pac)--;
					(*pav)++;
					argp = **pav;
				} else {
					return (BADFLAG);
				}
			}
		callfunc:

			if (*fmt == '&' && fmt[1] == '_') /* To disallow -f foo for f& */
				fmt++;
			else if (fmt[1] == ' ')	/* To disallow -ffoo for f& */
				fmt++;
			if ((val = checkfmt(fmt)) != NOTAFLAG)
				return (val);

			fmtp = sargp;
			if (hasdash)
				fmtp--;
			if (doubledash)
				fmtp--;
			if ((flags & (SETARGS|ARGVECTOR)) == (SETARGS|ARGVECTOR)) {
				int		ret;

				if (flagp->ga_funcp == NULL)
					return (BADFMT);

				ret = ((*flagp->ga_funcp) (argp, flagp->ga_arg,
								pac, pav, fmtp));
				if (ret != NOTAFILE)
					return (ret);
				fmt++;
			} else
			if (flags & SETARGS) {
				int	ret;
				void	*funarg = va_arg(args, void *);

				if (curarg == NULL)
					return (BADFMT);
				ret = ((*(getpargfun)curarg) (argp, funarg,
								pac, pav, fmtp));
				if (ret != NOTAFILE)
					return (ret);
				fmt++;
			} else {
				return (val);
			}
			/*
			 * Called function returns NOTAFILE: try next format.
			 */
		}
	}
}


/*
 *	parse args for combined single char flags
 */
typedef struct {
	void	*curarg;	/* The pointer to the arg to modify	   */
	void	*curfun;	/* The pointer to the function to call	   */
	char	c;		/* The single char flag character	   */
	char	type;		/* The type of the single char flag	   */
	char	fmt;		/* The format type of the single char flag */
	char	val;		/* The value to assign for BOOL flags	   */
} sflags;

LOCAL int
dosflags(argp, vfmt, pac, pav, flags, oargs)
	register const char	*argp;
		void		*vfmt;
		int		*pac;
		char	*const	**pav;
		int		flags;
		va_list		oargs;
{
	register const char	*fmt = (const char *)vfmt;
	struct ga_flags		*flagp = vfmt;
#define	MAXSF	64
		sflags	sf[MAXSF];
		char	fl[256];
		va_list args;
	register sflags	*rsf	= sf;
	register int	nsf	= 0;
	register const char *p	= argp;
	register int	i;
	register void	*curarg = (void *)0;
	getpargfun	curfun = 0;
		char	type;

	/*
	 * Initialize 'args' to the start of the argument list.
	 * I don't know any portable way to copy an arbitrary
	 * C object so I use a system-specific routine
	 * (probably a macro) from stdarg.h.  (Remember that
	 * if va_list is an array, 'args' will be a pointer
	 * and '&args' won't be what I would need for memcpy.)
	 * It is a system requirement for SVr4 compatibility
	 * to be able to do this assgignement. If your system
	 * defines va_list to be an array but does not define
	 * va_copy() you are lost.
	 * This is needed to make sure, that 'oargs' will not
	 * be clobbered.
	 */
	va_copy(args, oargs);

	if (flags & ARGVECTOR) {
		fmt = flagp->ga_format;
		if (fmt == NULL)
			fmt = "";
		if (flags & SETARGS) {
			curarg = flagp->ga_arg;
			curfun = flagp->ga_funcp;
		}
	} else if (flags & SETARGS) {
		/*
		 * We set curfun to curarg. We later get the real
		 * curarg in case that we see a function callback
		 * but we need curfun first in this case.
		 */
		curarg = va_arg(args, void *);
		curfun = (getpargfun)curarg;
	}

	for (i = 0; i < sizeof (fl); i++) {
		fl[i] = 0;
	}
	while (*p) {
		for (i = 0; i < nsf; i++) {
			if (rsf[i].c == *p)
				break;
		}
		if (i >= MAXSF) {
			va_end(args);
			return (BADFLAG);
		}
		if (i == nsf) {
			rsf[i].curarg = (void *)0;
			rsf[i].curfun = (void *)0;
			rsf[i].c = *p;
			rsf[i].type = (char)-1;
			rsf[i].fmt = '\0';
			rsf[i].val = (char)TRUE;
			nsf++;
		}
		fl[*p & 0xFF] = i;
		p++;
	}

again:
	while (*fmt) {
		if (!isfmtspec(*fmt) &&
		    (fmt[1] == ',' || fmt[1] == '+' ||
		    fmt[1] == '~' || fmt[1] == '%' || fmt[1] == '\0') &&
		    strchr(argp, *fmt)) {
			for (i = 0; i < nsf; i++) {
				if (rsf[i].c == *fmt) {
					if (fmt[1] == '+') {
						rsf[i].fmt = '+';
						fmt++;
						if (fmt[1] == ',' ||
						    fmt[1] == '\0') {
							rsf[i].type = 'i';
						} else if ((fmt[1] == 'l' ||
							    fmt[1] == 'L') &&
							    (fmt[2] == 'l' ||
							    fmt[2] == 'L')) {
							/*
							 * Type 'Q'uad (ll)
							 */
							rsf[i].type = 'Q';
							fmt++;
						} else {
							/*
							 * Type 'l','i','s','c'
							 */
							rsf[i].type = fmt[1];
						}
					} else if (fmt[1] == '%') {
						fmt++;
						rsf[i].fmt = '%';
						if (fmt[1] == '0')
							rsf[i].val = (char)FALSE;
						else if (fmt[1] == '1')
							rsf[i].val = (char)TRUE;
						fmt++;
						if (fmt[1] == ',' ||
						    fmt[1] == '\0') {
							rsf[i].type = 'i';
						} else if ((fmt[1] == 'l' ||
							    fmt[1] == 'L') &&
							    (fmt[2] == 'l' ||
							    fmt[2] == 'L')) {
							/*
							 * Type 'Q'uad (ll)
							 */
							rsf[i].type = 'Q';
							fmt++;
						} else {
							/*
							 * Type 'l','i','s','c'
							 */
							rsf[i].type = fmt[1];
						}
					} else if (fmt[1] == '~') {
						/*
						 * Let fmt point to ',' to
						 * prevent to fetch the
						 * func arg twice.
						 */
						fmt += 2;
						rsf[i].fmt = '~';
						rsf[i].type = '~';
						rsf[i].curfun = (void *)curfun;
						if ((flags & (SETARGS|ARGVECTOR)) == SETARGS)
							curarg = va_arg(args, void *);
					} else {
						/*
						 * ',' or '\0' for BOOL
						 */
						rsf[i].type = fmt[1];
					}
					rsf[i].curarg = curarg;
					break;
				}
			}
		}
		while (*fmt != ',' && *fmt != '\0') {
			/*
			 * function has extra arg on stack. The code above
			 * prevents us from fetching this arg twice.
			 */
			if ((*fmt == '&' || *fmt == '~') &&
			    (flags & (SETARGS|ARGVECTOR)) == SETARGS) {
				curarg = va_arg(args, void *);
			}
			fmt++;
		}
		if (*fmt != '\0')
			fmt++;
		else
			break;

		if ((flags & (SETARGS|ARGVECTOR)) == SETARGS) {
			/*
			 * We set curfun to curarg. We later get the real
			 * curarg in case that we see a function callback
			 * but we need curfun first in this case.
			 */
			curarg = va_arg(args, void *);
			curfun = (getpargfun)curarg;
		}
	}
	if ((flags & ARGVECTOR) && flagp[1].ga_format != NULL) {
		flagp++;
		fmt = flagp->ga_format;
		if (flags & SETARGS) {
			curarg = flagp->ga_arg;
			curfun = flagp->ga_funcp;
		}
		goto again;
	}

	for (p = argp; *p; p++) {
		char tfmt;

		i = fl[*p & 0xFF];
		tfmt  = rsf[i].fmt;
		type = rsf[i].type;
		if (type == (char)-1) {
			return (BADFLAG);
		}

		if ((flags & SETARGS) &&
		    (rsf[i].curfun || rsf[i].curarg)) {
			if (type == ',' || type == '\0') {
				*((int *)rsf[i].curarg) = TRUE;
			} else if (type == 'i' || type == 'I') {
				if (tfmt == '+')
					*((int *)rsf[i].curarg) += 1;
				else
					*((int *)rsf[i].curarg) = rsf[i].val;
			} else if (type == 'l' || type == 'L') {
				if (tfmt == '+')
					*((long *)rsf[i].curarg) += 1;
				else
					*((long *)rsf[i].curarg) = rsf[i].val;
			} else if (type == 'Q') {
				if (tfmt == '+')
					*((Llong *)rsf[i].curarg) += 1;
				else
					*((Llong *)rsf[i].curarg) = rsf[i].val;
			} else if (type == 's' || type == 'S') {
				if (tfmt == '+')
					*((short *)rsf[i].curarg) += 1;
				else
					*((short *)rsf[i].curarg) = rsf[i].val;
			} else if (type == 'c' || type == 'C') {
				if (tfmt == '+')
					*((char *)rsf[i].curarg) += 1;
				else
					*((char *)rsf[i].curarg) = rsf[i].val;
			} else if (type == '~') {
				int	ret;
				char	cfmt[3];

				cfmt[0] = '-';
				cfmt[1] = rsf[i].c;
				cfmt[2] = '\0';

				if (rsf[i].curfun == NULL)
					return (BADFMT);
				ret = ((*(getpargfun)rsf[i].curfun) ("",
								rsf[i].curarg,
								pac, pav, cfmt));
					return (ret);
			} else {
				return (BADFLAG);
			}
		}
	}
	return (NOTAFLAG);
}

/*
 *	If the next format character is a comma or the string delimiter,
 *	there are no invalid format specifiers. Return success.
 *	Otherwise raise the getarg_bad_format condition.
 */
LOCAL int
checkfmt(fmt)
	const char	*fmt;
{
	char	c;

	c = *(++fmt);	/* non constant expression */


	if (c == ',' || c == '\0') {
		return (NOTAFLAG);
	} else {
		raisecond("getarg_bad_format", (long)fmt);
		return (BADFMT);
	}
}

/*
 *	Parse the string as long as valid characters can be found.
 *	Valid flag identifiers are chosen from the set of
 *	alphanumeric characters, '-' and '_'.
 *	If the next character is an equal sign the string
 *	contains a valid flag identifier.
 */
LOCAL int
checkeql(str)
	register const char *str;
{
	register unsigned char c;

	for (c = (unsigned char)*str;
			isalnum(c) || c == '_' || c == '-' || c == '+';
								c = *++str)
		/* LINTED */
		;
	return (c == '=');
}

EXPORT char *
getargerror(err)
	int	err;
{
	if (err < RETMIN || err > RETMAX)
		return ("Illegal arg error");
	return (RNAME(err));
}
