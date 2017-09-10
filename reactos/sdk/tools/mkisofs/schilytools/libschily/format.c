/* @(#)format.c	1.62 17/08/03 Copyright 1985-2017 J. Schilling */
/*
 *	format
 *	common code for printf fprintf & sprintf
 *
 *	allows recursive printf with "%r", used in:
 *	error, comerr, comerrno, errmsg, errmsgno and the like
 *
 *	Copyright (c) 1985-2017 J. Schilling
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
#include <schily/varargs.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#ifdef	DEBUG
#include <schily/unistd.h>
#endif
#if	!defined(HAVE_STDLIB_H) || !defined(HAVE_GCVT)
extern	char	*gcvt __PR((double, int, char *));
#endif
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>

/*
 * As Llong is currently a 'best effort' long long, we usually need to
 * include long long print formats.
 * This may go away, if we implement maxint_t formats.
 */
#ifndef	USE_LONGLONG
#define	USE_LONGLONG
#endif

#ifdef	NO_LONGLONG
#undef	USE_LONGLONG
#endif

#ifndef	USE_NL_ARGS
#define	USE_NL_ARGS
#endif

#ifdef	NO_NL_ARGS
#undef	USE_NL_ARGS
#endif

/*
 * Avoid to keep copies of the variable arg list in case that
 * format() was compiled without including NL support for
 * argument reordering.
 */
#ifdef	USE_NL_ARGS
#define	args		fargs.ap	/* Use working copy */
#else
#define	args		oargs		/* Directly use format() arg */
#endif

/*
 * We may need to decide whether we should check whether all
 * flags occur according to the standard which is either directly past:
 * "%" or directly past "%n$".
 *
 * This however may make printf() slower in some cases.
 */
#ifdef	USE_CHECKFLAG
#define	CHECKFLAG()	if (fa.flags & GOTSTAR) goto flagerror
#else
#define	CHECKFLAG()
#endif

#ifdef	NO_USER_XCVT
	/*
	 * We cannot define our own gcvt() so we need to use a
	 * local name instead.
	 */
#ifndef	HAVE_GCVT
#	define	gcvt	js_gcvt
EXPORT	char *gcvt	__PR((double value, int ndigit, char *buf));
#endif
#endif

/*
 * Some CPU's (e.g. PDP-11) cannot do logical shifts.
 * They use rotate instead. Masking the low bits before,
 * makes rotate work too.
 */
#define	allmask(t)	((unsigned t)~((unsigned t)0))
#define	lowmask(t, x)	((unsigned t)~((unsigned t)((1 << (x))-1)))
#define	rshiftmask(t, s)((allmask(t) & lowmask(t, s)) >> (s))

#define	CHARMASK	makemask(char)
#define	SHORTMASK	makemask(short)
#define	INTMASK		makemask(int)
#define	LONGMASK	makemask(long)

#ifdef	DIVLBYS
extern	long	divlbys();
extern	long	modlbys();
#else
#define	divlbys(val, base)	((val)/(base))
#define	modlbys(val, base)	((val)%(base))
#endif

/*
 *	We use macros here to avoid the need to link to the international
 *	character routines.
 *	We don't need internationalization for our purpose.
 */
#define	is_dig(c)	(((c) >= '0') && ((c) <= '9'))
#define	is_cap(c)	((c) >= 'A' && (c) <= 'Z')
#define	to_cap(c)	(is_cap(c) ? c : c - 'a' + 'A')
#define	cap_ty(c)	(is_cap(c) ? 'L' : 'I')

#ifdef	HAVE_LONGLONG
typedef union {
	Ullong	ll;
	Ulong	l[2];
	char	c[8];
} quad_u;
#endif

typedef struct f_args {
#ifdef	FORMAT_BUFFER
#define	BFSIZ	256
	char	*ptr;			/* Current ptr in buf		*/
	int	cnt;			/* Free char count in buf	*/
#else
	void  (*outf)__PR((char, void *)); /* Func from format(fun, arg)	*/
#endif
	void	*farg;			/* Arg from format (fun, arg)	*/
	int	minusflag;		/* Fieldwidth is negative	*/
	int	flags;			/* General flags (+-#)		*/
	int	fldwidth;		/* Field width as in %3d	*/
	int	signific;		/* Significant chars as in %.4d	*/
	int	lzero;			/* Left '0' pad flag		*/
	char	*buf;			/* Out print buffer		*/
	char	*bufp;			/* Write ptr into buffer	*/
	char	fillc;			/* Left fill char (' ' or '0')	*/
	char	*prefix;		/* Prefix to print before buf	*/
	int	prefixlen;		/* Len of prefix ('+','-','0x')	*/
#ifdef	FORMAT_BUFFER
					/* rarely used members last:	*/
	char	iobuf[BFSIZ];		/* buffer for stdio		*/
	FILE	*fp;			/* FILE * for fprformat()	*/
	int	err;			/* FILE * I/O error		*/
#endif
} f_args;

#define	MINUSFLG	1	/* '-' flag */
#define	PLUSFLG		2	/* '+' flag */
#define	SPACEFLG	4	/* ' ' flag */
#define	HASHFLG		8	/* '#' flag */
#define	APOFLG		16	/* '\'' flag */
#define	GOTDOT		32	/* '.' found */
#define	GOTSTAR		64	/* '*' found */

#define	FMT_ARGMAX	30	/* Number of fast args */

LOCAL	void	prnum  __PR((Ulong, unsigned, f_args *));
LOCAL	void	prdnum __PR((Ulong, f_args *));
LOCAL	void	pronum __PR((Ulong, f_args *));
LOCAL	void	prxnum __PR((Ulong, f_args *));
LOCAL	void	prXnum __PR((Ulong, f_args *));
#ifdef	USE_LONGLONG
LOCAL	void	prlnum  __PR((Ullong, unsigned, f_args *));
LOCAL	void	prldnum __PR((Ullong, f_args *));
LOCAL	void	prlonum __PR((Ullong, f_args *));
LOCAL	void	prlxnum __PR((Ullong, f_args *));
LOCAL	void	prlXnum __PR((Ullong, f_args *));
#endif
LOCAL	int	prbuf  __PR((const char *, f_args *));
LOCAL	int	prc    __PR((char, f_args *));
LOCAL	int	prstring __PR((const char *, f_args *));
#ifdef	DEBUG
LOCAL	void	dbg_print __PR((char *fmt, int a, int b, int c, int d, int e, int f, int g, int h, int i));
#endif

#ifdef	USE_NL_ARGS
#ifndef	FORMAT_FUNC_NAME
#define	FORMAT_IMPL
EXPORT	void	_fmtarglist __PR((const char *fmt, va_lists_t, va_lists_t arglist[]));
EXPORT	void	_fmtgetarg  __PR((const char *fmt, int num, va_lists_t *));
#else
extern	void	_fmtarglist __PR((const char *fmt, va_lists_t, va_lists_t arglist[]));
extern	void	_fmtgetarg  __PR((const char *fmt, int num, va_lists_t *));
#endif
#endif

#ifdef	FORMAT_BUFFER
LOCAL char	xflsbuf	__PR((int c, f_args *ap));

LOCAL char
xflsbuf(c, ap)
	int	c;
	f_args	*ap;
{
	*ap->ptr++ = c;
	if (filewrite((FILE *)ap->fp, ap->iobuf, ap->ptr - ap->iobuf) < 0)
		ap->err = 1;

	ap->cnt = BFSIZ;
	ap->ptr = ap->iobuf;
	return (c);
}

#undef	ofun
#define	ofun(c, xp)		(--((f_args *)xp)->cnt <= 0 ? \
					xflsbuf(c, (f_args *)xp) : \
					(*(((f_args *)xp)->ptr)++ = (c)))

#endif

#ifndef	FORMAT_FUNC_NAME
#define	FORMAT_FUNC_NAME	format
#define	FORMAT_FUNC_PARM

#define	FORMAT_FUNC_PROTO_DECL	void (*fun)(char, void *),
#define	FORMAT_FUNC_KR_DECL	register void (*fun)();
#define	FORMAT_FUNC_KR_ARGS	fun,

#define	ofun(c, fp)		(*fun)(c, fp)
#endif

#ifdef	FORMAT_BUFFER
#define	FARG		((void *)((UIntptr_t)farg|1))
#else
#define	FARG		farg
#endif

#ifdef	PROTOTYPES
EXPORT int
FORMAT_FUNC_NAME(FORMAT_FUNC_PROTO_DECL
			void *farg,
			const char *fmt,
			va_list oargs)
#else
EXPORT int
FORMAT_FUNC_NAME(FORMAT_FUNC_KR_ARGS farg, fmt, oargs)
	FORMAT_FUNC_KR_DECL
	register void	*farg;
	register char	*fmt;
	va_list		oargs;
#endif
{
#ifdef	FORMAT_LOW_MEM
	char buf[512];
#else
	char buf[8192];
#endif
	const char *sfmt;
	register int unsflag;
	register long val;
	register char type;
	register char mode;
	register char c;
	int count;
	int num;
	int i;
	short sh;
	const char *str;
	double dval;
#ifdef	USE_LONGLONG
	Llong llval = 0;
#endif
	Ulong res;
	char *rfmt;
	f_args	fa;
#ifdef	USE_NL_ARGS
	va_lists_t	fargs;		/* Used to get arguments */
	va_lists_t	sargs;		/* Saved argument state */
	va_lists_t	arglist[FMT_ARGMAX+1]; /* List of fast args */
	const char 	*ofmt = fmt;	/* Saved original format */
	BOOL		didlist = FALSE; /* Need to scan arguments */
#endif

#ifdef	FORMAT_BUFFER
	if (((UIntptr_t)farg & 1) == 0) { /* Called externally	*/
		fa.cnt	= BFSIZ;
		fa.ptr	= fa.iobuf;
		fa.fp	= (FILE *)farg;
		fa.err	= 0;
		farg	= fa.farg = &fa;
	} else {			/* recursion		*/
		farg = (void *)((UIntptr_t)farg & ~1);
	}
#endif
#ifdef	FORMAT_FUNC_PARM
	fa.outf = fun;
#endif
	fa.farg = farg;
	count = 0;

#ifdef	USE_NL_ARGS
	va_copy(sargs.ap, oargs);	/* Keep a copy in sargs */
	fargs = sargs;			/* Make a working copy  */
#endif

	/*
	 * Main loop over the format string.
	 * Increment and check for end of string is made here.
	 */
	for (; *fmt != '\0'; fmt++) {
		c = *fmt;
		while (c != '%') {
			if (c == '\0')
				goto out;
			ofun(c, farg);
			c = *(++fmt);
			count++;
		}

		/*
		 * We reached a '%' sign.
		 */
		buf[0] = '\0';
		fa.buf = fa.bufp = buf;
		fa.minusflag = 0;
		fa.flags = 0;
		fa.fldwidth = 0;
		fa.signific = -1;
		fa.lzero = 0;
		fa.fillc = ' ';
		fa.prefixlen = 0;
		sfmt = fmt;
		unsflag = FALSE;
		type = '\0';
		mode = '\0';
		/*
		 * %<flags>f.s<length-mod><conversion-spec>
		 * %<flags>*.*<length-mod><conversion-spec>
		 * %n$<flags>f.s<length-mod><conversion-spec>
		 * %n$<flags>*n$.*n$<length-mod><conversion-spec>
		 */
	newflag:
		switch (*(++fmt)) {

		case '+':
			CHECKFLAG();
			fa.flags |= PLUSFLG;
			goto newflag;

		case '-':
			CHECKFLAG();
			fa.minusflag++;
			fa.flags |= MINUSFLG;
			goto newflag;

		case ' ':
			CHECKFLAG();
			/*
			 * If the space and the + flag are present,
			 * the space flag will be ignored.
			 */
			fa.flags |= SPACEFLG;
			goto newflag;

		case '#':
			CHECKFLAG();
			fa.flags |= HASHFLG;
			goto newflag;

		case '\'':
			CHECKFLAG();
			fa.flags |= APOFLG;
			goto newflag;

		case '.':
			fa.flags |= GOTDOT;
			fa.signific = 0;
			goto newflag;

		case '*':
			fa.flags |= GOTSTAR;
#ifdef	USE_NL_ARGS
			if (is_dig(fmt[1])) {	/* *n$ */
				fmt++;		/* Eat up '*' */
				goto dodig;
			}
#endif
			if (!(fa.flags & GOTDOT)) {
				fa.fldwidth = va_arg(args, int);
				/*
				 * A negative fieldwith is a minus flag with a
				 * positive fieldwidth.
				 */
				if (fa.fldwidth < 0) {
					fa.fldwidth = -fa.fldwidth;
					fa.minusflag = 1;
				}
			} else {
				/*
				 * A negative significance (precision) is taken
				 * as if the precision and '.' were omitted.
				 */
				fa.signific = va_arg(args, int);
				if (fa.signific < 0)
					fa.signific = -1;
			}
			goto newflag;

		case '0':
			/*
			 * '0' may be a flag.
			 */
			if (!(fa.flags & (GOTDOT | GOTSTAR | MINUSFLG)))
				fa.fillc = '0';
			/* FALLTHRU */
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
#ifdef	USE_NL_ARGS
		dodig:
#endif
			num = *fmt++ - '0';
			while (c = *fmt, is_dig(c)) {
				num *= 10;
				num += c - '0';
				fmt++;
			}
#ifdef	USE_NL_ARGS
			if (c == '$')
				goto doarglist;
#endif
			fmt--;			/* backup to last digit */
			if (!(fa.flags & GOTDOT))
				fa.fldwidth = num;
			else
				fa.signific = num;
			goto newflag;

#ifdef	USE_NL_ARGS
		doarglist:
			{
			va_lists_t	tmp;	/* Temporary arg state */
			if (num <= 0)		/* Illegal arg offset */
				goto newflag;	/* Continue after '$' */
			if (!didlist) {		/* Need to init arglist */
				_fmtarglist(ofmt, sargs, arglist);
				didlist = TRUE;
			}
			if (num <= FMT_ARGMAX) {
				tmp = arglist[num-1];
			} else {
				tmp = arglist[FMT_ARGMAX-1];
				_fmtgetarg(ofmt, num, &tmp);
			}
			if (!(fa.flags & GOTSTAR)) {
				fargs = tmp;
			} else {
				if (!(fa.flags & GOTDOT)) {
					fa.fldwidth = va_arg(tmp.ap, int);
					/*
					 * A negative fieldwith is a minus flag
					 * with a positive fieldwidth.
					 */
					if (fa.fldwidth < 0) {
						fa.fldwidth = -fa.fldwidth;
						fa.minusflag = 1;
					}
				} else {
					/*
					 * A negative significance (precision)
					 * is taken as if the precision and '.'
					 * were omitted.
					 */
					fa.signific = va_arg(tmp.ap, int);
					if (fa.signific < 0)
						fa.signific = -1;
				}
			}
			goto newflag;
			}
#endif

#ifdef	USE_CHECKFLAG
		flagerror:
			fmt = ++sfmt;		/* Don't print '%'   */
			continue;
#endif
		}

		if (strchr("UCSIL", *fmt)) {
			/*
			 * Enhancements to K&R and ANSI:
			 *
			 * got a type specifyer
			 *
			 * XXX 'S' in C99 is %ls, 'S' should become 'H'
			 */
			if (*fmt == 'U') {
				fmt++;
				unsflag = TRUE;
			}
			if (!strchr("CSILZODX", *fmt)) {
				/*
				 * Got only 'U'nsigned specifyer,
				 * use default type and mode.
				 */
				type = 'I';
				mode = 'D';
				fmt--;
			} else if (!strchr("CSIL", *fmt)) {
				/*
				 * no type, use default
				 */
				type = 'I';
				mode = *fmt;
			} else {
				/*
				 * got CSIL type
				 */
				type = *fmt++;
				if (!strchr("ZODX", mode = *fmt)) {
					/*
					 * Check long double "Le", "Lf" or "Lg"
					 */
					if (type == 'L' &&
					    (mode == 'e' ||
					    mode == 'f' ||
					    mode == 'g'))
						goto checkfmt;
					fmt--;
					mode = 'D'; /* default mode */
				}
			}
		} else {
	checkfmt:
		switch (*fmt) {

		case 'h':
			if (!type)
				type = 'H';	/* convert to short type */
			goto getmode;

		case 'l':
			if (!type)
				type = 'L';	/* convert to long type */
			goto getmode;

		case 'j':
			if (!type)
				type = 'J';	/* convert to intmax_t type */
			goto getmode;

		case 'z':			/* size_t */
#if	SIZEOF_SIZE_T == SIZEOF_INT
			if (!type)
				type = 'I';	/* convert to int type */
#else
#if	SIZEOF_SIZE_T == SIZEOF_LONG_INT
			if (!type)
				type = 'L';	/* convert to long type */
#else
#if	SIZEOF_SIZE_T == SIZEOF_LLONG
			if (!type)
				type = 'Q';	/* convert to long long type */
#else
error sizeof (size_t) is unknown
#endif
#endif
#endif
			goto getmode;

		case 't':			/* ptrdiff_t */
#if	SIZEOF_PTRDIFF_T == SIZEOF_INT
			if (!type)
				type = 'I';	/* convert to int type */
#else
#if	SIZEOF_PTRDIFF_T == SIZEOF_LONG_INT
			if (!type)
				type = 'L';	/* convert to long type */
#else
#if	SIZEOF_PTRDIFF_T == SIZEOF_LLONG
			if (!type)
				type = 'Q';	/* convert to long long type */
#else
error sizeof (ptrdiff_t) is unknown
#endif
#endif
#endif
		/*
		 * XXX Future length modifiers:
		 * XXX	'L' with double: long double
		 */

		getmode:
			if (!strchr("udioxXn", *(++fmt))) {
				/*
				 * %hhd -> char in decimal
				 */
				if (type == 'H' && *fmt == 'h') {
					type = 'C';
					goto getmode;
				}
#ifdef	USE_LONGLONG
				if (type == 'L' && *fmt == 'l') {
					type = 'Q';
					goto getmode;
				}
#endif
				fmt--;
				mode = 'D';
			} else {		/* One of "udioxXn": */
				mode = *fmt;
				if (mode == 'n')
					goto gotn;
				if (mode != 'x')
					mode = to_cap(mode);
				if (mode == 'U')
					unsflag = TRUE;
				else if (mode == 'I')	/* XXX */
					mode = 'D';
			}
			break;
		case 'x':
			mode = 'x';
			goto havemode;
		case 'X':
			mode = 'X';
			type = 'I';
			goto havemode;
		case 'u':
			unsflag = TRUE;
		/*
		 * XXX Need to remove uppercase letters for 'long'
		 * XXX in future for POSIX/C99 compliance.
		 */
			/* FALLTHRU */
		case 'o': case 'O':
		case 'd': case 'D':
		case 'i': case 'I':
		case 'Z':
			mode = to_cap(*fmt);
		havemode:
			if (!type)
				type = cap_ty(*fmt);
#ifdef	DEBUG
			dbg_print("*fmt: '%c' mode: '%c' type: '%c'\n",
							*fmt, mode, type);
#endif
			if (mode == 'I')	/* XXX kann entfallen */
				mode = 'D';	/* wenn besseres uflg */
			break;
		case 'p':
			mode = 'P';
			type = 'L';
			break;

		case '%':
			count += prc('%', &fa);
			continue;
		case ' ':
			count += prbuf("", &fa);
			continue;
		case 'c':
			c = va_arg(args, int);
			count += prc(c, &fa);
			continue;
		case 's':
			str = va_arg(args, char *);
			count += prstring(str, &fa);
			continue;
		case 'b':
			str = va_arg(args, char *);
			fa.signific = va_arg(args, int);
			count += prstring(str, &fa);
			continue;

#ifndef	NO_FLOATINGPOINT
		case 'e':
			if (fa.signific == -1)
				fa.signific = 6;
			if (type == 'L') {
#ifdef	HAVE_LONGDOUBLE
				long double ldval = va_arg(args, long double);

#if	(defined(HAVE_QECVT) || defined(HAVE__LDECVT))
				qftoes(buf, ldval, 0, fa.signific);
				count += prbuf(buf, &fa);
				continue;
#else
				dval = ldval;
#endif
#endif
			}
			dval = va_arg(args, double);
			ftoes(buf, dval, 0, fa.signific);
			count += prbuf(buf, &fa);
			continue;
		case 'f':
			if (fa.signific == -1)
				fa.signific = 6;
			if (type == 'L') {
#ifdef	HAVE_LONGDOUBLE
				long double ldval = va_arg(args, long double);

#if	(defined(HAVE_QFCVT) || defined(HAVE__LDFCVT))
				qftofs(buf, ldval, 0, fa.signific);
				count += prbuf(buf, &fa);
				continue;
#else
				dval = ldval;
#endif
#endif
			}
			dval = va_arg(args, double);
			ftofs(buf, dval, 0, fa.signific);
			count += prbuf(buf, &fa);
			continue;
		case 'g':
			if (fa.signific == -1)
				fa.signific = 6;
			if (fa.signific == 0)
				fa.signific = 1;
			if (type == 'L') {
#ifdef	HAVE_LONGDOUBLE
				long double ldval = va_arg(args, long double);

#if	(defined(HAVE_QGCVT) || defined(HAVE__LDGCVT))

#ifdef	HAVE__LDGCVT
#define	qgcvt(ld, n, b)	_ldgcvt(*(long_double *)&ld, n, b)
#endif
				(void) qgcvt(ldval, fa.signific, buf);
				count += prbuf(buf, &fa);
				continue;
#else
				dval = ldval;
#endif
#endif
			}
			dval = va_arg(args, double);
			(void) gcvt(dval, fa.signific, buf);
			count += prbuf(buf, &fa);
			continue;
#else
#	ifdef	USE_FLOATINGARGS
		case 'e':
		case 'f':
		case 'g':
			dval = va_arg(args, double);
			continue;
#	endif
#endif

		case 'r':			/* recursive printf */
		case 'R':			/* recursive printf */
			rfmt  = va_arg(args, char *);
			/*
			 * I don't know any portable way to get an arbitrary
			 * C object from a var arg list so I use a
			 * system-specific routine __va_arg_list() that knows
			 * if 'va_list' is an array. You will not be able to
			 * assign the value of __va_arg_list() but it works
			 * to be used as an argument of a function.
			 * It is a requirement for recursive printf to be able
			 * to use this function argument. If your system
			 * defines va_list to be an array you need to know this
			 * via autoconf or another mechanism.
			 * It would be nice to have something like
			 * __va_arg_list() in stdarg.h
			 */
			count += FORMAT_FUNC_NAME(FORMAT_FUNC_KR_ARGS
					FARG, rfmt, __va_arg_list(args));
			continue;

		gotn:
		case 'n':
			switch (type) {

			case 'C': {
				signed char *cp = va_arg(args, signed char *);

				*cp = count;
				}
				continue;
			case 'H': {
				short	*sp = va_arg(args, short *);

				*sp = count;
				}
				continue;
			case 'L': {
				long	*lp = va_arg(args, long *);

				*lp = count;
				}
				continue;
#ifdef	USE_LONGLONG
			case 'J':		/* For now Intmax_t is Llong */
			case 'Q': {
				Llong *qp = va_arg(args, Llong *);

				*qp = count;
				}
				continue;
#endif
			default: {
				int	*ip = va_arg(args, int *);

				*ip = count;
				}
				continue;
			}

		default:			/* Unknown '%' format */
			sfmt++;			/* Dont't print '%'   */
			count += fmt - sfmt;
			while (sfmt < fmt)
				ofun(*(sfmt++), farg);
			if (*fmt == '\0') {
				fmt--;
				continue;
			} else {
				ofun(*fmt, farg);
				count++;
				continue;
			}
		}
		}
		/*
		 * print numbers:
		 * first prepare type 'C'har, s'H'ort, 'I'nt, or 'L'ong
		 * or 'Q'ad and 'J'==maxint_t
		 */
		switch (type) {

		case 'C':
			c = va_arg(args, int);
			val = c;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= CHARMASK;
#else
				val = (unsigned char)val;
#endif
			break;
		case 'H':
		case 'S':			/* XXX remove 'S' in future */
			sh = va_arg(args, int);
			val = sh;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= SHORTMASK;
#else
				val = (unsigned short)val;
#endif
			break;
		case 'I':
		default:
			i = va_arg(args, int);
			val = i;		/* extend sign here */
			if (unsflag || mode != 'D')
#ifdef	DO_MASK
				val &= INTMASK;
#else
				val = (unsigned int)val;
#endif
			break;
		case 'P':
		case 'L':
			val = va_arg(args, long);
			break;
#ifdef	USE_LONGLONG
		case 'J':			/* For now Intmax_t is Llong */
			type = 'Q';		/* use 'Q' for processing    */
		case 'Q':
			llval = va_arg(args, Llong);
			val = llval != 0;
			break;
#endif
		}

		/*
		 * Final print out, take care of mode:
		 * mode is one of: 'O'ctal, 'D'ecimal, or he'X'
		 * oder 'Z'weierdarstellung.
		 */
		fa.bufp = &buf[sizeof (buf)-1];
		*--fa.bufp = '\0';

		if (val == 0 && mode != 'D') {
		printzero:
			/*
			 * Printing '0' with fieldwidth 0 results in no chars.
			 */
			fa.lzero = -1;
			if (fa.signific >= 0)
				fa.fillc = ' ';
			count += prstring("0", &fa);
			continue;
		} else switch (mode) {

		case 'D':
#ifdef	USE_LONGLONG
			if (type == 'Q') {
				if (!unsflag && llval < 0) {
					fa.prefix = "-";
					fa.prefixlen = 1;
					llval = -llval;
				} else if (fa.flags & PLUSFLG) {
					fa.prefix = "+";
					fa.prefixlen = 1;
				} else if (fa.flags & SPACEFLG) {
					fa.prefix = " ";
					fa.prefixlen = 1;
				}
				if (llval == 0)
					goto printzero;
				goto prunsigned;
			}
#endif
			if (!unsflag && val < 0) {
				fa.prefix = "-";
				fa.prefixlen = 1;
				val = -val;
			} else if (fa.flags & PLUSFLG) {
				fa.prefix = "+";
				fa.prefixlen = 1;
			} else if (fa.flags & SPACEFLG) {
				fa.prefix = " ";
				fa.prefixlen = 1;
			}
			if (val == 0)
				goto printzero;
			/* FALLTHRU */
		case 'U':
			/* output a long unsigned decimal number */
#ifdef	USE_LONGLONG
		prunsigned:
			if (type == 'Q')
				prldnum(llval, &fa);
			else
#endif
			prdnum(val, &fa);
			break;
		case 'O':
			/* output a long octal number */
			if (fa.flags & HASHFLG) {
				fa.prefix = "0";
				fa.prefixlen = 1;
			}
#ifdef	USE_LONGLONG
			if (type == 'Q') {
				prlonum(llval, &fa);
			} else
#endif
			{
				pronum(val & 07, &fa);
				if ((res = (val>>3) & rshiftmask(long, 3)) != 0)
					pronum(res, &fa);
			}
			break;
		case 'p':
		case 'x':
			/* output a hex long */
			if (fa.flags & HASHFLG) {
				fa.prefix = "0x";
				fa.prefixlen = 2;
			}
#ifdef	USE_LONGLONG
			if (type == 'Q')
				prlxnum(llval, &fa);
			else
#endif
			{
				prxnum(val & 0xF, &fa);
				if ((res = (val>>4) & rshiftmask(long, 4)) != 0)
					prxnum(res, &fa);
			}
			break;
		case 'P':
		case 'X':
			/* output a hex long */
			if (fa.flags & HASHFLG) {
				fa.prefix = "0X";
				fa.prefixlen = 2;
			}
#ifdef	USE_LONGLONG
			if (type == 'Q')
				prlXnum(llval, &fa);
			else
#endif
			{
				prXnum(val & 0xF, &fa);
				if ((res = (val>>4) & rshiftmask(long, 4)) != 0)
					prXnum(res, &fa);
			}
			break;
		case 'Z':
			/* output a binary long */
#ifdef	USE_LONGLONG
			if (type == 'Q')
				prlnum(llval, 2, &fa);
			else
#endif
			{
				prnum(val & 0x1, 2, &fa);
				if ((res = (val>>1) & rshiftmask(long, 1)) != 0)
					prnum(res, 2, &fa);
			}
		}
		fa.lzero = -1;
		/*
		 * If a precision (fielwidth) is specified
		 * on diouXx conversions, the '0' flag is ignored.
		 */
		if (fa.signific >= 0)
			fa.fillc = ' ';
		count += prbuf(fa.bufp, &fa);
	}
out:
#ifdef	FORMAT_BUFFER
	if (farg == &fa) {		/* Top level call, flush buffer */
		if (fa.err)
			return (EOF);
		if ((fa.ptr != fa.iobuf) &&
		    (filewrite(fa.fp, fa.iobuf, fa.ptr - fa.iobuf) < 0))
			return (EOF);
	}
#endif
	return (count);
}

/*
 * Routines to print (not negative) numbers in an arbitrary base
 */
LOCAL	unsigned char	dtab[]  = "0123456789abcdef";
LOCAL	unsigned char	udtab[] = "0123456789ABCDEF";

LOCAL void
prnum(val, base, fa)
	register Ulong val;
	register unsigned base;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[modlbys(val, base)];
		val = divlbys(val, base);
	} while (val > 0);

	fa->bufp = p;
}

LOCAL void
prdnum(val, fa)
	register Ulong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[modlbys(val, (unsigned)10)];
		val = divlbys(val, (unsigned)10);
	} while (val > 0);

	fa->bufp = p;
}

/*
 * We may need to use division here too (PDP-11, non two's complement ...)
 */
LOCAL void
pronum(val, fa)
	register Ulong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[val & 7];
		val >>= 3;
	} while (val > 0);

	fa->bufp = p;
}

LOCAL void
prxnum(val, fa)
	register Ulong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[val & 15];
		val >>= 4;
	} while (val > 0);

	fa->bufp = p;
}

LOCAL void
prXnum(val, fa)
	register Ulong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = udtab[val & 15];
		val >>= 4;
	} while (val > 0);

	fa->bufp = p;
}

#ifdef	USE_LONGLONG
LOCAL void
prlnum(val, base, fa)
	register Ullong val;
	register unsigned base;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[modlbys(val, base)];
		val = divlbys(val, base);
	} while (val > 0);

	fa->bufp = p;
}

LOCAL void
prldnum(val, fa)
	register Ullong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[val % (unsigned)10];
		val = val / (unsigned)10;
	} while (val > 0);

	fa->bufp = p;
}

LOCAL void
prlonum(val, fa)
	register Ullong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[val & 7];
		val >>= 3;
	} while (val > 0);

	fa->bufp = p;
}

LOCAL void
prlxnum(val, fa)
	register Ullong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = dtab[val & 15];
		val >>= 4;
	} while (val > 0);

	fa->bufp = p;
}

LOCAL void
prlXnum(val, fa)
	register Ullong val;
	f_args *fa;
{
	register char *p = fa->bufp;

	do {
		*--p = udtab[val & 15];
		val >>= 4;
	} while (val > 0);

	fa->bufp = p;
}

#endif

/*
 * Final buffer print out routine.
 */
LOCAL int
prbuf(s, fa)
	register const char *s;
	f_args *fa;
{
	register int diff;
	register int rfillc;
	register void *arg				= fa->farg;
#ifdef	FORMAT_FUNC_PARM
	register void (*fun) __PR((char, void *))	= fa->outf;
#endif
	register int count;
	register int lzero = 0;

	count = strlen(s);

	/*
	 * lzero becomes the number of left fill chars needed to reach signific
	 */
	if (fa->lzero < 0 && count < fa->signific)
		lzero = fa->signific - count;
	count += lzero + fa->prefixlen;
	diff = fa->fldwidth - count;
	if (diff > 0)
		count += diff;

	if (fa->prefixlen && fa->fillc != ' ') {
		while (*fa->prefix != '\0')
			ofun(*fa->prefix++, arg);
	}
	if (!fa->minusflag) {
		rfillc = fa->fillc;
		while (--diff >= 0)
			ofun(rfillc, arg);
	}
	if (fa->prefixlen && fa->fillc == ' ') {
		while (*fa->prefix != '\0')
			ofun(*fa->prefix++, arg);
	}
	if (lzero > 0) {
		rfillc = '0';
		while (--lzero >= 0)
			ofun(rfillc, arg);
	}
	while (*s != '\0')
		ofun(*s++, arg);
	if (fa->minusflag) {
		rfillc = ' ';
		while (--diff >= 0)
			ofun(rfillc, arg);
	}
	return (count);
}

/*
 * Print out one char, allowing prc('\0')
 * Similar to prbuf()
 */
#ifdef	PROTOTYPES

LOCAL int
prc(char c, f_args *fa)

#else

LOCAL int
prc(c, fa)
	char	c;
	f_args *fa;
#endif
{
	register int diff;
	register int rfillc;
	register void *arg				= fa->farg;
#ifdef	FORMAT_FUNC_PARM
	register void (*fun) __PR((char, void *))	= fa->outf;
#endif
	register int count;

	count = 1;
	diff = fa->fldwidth - 1;
	if (diff > 0)
		count += diff;

	if (!fa->minusflag) {
		rfillc = fa->fillc;
		while (--diff >= 0)
			ofun(rfillc, arg);
	}
	ofun(c, arg);
	if (fa->minusflag) {
		rfillc = ' ';
		while (--diff >= 0)
			ofun(rfillc, arg);
	}
	return (count);
}

/*
 * String output routine.
 * If fa->signific is >= 0, it uses only fa->signific chars.
 * If fa->signific is 0, print no characters.
 */
LOCAL int
prstring(s, fa)
	register const char	*s;
	f_args *fa;
{
	register char	*bp;
	register int	signific;

	if (s == NULL)
		return (prbuf("(NULL POINTER)", fa));

	if (fa->signific < 0)
		return (prbuf(s, fa));

	bp = fa->buf;
	signific = fa->signific;

	while (--signific >= 0 && *s != '\0')
		*bp++ = *s++;
	*bp = '\0';

	return (prbuf(fa->buf, fa));
}

#ifdef	DEBUG
LOCAL void
dbg_print(fmt, a, b, c, d, e, f, g, h, i)
char *fmt;
{
	char	ff[1024];

	sprintf(ff, fmt, a, b, c, d, e, f, g, h, i);
	write(STDERR_FILENO, ff, strlen(ff));
}
#endif

#ifdef	USE_NL_ARGS
#ifdef	FORMAT_IMPL
/*
 * The following code is shared between format() and fprformat().
 */

/*
 * Format argument types.
 * As "char" and "short" type arguments are fetched as "int"
 * we start with size "int" and ignore the 'h' modifier when
 * parsing sizes.
 */
#define	AT_NONE			0
#define	AT_INT			1
#define	AT_LONG			2
#define	AT_LONG_LONG		3
#define	AT_DOUBLE		4
#define	AT_LONG_DOUBLE		5
#define	AT_VOID_PTR		6
#define	AT_CHAR_PTR		7
#define	AT_SHORT_PTR		8
#define	AT_INT_PTR		9
#define	AT_LONG_PTR		10
#define	AT_LONG_LONG_PTR	11
#define	AT_R_FMT		12
#define	AT_R_VA_LIST		13
#define	AT_BOUNDS		14

#define	AF_NONE			0
#define	AF_LONG			1
#define	AF_LONG_LONG		2
#define	AF_LONG_DOUBLE		4
#define	AF_STAR			8

static	const char	skips[] = "+- #'.$h1234567890";
static	const char	*digits = &skips[8];

/*
 * Parse the format string and store the first FMT_ARGMAX args in the arglist
 * parameter.
 *
 * This is done in two stages:
 *	1	parse the format string and store the types in argtypes[].
 *	2	use the type list in argtypes[], fetch the args in order and
 *		store the related va_list state in arglist[]
 */
EXPORT void
_fmtarglist(fmt, fargs, arglist)
	const char	*fmt;
	va_lists_t	fargs;
	va_lists_t	arglist[];
{
	int	i;
	int	argindex;
	int	maxindex;
	int	thistype;
	int	thisflag;
	int	argtypes[FMT_ARGMAX+1];

	for (i = 0; i < FMT_ARGMAX; i++)
		argtypes[i] = AT_NONE;

	maxindex = -1;
	argindex = 0;
	while ((fmt = strchr(fmt, '%')) != NULL) {
		fmt++;
		i = strspn(fmt, digits);
		if (fmt[i] == '$') {
			int	c;

			argindex = *fmt++ - '0';
			while (c = *fmt, is_dig(c)) {
				argindex *= 10;
				argindex += c - '0';
				fmt++;
			}
			argindex -= 1;
		}
		thistype = AT_NONE;
		thisflag = AF_NONE;
	newarg:
		fmt += strspn(fmt, skips);
		switch (*fmt++) {

		case '%':		/* %% format no arg */
			continue;

		case 'l':
			if (thisflag & AF_LONG) {
				thisflag |= AF_LONG_LONG;
			} else {
				thisflag |= AF_LONG;
			}
			goto newarg;
		case 'j':		/* intmax_t for now is long long */
			thisflag |= AF_LONG_LONG;
			goto newarg;
		case 'z':		/* size_t */
#if	SIZEOF_SIZE_T == SIZEOF_INT
			if (thistype == AT_NONE)
				thistype = AT_INT;
#else
#if	SIZEOF_SIZE_T == SIZEOF_LONG_INT
			if (thistype == AT_NONE)
				thistype = AT_LONG;
#else
#if	SIZEOF_SIZE_T == SIZEOF_LLONG
			if (thistype == AT_NONE)
				thistype = AT_LONG_LONG;
#else
error sizeof (size_t) is unknown
#endif
#endif
#endif
			goto newarg;
		case 't':		/* ptrdiff_t */
#if	SIZEOF_PTRDIFF_T == SIZEOF_INT
			if (thistype == AT_NONE)
				thistype = AT_INT;
#else
#if	SIZEOF_PTRDIFF_T == SIZEOF_LONG_INT
			if (thistype == AT_NONE)
				thistype = AT_LONG;
#else
#if	SIZEOF_PTRDIFF_T == SIZEOF_LLONG
			if (thistype == AT_NONE)
				thistype = AT_LONG_LONG;
#else
error sizeof (ptrdiff_t) is unknown
#endif
#endif
#endif
			goto newarg;
#ifndef	NO_UCSIL
			/*
			 * Enhancements to K&R and ANSI:
			 *
			 * got a type specifyer
			 *
			 * XXX 'S' in C99 is %ls, 'S' should become 'H'
			 */
		case 'U':
			if (!strchr("CSILZODX", *fmt)) {
				/*
				 * Got only 'U'nsigned specifyer,
				 * use default type and mode.
				 */
				thistype = AT_INT;
				break;
			}
			if (!strchr("CSIL", *fmt)) {
				/*
				 * Got 'U' and ZODX.
				 * no type, use default
				 */
				thistype = AT_INT;
				fmt++;	/* Skip ZODX */
				break;
			}
			fmt++;		/* Unsigned, skip 'U', get CSIL */
			/* FALLTHRU */
		case 'C':
		case 'S':
		case 'I':
		case 'L':
			fmt--;		/* Undo fmt++ from switch() */
			{
				/*
				 * got CSIL type
				 */
				int	type = *fmt++;	/* Undo above fmt-- */
				int	mode = *fmt;
				if (!strchr("ZODX", mode)) {
					/*
					 * Check long double "Le", "Lf" or "Lg"
					 */
					if (type == 'L' &&
					    (mode == 'e' ||
					    mode == 'f' ||
					    mode == 'g')) {
						thisflag |= AF_LONG_DOUBLE;
						goto newarg;
					}
				} else {
					fmt++;	/* Skip ZODX */
				}
				if (type == 'L')
					thistype = AT_LONG;
				else
					thistype = AT_INT;
			}
			break;
#else
		case 'L':
			thisflag |= AF_LONG_DOUBLE;
			goto newarg;
#endif

		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			if (thisflag & AF_LONG_DOUBLE)
				thistype = AT_LONG_DOUBLE;
			else
				thistype = AT_DOUBLE;
			break;

		case 'p':
			thistype = AT_VOID_PTR;
			break;
		case 's':
			thistype = AT_CHAR_PTR;
			break;
		case 'b':
			thistype = AT_BOUNDS;
			break;
		case 'n':
			if (thisflag & AF_LONG_LONG)
				thistype = AT_LONG_LONG_PTR;
			else if (thistype & AF_LONG)
				thistype = AT_LONG_PTR;
			else
				thistype = AT_INT_PTR;
			break;
		case 'r':
			thistype = AT_R_FMT;
			break;
		default:
			if (thistype == AT_NONE) {
				if (thisflag & AF_LONG_LONG)
					thistype = AT_LONG_LONG;
				else if (thistype & AF_LONG)
					thistype = AT_LONG;
				else
					thistype = AT_INT;
			}
			break;

		case '*':
			if (is_dig(*fmt)) {
				int	c;
				int	starindex;

				starindex = *fmt++ - '0';
				while (c = *fmt, is_dig(c)) {
					starindex *= 10;
					starindex += c - '0';
					fmt++;
				}
				starindex -= 1;
				if (starindex >= 0 && starindex < FMT_ARGMAX) {
					argtypes[starindex] = AT_INT;
					if (starindex > maxindex)
						maxindex = starindex;
				}
				goto newarg;
			}
			thistype = AT_INT;
			thisflag |= AF_STAR; /* Make sure to rescan for type */
			break;
		}
		if (argindex >= 0 && argindex < FMT_ARGMAX) {
			argtypes[argindex] = thistype;
			if (thistype == AT_R_FMT)
				argtypes[++argindex] = AT_R_VA_LIST;
			else if (thistype == AT_BOUNDS)
				argtypes[++argindex] = AT_INT;

			if (argindex > maxindex)
				maxindex = argindex;
		}
		++argindex;		/* Default to next arg in list */
		if (thisflag & AF_STAR) { /* Found '*', continue for type */
			thisflag &= ~AF_STAR;
			goto newarg;
		}
	}

	for (i = 0; i <= maxindex; i++) { /* Do not fetch more args than known */
		arglist[i] = fargs;	/* Save state before fetching this */

		switch (argtypes[i]) {

		default:
			/* FALLTHRU */
		case AT_NONE:		/* This matches '*' args */
			/* FALLTHRU */
		case AT_INT:
			(void) va_arg(fargs.ap, int);
			break;
		case AT_LONG:
			(void) va_arg(fargs.ap, long);
			break;
		case AT_LONG_LONG:
			(void) va_arg(fargs.ap, Llong);
			break;
		case AT_DOUBLE:
			(void) va_arg(fargs.ap, double);
			break;
		case AT_LONG_DOUBLE:
#ifdef	HAVE_LONGDOUBLE
			(void) va_arg(fargs.ap, long double);
#endif
			break;
		case AT_VOID_PTR:
			(void) va_arg(fargs.ap, void *);
			break;
		case AT_CHAR_PTR:
			(void) va_arg(fargs.ap, char *);
			break;
		case AT_SHORT_PTR:
			(void) va_arg(fargs.ap, short *);
			break;
		case AT_INT_PTR:
			(void) va_arg(fargs.ap, int *);
			break;
		case AT_LONG_PTR:
			(void) va_arg(fargs.ap, long *);
			break;
		case AT_LONG_LONG_PTR:
			(void) va_arg(fargs.ap, Llong *);
			break;
		case AT_R_FMT:
			(void) va_arg(fargs.ap, char *);
			arglist[++i] = fargs;
			(void) __va_arg_list(fargs.ap);
			break;
		case AT_R_VA_LIST:
			break;
		case AT_BOUNDS:
			(void) va_arg(fargs.ap, char *);
			arglist[++i] = fargs;
			(void) va_arg(fargs.ap, int);
			break;
		}
	}
}

/*
 * In case that the format references an argument > FMT_ARGMAX, we use this
 * implementation. It is slow (n*n - where n is (argno - FMT_ARGMAX)).
 * Fortunately, it is most unlikely that there are more positional args than
 * the current FMT_ARGMAX definition of 30.
 */
EXPORT void
_fmtgetarg(fmt, num, fargs)
	const char	*fmt;
	int		num;
	va_lists_t	*fargs;
{
	const char	*sfmt = fmt;
	int		i;

	/*
	 * Hacky preliminary support for all int type args bejond FMT_ARGMAX.
	 */
	for (i = FMT_ARGMAX; i < num; i++)
		(void) va_arg((*fargs).ap, int);
}
#endif	/* FORMAT_IMPL */
#endif	/* USE_NL_ARGS */
