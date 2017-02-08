/* @(#)match.c	1.34 16/11/27 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)match.c	1.34 16/11/27 joerg";
#endif
/*
 * 27-Mar-96: Jan-Piet Mens <jpm@mens.de>
 * added 'match' option (-m) to specify regular expressions NOT to be included
 * in the CD image.
 *
 * Re-written 13-Apr-2000 James Pearson
 * now uses a generic set of routines
 * Conversions to make the code more portable May 2000 .. March 2004
 * Copyright (c) 2000-2016 J. Schilling
 */

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/libport.h>
#include <schily/nlsdefs.h>
#include "match.h"

struct match {
	struct match *next;
	char	 *name;
};

typedef struct match match;

static BOOL	isort;

static match *mats[MAX_MAT];

static char *mesg[MAX_MAT] = {
	"excluded",
	"excluded ISO-9660",
	"excluded Joliet",
	"excluded UDF",
	"hidden attribute ISO-9660",
#ifdef APPLE_HYB
	"excluded HFS",
#endif /* APPLE_HYB */
};

#ifdef SORTING
struct sort_match {
	struct sort_match	*next;
	char			*name;
	int			val;
};

typedef struct sort_match sort_match;

static sort_match	*s_mats;

EXPORT int
add_sort_match(fn, val)
	char	*fn;
	int	val;
{
	sort_match *s_mat;

	s_mat = (sort_match *)malloc(sizeof (sort_match));
	if (s_mat == NULL) {
		errmsg(_("Can't allocate memory for sort filename\n"));
		return (0);
	}

	if ((s_mat->name = strdup(fn)) == NULL) {
		errmsg(_("Can't allocate memory for sort filename\n"));
		free(s_mat);
		return (0);
	}

	/* need to reserve the minimum value for other uses */
	if (val == NOT_SORTED)
		val++;

	s_mat->val = val;
	s_mat->next = s_mats;
	s_mats = s_mat;

	return (1);
}

EXPORT int
add_sort_list(file, valp, pac, pav, opt)
	const char	*file;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	*opt;
{
	FILE	*fp;
	char	name[4096];
	char	*p;
	int	val;
extern	int	do_sort;

	while (*opt == '-')
		opt++;
	if (*opt == 'i')
		isort = TRUE;
	do_sort++;
	if ((fp = fopen(file, "r")) == NULL) {
		comerr(_("Can't open sort file list %s\n"), file);
	}

	while (fgets(name, sizeof (name), fp) != NULL) {
		/*
		 * look for the last space or tab character
		 */
		if ((p = strrchr(name, ' ')) == NULL)
			p = strrchr(name, '\t');
		else if (strrchr(p, '\t') != NULL)	/* Tab after space? */
			p = strrchr(p, '\t');

		if (p == NULL) {
			/*
			 * XXX old code did not abort here.
			 */
			comerrno(EX_BAD, _("Incorrect sort file format\n\t%s\n"), name);
			continue;
		} else {
			*p = '\0';
			val = atoi(++p);
		}
		if (!add_sort_match(name, val)) {
			fclose(fp);
			return (-1);
		}
	}

	fclose(fp);
	return (1);
}

EXPORT int
sort_matches(fn, val)
	char	*fn;
	int	val;
{
	register sort_match	*s_mat;
		int		flags = FNM_PATHNAME;

	if (isort)
		flags |= FNM_IGNORECASE;

	for (s_mat = s_mats; s_mat; s_mat = s_mat->next) {
		if (fnmatch(s_mat->name, fn, flags) != FNM_NOMATCH) {
			return (s_mat->val); /* found sort value */
		}
	}
	return (val); /* not found - default sort value */
}

EXPORT void
del_sort()
{
	register sort_match * s_mat, *s_mat1;

	s_mat = s_mats;
	while (s_mat) {
		s_mat1 = s_mat->next;

		free(s_mat->name);
		free(s_mat);

		s_mat = s_mat1;
	}

	s_mats = 0;
}

#endif /* SORTING */


EXPORT int
gen_add_match(fn, n)
	char	*fn;
	int	n;
{
	match	*mat;

	if (n >= MAX_MAT) {
		errmsgno(EX_BAD, _("Too many patterns.\n"));
		return (0);
	}

	mat = (match *)malloc(sizeof (match));
	if (mat == NULL) {
		errmsg(_("Can't allocate memory for %s filename\n"), mesg[n]);
		return (0);
	}

	if ((mat->name = strdup(fn)) == NULL) {
		errmsg(_("Can't allocate memory for %s filename\n"), mesg[n]);
		free(mat);
		return (0);
	}

	mat->next = mats[n];
	mats[n] = mat;

	return (1);
}

EXPORT int
add_match(fn)
	char	*fn;
{
	int	ret = gen_add_match(fn, EXCLUDE);

	if (ret == 0)
		return (-1);
	return (1);
}

EXPORT int
i_add_match(fn)
	char	*fn;
{
	int	ret = gen_add_match(fn, I_HIDE);

	if (ret == 0)
		return (-1);
	return (1);
}

EXPORT int
h_add_match(fn)
	char	*fn;
{
	int	ret = gen_add_match(fn, H_HIDE);

	if (ret == 0)
		return (-1);
	return (1);
}

#ifdef	APPLE_HYB
EXPORT int
hfs_add_match(fn)
	char	*fn;
{
	int	ret = gen_add_match(fn, HFS_HIDE);

	if (ret == 0)
		return (-1);
	return (1);
}
#endif	/* APPLE_HYB */

EXPORT int
j_add_match(fn)
	char	*fn;
{
	int	ret = gen_add_match(fn, J_HIDE);

	if (ret == 0)
		return (-1);
	return (1);
}

EXPORT int
u_add_match(fn)
	char	*fn;
{
	int	ret = gen_add_match(fn, U_HIDE);

	if (ret == 0)
		return (-1);
	return (1);
}

EXPORT void
gen_add_list(file, n)
	char	*file;
	int	n;
{
	FILE	*fp;
	char	name[4096];
	int	len;

	if ((fp = fopen(file, "r")) == NULL) {
		comerr(_("Can't open %s file list %s\n"), mesg[n], file);
	}

	while (fgets(name, sizeof (name), fp) != NULL) {
		/*
		 * strip of '\n'
		 */
		len = strlen(name);
		if (name[len - 1] == '\n') {
			name[len - 1] = '\0';
		}
		if (!gen_add_match(name, n)) {
			fclose(fp);
			return;
		}
	}

	fclose(fp);
}

EXPORT int
add_list(fn)
	char	*fn;
{
	gen_add_list(fn, EXCLUDE);
	return (1);
}

EXPORT int
i_add_list(fn)
	char	*fn;
{
	gen_add_list(fn, I_HIDE);
	return (1);
}

EXPORT int
h_add_list(fn)
	char	*fn;
{
	gen_add_list(fn, H_HIDE);
	return (1);
}

EXPORT int
j_add_list(fn)
	char	*fn;
{
	gen_add_list(fn, J_HIDE);
	return (1);
}

EXPORT int
u_add_list(fn)
	char	*fn;
{
	gen_add_list(fn, U_HIDE);
	return (1);
}

#ifdef	APPLE_HYB
EXPORT int
hfs_add_list(fn)
	char	*fn;
{
	gen_add_list(fn, HFS_HIDE);
	return (1);
}
#endif	/* APPLE_HYB */

EXPORT int
gen_matches(fn, n)
	char	*fn;
	int	n;
{
	register match * mat;
		int		flags = FNM_PATHNAME;


	if (n >= MAX_MAT)
		return (0);

	if (match_igncase)
		flags |= FNM_IGNORECASE;

	for (mat = mats[n]; mat; mat = mat->next) {
		if (fnmatch(mat->name, fn, flags) != FNM_NOMATCH) {
			return (1);	/* found -> excluded filename */
		}
	}
	return (0);			/* not found -> not excluded */
}

EXPORT int
gen_ishidden(n)
	int	n;
{
	if (n >= MAX_MAT)
		return (0);

	return ((int)(mats[n] != 0));
}

EXPORT void
gen_del_match(n)
	int	n;
{
	register match	*mat;
	register match 	*mat1;

	if (n >= MAX_MAT)
		return;

	mat = mats[n];

	while (mat) {
		mat1 = mat->next;

		free(mat->name);
		free(mat);

		mat = mat1;
	}

	mats[n] = 0;
}
