#if defined __REACTOS__ && defined(_MSC_VER) && _MSC_VER == 1700
#pragma message("Warning VS2012 extrawurst: wctype should come from MS CRT")
// due to bugs in libschily the VS2012 does fail to link wctype-functions
// used by match.c and fnmatch.c. But VS2010/VS2013 can properly link the MS CRT versions of it.
// Therefore we provide libschily implementation of wctype.c for VS2012.


/* @(#)wctype.c	1.3 17/08/13 Copyright 2017 J. Schilling */
/*
 *	Emulate the behavior of wctype() and iswctype()
 *
 *	Copyright (c) 2017 J. Schilling
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

#include <schily/ctype.h>
#include <schily/wctype.h>
#include <schily/wchar.h>
#include <schily/string.h>
#include <schily/schily.h>

#ifndef	HAVE_WCTYPE
LOCAL struct wct {
	char		*name;
	wctype_t	val;
} wct[] = {
	{"alnum", 1},
	{"alpha", 2},
	{"blank", 3},
	{"cntrl", 4},
	{"digit", 5},
	{"graph", 6},
	{"lower", 7},
	{"print", 8},
	{"punct", 9},
	{"space", 10},
	{"upper", 11},
	{"xdigit", 12},
	{ NULL, 0}
};

wctype_t
wctype(n)
	const char	*n;
{
	register struct wct *wp = wct;

	for (; wp->name; wp++) {
		if (*n != *wp->name)
			continue;
		if (strcmp(n, wp->name) == 0)
			return (wp->val);
	}
	return (0);
}

int
iswctype(wc, t)
	wint_t		wc;
	wctype_t	t;
{
	switch (t) {

	case 1: return (iswalnum(wc));
	case 2: return (iswalpha(wc));
#if defined(HAVE_ISWBLANK) || ((MB_LEN_MAX == 1) && defined(HAVE_ISBLANK))
	case 3: return (iswblank(wc));
#else
	case 3: return (isspace(wc));
#endif
	case 4: return (iswcntrl(wc));
	case 5: return (iswdigit(wc));
	case 6: return (iswgraph(wc));
	case 7: return (iswlower(wc));
	case 8: return (iswprint(wc));
	case 9: return (iswpunct(wc));
	case 10: return (iswspace(wc));
	case 11: return (iswupper(wc));
	case 12: return (iswxdigit(wc));

	default:
		return (0);
	}
}
#endif	/* HAVE_WCTYPE */
#endif //__REACTOS__
