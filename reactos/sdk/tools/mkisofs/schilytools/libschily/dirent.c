/* @(#)dirent.c	1.4 17/02/02 Copyright 2011-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)dirent.c	1.4 17/02/02 Copyright 2011-2017 J. Schilling";
#endif
/*
 *	Copyright (c) 2011-2017 J. Schilling
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

#include <schily/dirent.h>
#include <schily/maxpath.h>
#include <schily/string.h>
#include <schily/errno.h>

#ifdef	NEED_READDIR

#if	defined(__MINGW32__) || defined(_MSC_VER)

#include <schily/windows.h>
#include <schily/utypes.h>
#include <schily/schily.h>

EXPORT	DIR		*opendir __PR((const char *));
EXPORT	int		closedir __PR((DIR *));
EXPORT	struct dirent	*readdir __PR((DIR *));

EXPORT DIR *
opendir(dname)
	const char	*dname;
{
	char	path[PATH_MAX];
	size_t	len;
	uint32_t attr;
	DIR	*dp;

	if (dname == NULL) {
		seterrno(EFAULT);
		return ((DIR *)0);
	}
	len = strlen(dname);
	if (len > PATH_MAX) {
		seterrno(ENAMETOOLONG);
		return ((DIR *)0);
	}
	if (len == 0) {
		seterrno(ENOENT);
		return ((DIR *)0);
	}

	attr = GetFileAttributes(dname);
	if (attr == INVALID_FILE_ATTRIBUTES ||
	    (attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		seterrno(ENOTDIR);
		return ((DIR *)0);
	}
	path[0] = '\0';
	_fullpath(path, dname, PATH_MAX);
	len = strlen(path);
	if (len == 0) {
		seterrno(ENOENT);
		return ((DIR *)0);
	}

	dp = malloc(sizeof (*dp) + len + 2);	/* Add 2 for "/ *" */
	if (dp == NULL) {
		seterrno(ENOMEM);
		return ((DIR *)0);
	}
	strcpy(dp->dd_dirname, path);
	if (dp->dd_dirname[len-1] != '/' &&
	    dp->dd_dirname[len-1] != '\\') {
		dp->dd_dirname[len] = '\\';
		len++;
	}
	dp->dd_dirname[len++] = '*';
	dp->dd_dirname[len]   = '\0';
	dp->dd_handle = -1;
	dp->dd_state  = 0;

	dp->dd_dir.d_ino    = 0;
	dp->dd_dir.d_reclen = 0;
	dp->dd_dir.d_namlen = 0;
	zerobytes(dp->dd_dir.d_name, sizeof (dp->dd_dir.d_name));

	return (dp);
}

EXPORT	int
closedir(dp)
	DIR	*dp;
{
	int	ret = 0;

	if (dp == NULL) {
		seterrno(EFAULT);
		return (-1);
	}
	if (dp->dd_handle != -1) {
		ret = _findclose(dp->dd_handle);
	}
	free(dp);

	return (ret);
}

EXPORT struct dirent *
readdir(dp)
	DIR	*dp;
{
	if (dp == NULL) {
		seterrno(EFAULT);
		return ((struct dirent *)0);
	}
	if (dp->dd_state == (char)-1) {
		return ((struct dirent *)0);
	} else if (dp->dd_state == (char)0) {
		dp->dd_handle = _findfirst(dp->dd_dirname, &(dp->dd_data));
		if (dp->dd_handle != -1)
			dp->dd_state = 1;
		else
			dp->dd_state = -1;
	} else {
		if (_findnext(dp->dd_handle, &(dp->dd_data))) {
			uint32_t	werrno = GetLastError();

			if (werrno == ERROR_NO_MORE_FILES)
				seterrno(0);
			_findclose(dp->dd_handle);
			dp->dd_handle = -1;
			dp->dd_state = -1;
		} else {
			dp->dd_state = 1;	/* state++ to support seekdir */
		}
	}
	if (dp->dd_state > 0) {
		strlcpy(dp->dd_dir.d_name, dp->dd_data.name,
				sizeof (dp->dd_dir.d_name));

		return (&dp->dd_dir);
	}
	return ((struct dirent *)0);
}

#endif	/* defined(__MINGW32__) || defined(_MSC_VER) */

#endif	/* NEED_READDIR */
