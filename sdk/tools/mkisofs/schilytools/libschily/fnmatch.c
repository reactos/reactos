/* @(#)fnmatch.c	8.24 17/08/30 2005-2017 J. Schilling from 8.2 (Berkeley) */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fnmatch.c	8.24 17/08/30 2005-2017 J. Schilling from 8.2 (Berkeley)";
#endif
/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Guido van Rossum.
 *
 * Copyright (c) 2005-2017 J. Schilling
 * Copyright (c) 2011 The FreeBSD Foundation
 * All rights reserved.
 * Portions of this software were developed by David Chisnall
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static UConst char sccsid[] = "@(#)fnmatch.c	8.24 (Berkeley) 08/30/17";
#endif /* LIBC_SCCS and not lint */
/* "FBSD src/lib/libc/gen/fnmatch.c,v 1.19 2010/04/16 22:29:24 jilles Exp $" */

/*
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a filename or pathname to a pattern.
 */

/*
 * Some notes on multibyte character support:
 * 1. Patterns with illegal byte sequences match nothing.
 * 2. Illegal byte sequences in the "string" argument are handled by treating
 *    them as single-byte characters with a value of the first byte of the
 *    sequence cast to wchar_t.
 * 3. Multibyte conversion state objects (mbstate_t) are passed around and
 *    used for most, but not all, conversions. Further work will be required
 *    to support state-dependent encodings.
 */

#include <schily/mconfig.h>
#include <schily/fnmatch.h>
#include <schily/limits.h>
#include <schily/string.h>
#include <schily/wchar.h>
#include <schily/wctype.h>
#include <schily/libport.h>	/* Define missing prototypes */

#define	EOS	'\0'

#define	RANGE_MATCH	1
#define	RANGE_NOMATCH	0
#define	RANGE_ERROR	(-1)

#define	CL_SIZE		32	/* Max size for '[: :]'			*/

static int rangematch __PR((const char *, wchar_t, int, char **, mbstate_t *));
static int fnmatch1 __PR((const char *, const char *, const char *, int,
				mbstate_t, mbstate_t));

#ifndef	HAVE_FNMATCH
#undef	fnmatch

/*
 * The Cygwin compile environment incorrectly implements #pragma weak.
 * The weak symbols are only defined as local symbols making it impossible
 * to use them from outside the scope of this source file.
 * A platform that allows linking with global symbols has HAVE_LINK_WEAK
 * defined.
 */
#if defined(HAVE_PRAGMA_WEAK) && defined(HAVE_LINK_WEAK)
#pragma	weak fnmatch =	js_fnmatch
#else
int
fnmatch(pattern, string, flags)
	const char	*pattern;
	const char	*string;
	int		flags;
{
	return (js_fnmatch(pattern, string, flags));
}
#endif
#endif

int
js_fnmatch(pattern, string, flags)
	const char	*pattern;
	const char	*string;
	int		flags;
{
	/*
	 * SunPro C gives a warning if we do not initialize an object:
	 * static const mbstate_t initial;
	 * GCC gives a warning if we try to initialize it.
	 * As the POSIX standard forbids mbstate_t from being an array,
	 * we do not need "const", the var is always copied when used as
	 * a parapemeter for fnmatch1();
	 */
	static mbstate_t initial;

	return (fnmatch1(pattern, string, string, flags, initial, initial));
}

static int
fnmatch1(pattern, string, stringstart, flags, patmbs, strmbs)
	const char	*pattern;
	const char	*string;
	const char	*stringstart;
	int		flags;
	mbstate_t	patmbs;
	mbstate_t	strmbs;
{
	const char *bt_pattern, *bt_string;
	mbstate_t bt_patmbs, bt_strmbs;
	char *newp;
	char c;
	wchar_t pc, sc;
	size_t pclen, sclen;

	bt_pattern = bt_string = NULL;
	for (;;) {
		pclen = mbrtowc(&pc, pattern, MB_LEN_MAX, &patmbs);
		if (pclen == (size_t)-1 || pclen == (size_t)-2)
			return (FNM_NOMATCH);
		pattern += pclen;
		sclen = mbrtowc(&sc, string, MB_LEN_MAX, &strmbs);
		if (sclen == (size_t)-1 || sclen == (size_t)-2) {
			sc = (unsigned char)*string;
			sclen = 1;
			memset(&strmbs, 0, sizeof (strmbs));
		}
		switch (pc) {
		case EOS:
			if ((flags & FNM_LEADING_DIR) && sc == '/')
				return (0);
			if (sc == EOS)
				return (0);
			goto backtrack;
		case '?':
			if (sc == EOS)
				return (FNM_NOMATCH);
			if (sc == '/' && (flags & FNM_PATHNAME))
				goto backtrack;
			if (sc == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				goto backtrack;
			string += sclen;
			break;
		case '*':
			c = *pattern;
			/* Collapse multiple stars. */
			while (c == '*')
				c = *++pattern;

			if (sc == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				goto backtrack;

			/* Optimize for pattern with * at end or before /. */
			if (c == EOS) {
				if (flags & FNM_PATHNAME)
					return ((flags & FNM_LEADING_DIR) ||
					    strchr(string, '/') == NULL ?
					    0 : FNM_NOMATCH);
				else
					return (0);
			} else if (c == '/' && flags & FNM_PATHNAME) {
				if ((string = strchr(string, '/')) == NULL)
					return (FNM_NOMATCH);
				break;
			}

			/*
			 * First try the shortest match for the '*' that
			 * could work. We can forget any earlier '*' since
			 * there is no way having it match more characters
			 * can help us, given that we are already here.
			 */
			bt_pattern = pattern, bt_patmbs = patmbs;
			bt_string = string, bt_strmbs = strmbs;
			break;
		case '[':
			if (sc == EOS)
				return (FNM_NOMATCH);
			if (sc == '/' && (flags & FNM_PATHNAME))
				goto backtrack;
			if (sc == '.' && (flags & FNM_PERIOD) &&
			    (string == stringstart ||
			    ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
				goto backtrack;

			switch (rangematch(pattern, sc, flags, &newp,
			    &patmbs)) {
			case RANGE_ERROR:
				goto norm;
			case RANGE_MATCH:
				pattern = newp;
				break;
			case RANGE_NOMATCH:
				goto backtrack;
			}
			string += sclen;
			break;
		case '\\':
			if (!(flags & FNM_NOESCAPE)) {
				pclen = mbrtowc(&pc, pattern, MB_LEN_MAX,
				    &patmbs);
				if (pclen == (size_t)-1 || pclen == (size_t)-2)
					return (FNM_NOMATCH);
				if (pclen == 0)
					return (FNM_NOMATCH);
				pattern += pclen;
			}
			/* FALLTHROUGH */
		default:
		norm:
			string += sclen;
			if (pc == sc) {
				;
			} else if ((flags & FNM_CASEFOLD) &&
				    (towlower(pc) == towlower(sc))) {
				;
			} else {
		backtrack:
				/*
				 * If we have a mismatch (other than hitting
				 * the end of the string), go back to the last
				 * '*' seen and have it match one additional
				 * character.
				 */
				if (bt_pattern == NULL)
					return (FNM_NOMATCH);
				sclen = mbrtowc(&sc, bt_string, MB_LEN_MAX,
				    &bt_strmbs);
				if (sclen == (size_t)-1 ||
				    sclen == (size_t)-2) {
					sc = (unsigned char)*bt_string;
					sclen = 1;
					memset(&bt_strmbs, 0,
					    sizeof (bt_strmbs));
				}
				if (sc == EOS)
					return (FNM_NOMATCH);
				if (sc == '/' && flags & FNM_PATHNAME)
					return (FNM_NOMATCH);
				bt_string += sclen;
				pattern = bt_pattern, patmbs = bt_patmbs;
				string = bt_string, strmbs = bt_strmbs;
			}
			break;
		}
	}
	/* NOTREACHED */
}

#ifdef	PROTOTYPES
static int
rangematch(const char *pattern, wchar_t test, int flags, char **newp,
	    mbstate_t *patmbs)
#else
static int
rangematch(pattern, test, flags, newp, patmbs)
	const char *pattern;
	wchar_t test;
	int flags;
	char **newp;
	mbstate_t *patmbs;
#endif
{
	int negate, ok;
	wchar_t c, c2;
	wchar_t	otest = test;
	size_t pclen;
	const char *origpat;
#ifdef	XXX_COLLATE
	struct xlocale_collate *table = (struct xlocale_collate *)
				    __get_locale()->components[XLC_COLLATE];
#endif

	/*
	 * A bracket expression starting with an unquoted circumflex
	 * character produces unspecified results (IEEE 1003.2-1992,
	 * 3.13.2).  This implementation treats it like '!', for
	 * consistency with the regular expression syntax.
	 * J.T. Conklin (conklin@ngai.kaleida.com)
	 */
	if ((negate = (*pattern == '!' || *pattern == '^')))
		++pattern;

	if (flags & FNM_CASEFOLD)
		test = towlower(test);

	/*
	 * A right bracket shall lose its special meaning and represent
	 * itself in a bracket expression if it occurs first in the list.
	 * -- POSIX.2 2.8.3.2
	 */
	ok = 0;
	origpat = pattern;
	for (;;) {
		int	quoted = 0;

		if (*pattern == ']' && pattern > origpat) {
			pattern++;
			break;
		} else if (*pattern == '\0') {
			return (RANGE_ERROR);
		} else if (*pattern == '/' && (flags & FNM_PATHNAME)) {
			return (RANGE_NOMATCH);
		} else if (*pattern == '\\' && !(flags & FNM_NOESCAPE)) {
			pattern++;
			quoted++;
		}
		pclen = mbrtowc(&c, pattern, MB_LEN_MAX, patmbs);
		if (pclen == (size_t)-1 || pclen == (size_t)-2)
			return (RANGE_NOMATCH);
		pattern += pclen;

		if (!quoted && c == '[') {
			if (pattern[0] == ':') {
				char	class[CL_SIZE+1];
				char	*pc = class;
				const char	*p;

				p = pattern + 1;	/* Eat ':' */
				for (;;) {
					if (*p == '\0')
						return (RANGE_ERROR);
					if (*p == ':' && p[1] == ']')
						break;
					if (pc >= &class[CL_SIZE])
						return (RANGE_ERROR);
					*pc++ = *p++;
				}
				if (pc == class)
					return (RANGE_ERROR);
				*pc = '\0';
				pattern = p + 2;	/* Skip ":]" */
				if (iswctype(otest, wctype(class))) {
					ok = 1;
				} else if (flags & FNM_CASEFOLD) {
					/*
					 * Convert to the other case
					 */
					if (strcmp(class, "upper") == 0)
						if (iswctype(otest,
						    wctype("lower")))
							ok = 1;
					else if (strcmp(class, "lower") == 0)
						if (iswctype(otest,
						    wctype("upper")))
							ok = 1;
				}
				continue;
			}
		}

		if (flags & FNM_CASEFOLD)
			c = towlower(c);

		if (*pattern == '-' && *(pattern + 1) != EOS &&
		    *(pattern + 1) != ']') {
			if (*++pattern == '\\' && !(flags & FNM_NOESCAPE))
				if (*pattern != EOS)
					pattern++;
			pclen = mbrtowc(&c2, pattern, MB_LEN_MAX, patmbs);
			if (pclen == (size_t)-1 || pclen == (size_t)-2)
				return (RANGE_NOMATCH);
			pattern += pclen;
			if (c2 == EOS)
				return (RANGE_ERROR);

			if (flags & FNM_CASEFOLD)
				c2 = towlower(c2);

#ifdef	XXX_COLLATE
			if (table->__collate_load_error ?
			    c <= test && test <= c2 :
			    __wcollate_range_cmp(c, test) <= 0 &&
			    __wcollate_range_cmp(test, c2) <= 0)
				ok = 1;
#else
			if (c <= test && test <= c2)
				ok = 1;
#endif
		} else if (c == test)
			ok = 1;
	}

	*newp = (char *)pattern;
	return (ok == negate ? RANGE_NOMATCH : RANGE_MATCH);
}
