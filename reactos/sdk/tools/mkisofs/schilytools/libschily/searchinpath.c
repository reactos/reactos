/* @(#)searchinpath.c	1.5 16/08/01 Copyright 1999-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)searchinpath.c	1.5 16/08/01 Copyright 1999-2016 J. Schilling";
#endif
/*
 *	Search a file name in the PATH of the current exeecutable.
 *	Return the path to the file name in allocated space.
 *
 *	Copyright (c) 1999-2016 J. Schilling
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
#include <schily/string.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>		/* getexecname() */
#include <schily/stat.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/errno.h>


#define	NAMEMAX	4096

EXPORT	char	*searchfileinpath	__PR((char *name, int mode,
							int file_mode,
							char *path));
LOCAL	char	*searchonefile		__PR((char *name, int mode,
							BOOL plain_file,
							char *xn,
							char *nbuf,
							char *np, char *ep));
#if defined(__DJGPP__)
LOCAL	char 	*strbs2s		__PR((char *s));
#endif

#ifdef	JOS
#define	enofile(t)			((t) == EMISSDIR || \
					(t)  == ENOFILE || \
					(t)  == EISADIR || \
					(t)  == EIOERR)
#else
#define	enofile(t)			((t) == ENOENT || \
					(t)  == ENOTDIR || \
					(t)  == EISDIR || \
					(t)  == EIO)
#endif

/*
 * Search for the "name" file in the PATH of the user.
 * Assume that the file is ... bin/../name.
 */
EXPORT char *
searchfileinpath(name, mode, file_mode, path)
	char	*name;			/* Find <execname>/../name in PATH	*/
	int	mode;			/* Mode for access() e.g. X_OK		*/
	int	file_mode;		/* How to check files			*/
	char	*path;			/* PATH to use if not NULL		*/
{
	char	pbuf[NAMEMAX];
	char	*nbuf = pbuf;
	char	*np;
	char	*ep;
	char	*xn;
	int	nlen = strlen(name);
	int	oerrno = geterrno();
	int	err = 0;
#ifdef	HAVE_GETEXECNAME
	char	*pn = (char *)getexecname();	/* pn is on the stack */
#else
	char	*pn = getexecpath();		/* pn is from strdup() */
	char	ebuf[NAMEMAX];

	if (pn) {
		strlcpy(ebuf, pn, sizeof (ebuf));
		free(pn);
		pn = ebuf;
	}
#endif

	if (pn == NULL)
		xn = get_progname();
	else
		xn = pn;
	if ((np = strrchr(xn, '/')) != NULL)
		xn = ++np;

	/*
	 * getexecname() is the best choice for our seach. getexecname()
	 * returns either "foo" (if called from the current directory) or
	 * an absolute path after symlinks have been resolved.
	 * If getexecname() returns a path with slashes, try to search
	 * first relatively to the known location of the current binary.
	 */
	if ((file_mode & SIP_ONLY_PATH) == 0 &&
		pn != NULL && strchr(pn, '/') != NULL) {
		strlcpy(nbuf, pn, sizeof (pbuf));
		np = nbuf + strlen(nbuf);

		while (np > nbuf && np[-1] != '/')
			*--np = '\0';
		pn = &nbuf[sizeof (pbuf) - 1];
		if ((np = searchonefile(name, mode,
				file_mode & (SIP_PLAIN_FILE|SIP_NO_STRIPBIN),
				xn,
				nbuf, np, pn)) != NULL) {
			seterrno(oerrno);
			return (np);
		}
	}

	if (file_mode & SIP_NO_PATH)
		return (NULL);

	if (path == NULL)
		path = getenv("PATH");
	if (path == NULL)
		return (NULL);


#ifdef __DJGPP__
	path = strdup(path);
	if (path == NULL)
		return (NULL);
	strbs2s(path);	/* PATH under DJGPP can contain both slashes */
#endif

	/*
	 * A PATH name search should lead us to the path under which we
	 * called the binary, but not necessarily to the install path as
	 * we may have been called thorugh a symlink or hardlink. In case
	 * of a symlink, we can follow the link. In case of a hardlink, we
	 * are lost.
	 */
	ep = &nbuf[sizeof (pbuf) - 1];
	for (;;) {
		np = nbuf;
		while (*path != PATH_ENV_DELIM && *path != '\0' &&
		    np < &nbuf[sizeof (pbuf) - nlen])
				*np++ = *path++;
		*np = '\0';
		if ((np = searchonefile(name, mode,
				file_mode & (SIP_PLAIN_FILE|SIP_NO_STRIPBIN),
				xn,
				nbuf, np, ep)) != NULL) {
#ifdef __DJGPP__
			free(path);
#endif
			seterrno(oerrno);
			return (np);
		}
		if (err == 0) {
			err = geterrno();
			if (enofile(err))
				err = 0;
		}
		if (*path == '\0')
			break;
		path++;
	}
#ifdef __DJGPP__
	free(path);
#endif
	if (err)
		seterrno(err);
	else
		seterrno(oerrno);
	return (NULL);
}

LOCAL char *
searchonefile(name, mode, plain_file, xn, nbuf, np, ep)
	register char	*name;		/* Find <execname>/../name in PATH	*/
		int	mode;		/* Mode for access() e.g. X_OK		*/
		BOOL	plain_file;	/* Whether to check only plain files	*/
		char	*xn;		/* The basename of the executable	*/
	register char	*nbuf;		/* Name buffer base			*/
	register char	*np;		/* Where to append name to path		*/
	register char	*ep;		/* Point to last valid char in nbuf	*/
{
	struct stat	sb;

	while (np > nbuf && np[-1] == '/')
		*--np = '\0';
	if (xn) {
		*np++ = '/';
		strlcpy(np, xn, ep - np);
		if (stat(nbuf, &sb) < 0)
			return (NULL);
		if (!S_ISREG(sb.st_mode))
			return (NULL);
		*--np = '\0';
	}
	if ((plain_file & SIP_NO_STRIPBIN) == 0) {
		if (np >= &nbuf[4] && streql(&np[-4], "/bin"))
			np = &np[-4];
	}
	plain_file &= SIP_PLAIN_FILE;
	*np++ = '/';
	*np   = '\0';
	strlcpy(np, name, ep - np);

	seterrno(0);
	if (stat(nbuf, &sb) >= 0) {
		if ((!plain_file || S_ISREG(sb.st_mode)) &&
			(eaccess(nbuf, mode) >= 0)) {
			return (strdup(nbuf));
		}
		if (geterrno() == 0)
			seterrno(EACCES);
	}
	return (NULL);
}

#ifdef __DJGPP__
LOCAL char *
strbs2s(s)
	char	*s;
{
	char	*tmp = s;

	if (tmp) {
		while (*tmp) {
			if (*tmp == '\\')
				*tmp = '/';
			tmp++;
		}
	}
	return (s);
}
#endif
