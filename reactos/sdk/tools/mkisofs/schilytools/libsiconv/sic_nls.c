/* @(#)sic_nls.c	1.18 14/01/15 Copyright 2007-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sic_nls.c	1.18 14/01/15 Copyright 2007-2014 J. Schilling";
#endif
/*
 * This code reads translation files in the format used by
 * the Unicode Organization (www.unicode.org).
 *
 * The current implementation is only useful to create translations
 * from single byte character sets to unicode.
 * We use this code on systems that do not provide the iconv() function.
 *
 * Copyright 2007-2014 J. Schilling
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/libport.h>	/* For strdup() */
#include <schily/unistd.h>	/* For R_OK	*/
#include <schily/schily.h>
#include <schily/dirent.h>
#include <schily/siconv.h>

#define	TAB_SIZE	(UINT8_MAX+1)
#define	__CAN_TAB_SIZE__

#ifndef	PROTOTYPES
#undef	__CAN_TAB_SIZE__
#endif
#if (!defined(__STDC__) || __STDC__ < 1) && \
	!defined(__SUNPRO_C) /* Sun Compilers are OK even with __STDC__ 0 */
/*
 * C-preprocessors from K&R compilers cannot do the computation for TAB_SIZE
 * in the next line We need to disable this test in case of a K&R compiler.
 */
#undef	__CAN_TAB_SIZE__
#endif
#ifdef	__GNUC__
#if	__GNUC__ < 2
#undef	__CAN_TAB_SIZE__
#endif
#if	__GNUC__ < 3 && __GNUC_MINOR__ < 95
#undef	__CAN_TAB_SIZE__
#endif
#endif
#if defined(VMS) && !defined(__GNUC__)
#undef	__CAN_TAB_SIZE__
#endif

#ifdef	__CAN_TAB_SIZE__
#if	TAB_SIZE < 256
Error Table size too small
#endif
#endif

LOCAL UInt8_t	nullpage[TAB_SIZE] = { 0 };
LOCAL char	*ins_base;

LOCAL	siconvt_t	*insert_sic		__PR((siconvt_t *sip));
LOCAL	int		remove_sic		__PR((siconvt_t *sip));
EXPORT	siconvt_t	*sic_open		__PR((char *name));
EXPORT	const char	*sic_base		__PR((void));
EXPORT	int		sic_close		__PR((siconvt_t *sip));
EXPORT	int		sic_list		__PR((FILE *f));
LOCAL	void		freetbl			__PR((UInt8_t **uni2cs));
LOCAL	FILE		*pfopen			__PR((char *name));
LOCAL	siconvt_t	*create_sic		__PR((char *name));
#ifdef	USE_ICONV
LOCAL	siconvt_t	*create_iconv_sic	__PR((char *name));
LOCAL	siconvt_t	*dup_iconv_sic		__PR((siconvt_t *sip));
#endif

/*
 * Global list for translation tables
 */
LOCAL siconvt_t	*glist = (siconvt_t *) NULL;

/*
 * Insert a table into the global list and allow to reuse it
 */
LOCAL siconvt_t *
insert_sic(sip)
	siconvt_t	*sip;
{
	siconvt_t	**sp = &glist;

	if (sip == (siconvt_t *)NULL)		/* No table arg */
		return ((siconvt_t *)NULL);
	if (sip->sic_next)			/* Already in list */
		return (sip);

	while (*sp) {
		if (sip == *sp) {		/* Already in list */
			return (sip);
		}
		sp = &(*sp)->sic_next;
	}
	sip->sic_next = glist;
	glist = sip;
	return (sip);
}

/*
 * Remove a table from the global list
 */
LOCAL int
remove_sic(sip)
	siconvt_t	*sip;
{
	siconvt_t	**sp = &glist;

	while (*sp) {
#ifdef	USE_ICONV
		if (strcmp(sip->sic_name, (*sp)->sic_name) == 0) {
			siconvt_t	*sap = *sp;

			if (sip == *sp) {
				*sp = sip->sic_next;
				return (0);
			}
			while (sap->sic_alt != NULL) {
				if (sap->sic_alt == sip) {
					sap->sic_alt = sip->sic_alt;
					sip->sic_name = NULL;	/* No free() */
					return (0);
				}
				sap = sap->sic_alt;
			}
		}
#endif
		if (sip == *sp) {
			*sp = sip->sic_next;
			return (0);
		}
		sp = &(*sp)->sic_next;
	}
	return (-1);
}

/*
 * Open a new translation
 */
EXPORT siconvt_t *
sic_open(charset)
	char	*charset;
{
	siconvt_t	*sip = glist;

	if (charset == NULL || *charset == '\0')
		return ((siconvt_t *)NULL);

	while (sip) {
		if (strcmp(sip->sic_name, charset) == 0) {
#ifdef	USE_ICONV
			if (sip->sic_cd2uni != 0)
				return (dup_iconv_sic(sip));
#endif
			sip->sic_refcnt++;
			return (sip);
		}
		sip = sip->sic_next;
	}
	return (create_sic(charset));
}

/*
 * Open a new translation
 */
EXPORT const char *
sic_base()
{
	if (ins_base == NULL) {
		ins_base = searchfileinpath("lib/siconv/iso8859-1", R_OK,
					SIP_PLAIN_FILE, NULL);
		if (ins_base != NULL) {
			int	len = strlen(ins_base);

			ins_base[len - 9] = '\0';
		}
	}
	return (ins_base);
}

/*
 * Close a translation
 */
EXPORT int
sic_close(sip)
	siconvt_t	*sip;
{
	if (remove_sic(sip) < 0)
		return (-1);

	if (--sip->sic_refcnt > 0)
		return (0);

	if (sip->sic_name)
		free(sip->sic_name);
	if (sip->sic_uni2cs)
		freetbl(sip->sic_uni2cs);
	if (sip->sic_cs2uni)
		free(sip->sic_cs2uni);
#ifdef	USE_ICONV
	if (sip->sic_cd2uni)
		iconv_close(sip->sic_cd2uni);
	if (sip->sic_uni2cd)
		iconv_close(sip->sic_uni2cd);
#endif

	return (0);
}

/*
 * List all possible translation files in the install directory.
 */
EXPORT int
sic_list(f)
	FILE	*f;
{
	char		path[1024];
	DIR		*d;
	struct dirent	*dp;
	int		i = 0;

	if (ins_base == NULL)
		(void) sic_base();

	if (ins_base != NULL)
		snprintf(path, sizeof (path), "%s", ins_base);
	else
		snprintf(path, sizeof (path), "%s/lib/siconv/", INS_BASE);
	if ((d = opendir(path)) == NULL)
		return (-1);

	while ((dp = readdir(d)) != NULL) {
		if (dp->d_name[0] == '.') {
			if (dp->d_name[1] == '\0')
				continue;
			if (dp->d_name[1] == '.' && dp->d_name[2] == '\0')
				continue;
		}
		fprintf(f, "%s\n", dp->d_name);
		i++;
	}
	return (i);
}

/*
 * Free a reverse (uncode -> char) translation table
 */
LOCAL void
freetbl(uni2cs)
	UInt8_t	**uni2cs;
{
	int	i;

	for (i = 0; i < TAB_SIZE; i++) {
		if (uni2cs[i] != nullpage) {
			free(uni2cs[i]);
		}
	}
	free(uni2cs);
}

/*
 * Search a tranlation table, first in the current directory and then
 * in the install directory.
 */
LOCAL FILE *
pfopen(name)
	char	*name;
{
	char	path[1024];
	char	*p;

	if (strchr(name, '/'))
		return (fopen(name, "r"));

	if (ins_base == NULL)
		(void) sic_base();

	p = ins_base;
	if (p != NULL) {
		snprintf(path, sizeof (path), "%s%s", p, name);
		return (fopen(path, "r"));
	}
	snprintf(path, sizeof (path), "%s/lib/siconv/%s", INS_BASE, name);
	return (fopen(path, "r"));
}


/*
 * Create a new translation either from a file or from iconv_open()
 */
LOCAL siconvt_t *
create_sic(name)
	char	*name;
{
	UInt16_t	*cs2uni  = NULL;
	UInt8_t		**uni2cs = NULL;
	siconvt_t	*sip;
	char		line[1024];
	FILE		*f;
	unsigned	ch;
	unsigned	uni;
	int		i;
	int		numtrans = 0;

	if (name == NULL || *name == '\0')
		return ((siconvt_t *)NULL);

#ifdef	USE_ICONV
	/*
	 * Explicitly search for an iconv based translation
	 */
	if (strncmp("iconv:", name, 6) == 0) {
		return (create_iconv_sic(name));
	}
#else
	if (strncmp("iconv:", name, 6) == 0) {
		return ((siconvt_t *)NULL);
	}
#endif

	if ((f = pfopen(name)) == (FILE *)NULL) {
		if (strcmp(name, "default") == 0) {
			if ((cs2uni = (UInt16_t *)
			    malloc(sizeof (UInt16_t) * TAB_SIZE)) == NULL) {
				return ((siconvt_t *)NULL);
			}
			/*
			 * Set up a 1:1 translation table like ISO-8859-1
			 */
			for (i = 0; i < TAB_SIZE; i++)
				cs2uni[i] = i;
			goto do_reverse;
		}
#ifdef	USE_ICONV
		return (create_iconv_sic(name));
#else
		return ((siconvt_t *)NULL);
#endif
	}

	if ((cs2uni = (UInt16_t *)
			malloc(sizeof (UInt16_t) * TAB_SIZE)) == NULL) {
		fclose(f);
		return ((siconvt_t *)NULL);
	}

	/*
	 * Set up mapping base.
	 * Always map the control characters 0x00 .. 0x1F
	 */
	for (i = 0; i < 32; i++)
		cs2uni[i] = i;

	for (i = 32; i < TAB_SIZE; i++)
		cs2uni[i] = '\0'; /* nul marks an illegal character */

	cs2uni[0x7f] = 0x7F;	/* Always map DELETE character 0x7F */

	while (fgets(line, sizeof (line), f) != NULL) {
		char	*p;

		if ((p = strchr(line, '#')) != NULL)
			*p = '\0';

		if (sscanf(line, "%x%x", &ch, &uni) == 2) {
			/*
			 * Only accept exactly two values in the right range.
			 */
			if (ch > 0xFF || uni > 0xFFFF)
				continue;

			cs2uni[ch] = uni; /* Set up unicode translation */
			numtrans++;
		}
	}
	fclose(f);

	if (numtrans == 0) {		/* No valid translations found */
		free(cs2uni);
		return ((siconvt_t *)NULL);
	}

do_reverse:
	if ((uni2cs = (UInt8_t **)
			malloc(sizeof (unsigned char *) * TAB_SIZE)) == NULL) {
		free(cs2uni);
		return ((siconvt_t *)NULL);
	}
	for (i = 0; i < TAB_SIZE; i++)	/* Map all pages to the nullpage */
		uni2cs[i] = nullpage;

	/*
	 * Create a reversed table from the forward table read from the file.
	 */
	for (i = 0; i < TAB_SIZE; i++) {
		UInt8_t	high;
		UInt8_t	low;
		UInt8_t	*page;

		uni = cs2uni[i];
		high = (uni >> 8) & 0xFF;
		low = uni & 0xFF;
		page = uni2cs[high];

		if (page == nullpage) {
			int	j;

			/*
			 * Do not write to the nullpage but replace it by
			 * new and specific memory.
			 */
			if ((page = (UInt8_t *) malloc(TAB_SIZE)) == NULL) {
				free(cs2uni);
				freetbl(uni2cs);
				return ((siconvt_t *)NULL);
			}
			for (j = 0; j < TAB_SIZE; j++)
				page[j] = '\0';
			uni2cs[high] = page;
		}
		page[low] = i;		/* Set up the reverse translation */
	}

	if ((sip = (siconvt_t *)malloc(sizeof (siconvt_t))) == NULL) {
		free(cs2uni);
		freetbl(uni2cs);
		return ((siconvt_t *)NULL);
	}

	sip->sic_name = strdup(name);
	sip->sic_uni2cs = uni2cs;
	sip->sic_cs2uni = cs2uni;
	sip->sic_cd2uni = NULL;
	sip->sic_uni2cd = NULL;
	sip->sic_alt    = NULL;
	sip->sic_next   = NULL;
	sip->sic_refcnt = 1;

	return (insert_sic(sip));
}


#ifdef	USE_ICONV

/*
 * Create a new translation from iconv_open()
 */
LOCAL siconvt_t *
create_iconv_sic(name)
	char	*name;
{
	siconvt_t	*sip;
	iconv_t		to;
	iconv_t		from;
	char		*nm;

/*cerror("init_unls_iconv(%s)\n", name);*/
	if (name == NULL || *name == '\0')
		return ((siconvt_t *)NULL);

	nm = name;
	if (strncmp("iconv:", name, 6) == 0)
		nm = &name[6];

	if ((sip = (siconvt_t *)malloc(sizeof (siconvt_t)))
							== NULL) {
		return ((siconvt_t *)NULL);
	}
	if ((from = iconv_open("UCS-2BE", nm)) == (iconv_t)-1) {
		free(sip);
		return ((siconvt_t *)NULL);
	}
	if ((to = iconv_open(nm, "UCS-2BE")) == (iconv_t)-1) {
		free(sip);
		iconv_close(from);
		return ((siconvt_t *)NULL);
	}

	sip->sic_name = strdup(name);
	sip->sic_uni2cs = NULL;
	sip->sic_cs2uni = NULL;
	sip->sic_cd2uni = from;
	sip->sic_uni2cd = to;
	sip->sic_alt    = NULL;
	sip->sic_next   = NULL;
	sip->sic_refcnt = 1;
	return (insert_sic(sip));
}

/*
 * As the iconv conversion is stateful, we need to create a new translation
 * if we like to get the same translation again.
 */
LOCAL siconvt_t *
dup_iconv_sic(sip)
	siconvt_t	*sip;
{
	siconvt_t	*sp;
	iconv_t		to;
	iconv_t		from;
	char		*nm;

	if ((sp = (siconvt_t *)malloc(sizeof (siconvt_t)))
							== NULL) {
		return ((siconvt_t *)NULL);
	}
	nm = sip->sic_name;
	if (strncmp("iconv:", nm, 6) == 0)
		nm = &nm[6];
	if ((from = iconv_open("UCS-2BE", nm)) == (iconv_t)-1) {
		free(sp);
		return ((siconvt_t *)NULL);
	}
	if ((to = iconv_open(nm, "UCS-2BE")) == (iconv_t)-1) {
		free(sp);
		iconv_close(from);
		return ((siconvt_t *)NULL);
	}
	sp->sic_name = sip->sic_name;	/* Allow to compare name pointers */
	sp->sic_uni2cs = NULL;
	sp->sic_cs2uni = NULL;
	sp->sic_cd2uni = from;
	sp->sic_uni2cd = to;
	sp->sic_alt    = NULL;
	sp->sic_next   = NULL;
	sp->sic_refcnt = 1;
	sip->sic_alt = sp;
	return (sp);
}

#endif	/* USE_UNLS */
