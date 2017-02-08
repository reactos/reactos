/* @(#)name.c	1.38 10/12/19 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)name.c	1.38 10/12/19 joerg";

#endif
/*
 * File name.c - map full Unix file names to unique 8.3 names that
 * would be valid on DOS.
 *
 *
 * Written by Eric Youngdale (1993).
 * Almost totally rewritten by J. Schilling (2000).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999,2000-2010 J. Schilling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "mkisofs.h"
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/ctype.h>

void	iso9660_check		__PR((struct iso_directory_record *idr,	struct directory_entry *ndr));
int	iso9660_file_length	__PR((const char *name,
					struct directory_entry *sresult,
					int dirflag));

void
iso9660_check(idr, ndr)
	struct iso_directory_record *idr;
	struct directory_entry *ndr;
{
	int	nlen;
	char	schar;
	char	*p;
	char	*np;

	nlen = idr->name_len[0];
	schar = idr->name[nlen];

	if (nlen == 1 && (idr->name[0] == '\0' || idr->name[0] == '\001'))
		return;

	idr->name[nlen] = '\0';		/* Make it null terminated */
	if ((p = strrchr(idr->name, ';')) != NULL) {
		*p = '\0';		/* Strip off old version # */
	}
	iso9660_file_length(idr->name, ndr,
				(idr->flags[0] & ISO_DIRECTORY) != 0);

	if ((np = strrchr(ndr->isorec.name, ';')) != NULL) {
		*np = '\0';		/* Strip off new version # */
	}
	if (strcmp(idr->name, ndr->isorec.name) != 0) {
		if (p)
			*p = ';';	/* Restore old version # */
		if (np)
			*np = ';';	/* Restore new version # */
		errmsgno(EX_BAD,
			_("Old session has illegal name '%.*s' length %d\n"),
			idr->name_len[0],
			idr->name,
			idr->name_len[0]);
		errmsgno(EX_BAD,
			_("New session will use    name '%s'\n"),
			ndr->isorec.name);
	}
	if (p)
		*p = ';';		/* Restore old version # */
	if (np)
		*np = ';';		/* Restore new version # */
	idr->name[nlen] = schar;	/* Restore old iso record*/
}

/*
 * Function:	iso9660_file_length
 *
 * Purpose:	Map file name to 8.3 format, return length
 *		of result.
 *
 * Arguments:	name	file name we need to map.
 *		sresult	directory entry structure to contain mapped name.
 *		dirflag	flag indicating whether this is a directory or not.
 *
 * Note:	name being const * is a bug introduced by Eric but hard to
 *		fix without going through the whole source.
 */
int
iso9660_file_length(name, sresult, dirflag)
	const char	*name;			/* Not really const !!! */
	struct directory_entry *sresult;
	int		dirflag;
{
	char		c;
	char		*cp;
	int		before_dot = 8;
	int		after_dot = 3;
	int		chars_after_dot = 0;
	int		chars_before_dot = 0;
	int		current_length = 0;
	int		extra = 0;
	int		ignore = 0;
	char		*last_dot;
	const char	*pnt;
	int		priority = 32767;
	char		*result;
	int		ochars_after_dot;
	int		ochars_before_dot;
	int		seen_dot = 0;
	int		seen_semic = 0;
#ifdef		Eric_code_does_not_work
	int		tildes = 0;
#endif

	result = sresult->isorec.name;

	if (sresult->priority)
		priority = sresult->priority;

	/*
	 * For the '.' entry, generate the correct record, and return 1 for
	 * the length.
	 */
	if (strcmp(name, ".") == 0) {
		*result = 0;
		return (1);
	}
	/*
	 * For the '..' entry, generate the correct record, and return 1
	 * for the length.
	 */
	if (strcmp(name, "..") == 0) {
		*result++ = 1;
		*result++ = 0;
		return (1);
	}
	/*
	 * Now scan the directory one character at a time, and figure out
	 * what to do.
	 */
	pnt = name;

	/*
	 * Find the '.' that we intend to use for the extension.
	 * Usually this is the last dot, but if we have . followed by nothing
	 * or a ~, we would consider this to be unsatisfactory, and we keep
	 * searching.
	 */
	last_dot = strrchr(pnt, '.');
	if ((last_dot != NULL) &&
	    ((last_dot[1] == '~') || (last_dot[1] == '\0'))) {
		cp = last_dot;
		*cp = '\0';
		last_dot = strrchr(pnt, '.');
		*cp = '.';
		/*
		 * If we found no better '.' back up to the last match.
		 */
		if (last_dot == NULL)
			last_dot = cp;
	}

	if (last_dot != NULL) {
		ochars_after_dot = strlen(last_dot);	/* dot counts */
		ochars_before_dot = last_dot - pnt;
	} else {
		ochars_before_dot = 128;
		ochars_after_dot = 0;
	}
	/*
	 * If we have full names, the names we generate will not work
	 * on a DOS machine, since they are not guaranteed to be 8.3.
	 * Nonetheless, in many cases this is a useful option.  We
	 * still only allow one '.' character in the name, however.
	 */
	if (full_iso9660_filenames || iso9660_level > 1) {
		before_dot = iso9660_namelen;
		after_dot = before_dot - 1;

		if (!dirflag) {
			if (ochars_after_dot > ((iso9660_namelen/2)+1)) {
				/*
				 * The minimum number of characters before
				 * the dot is 3 to allow renaming.
				 * Let us allow to have 15 characters after
				 * dot to give more rational filenames.
				 */
				before_dot = iso9660_namelen/2;
				after_dot = ochars_after_dot;
			} else {
				before_dot -= ochars_after_dot; /* dot counts */
				after_dot = ochars_after_dot;
			}
		}
	}

	while (*pnt) {
#ifdef VMS
		if (strcmp(pnt, ".DIR;1") == 0) {
			break;
		}
#endif

#ifdef		Eric_code_does_not_work
		/*
		 * XXX If we make this code active we get corrupted direcrory
		 * XXX trees with infinite loops.
		 */
		/*
		 * This character indicates a Unix style of backup file
		 * generated by some editors.  Lower the priority of the file.
		 */
		if (iso_translate && *pnt == '#') {
			priority = 1;
			pnt++;
			continue;
		}
		/*
		 * This character indicates a Unix style of backup file
		 * generated by some editors.  Lower the priority of the file.
		 */
		if (iso_translate && *pnt == '~') {
			priority = 1;
			tildes++;
			pnt++;
			continue;
		}
#endif
		/*
		 * This might come up if we had some joker already try and put
		 * iso9660 version numbers into the file names.  This would be
		 * a silly thing to do on a Unix box, but we check for it
		 * anyways.  If we see this, then we don't have to add our own
		 * version number at the end. UNLESS the ';' is part of the
		 * filename and no valid version number is following.
		 */
		if (use_fileversion && *pnt == ';' && seen_dot) {
			/*
			 * Check if a valid version number follows.
			 * The maximum valid version number is 32767.
			 */
			for (c = 1, cp = (char *)&pnt[1]; c < 6 && *cp; c++, cp++) {
				if (*cp < '0' || *cp > '9')
					break;
			}
			if (c <= 6 && *cp == '\0' && atoi(&pnt[1]) <= 32767)
				seen_semic++;
		}
		/*
		 * If we have a name with multiple '.' characters, we ignore
		 * everything after we have gotten the extension.
		 */
		if (ignore) {
			pnt++;
			continue;
		}
		if (current_length >= iso9660_namelen) {
#ifdef	nono
			/*
			 * Does not work as we may truncate before the dot.
			 */
			error(_("Truncating '%s' to '%.*s'.\n"),
				name,
				current_length, sresult->isorec.name);
			ignore++;
#endif
			pnt++;
			continue;
		}
		/* Spin past any iso9660 version number we might have. */
		if (seen_semic) {
			if (seen_semic == 1) {
				seen_semic++;
				*result++ = ';';
			}
			if (*pnt >= '0' && *pnt <= '9') {
				*result++ = *pnt;
			}
			extra++;
			pnt++;
			continue;
		}

		if (*pnt == '.') {
			if (!allow_multidot) {
				if (strcmp(pnt, ".tar.gz") == 0)
					pnt = last_dot = ".tgz";
				if (strcmp(pnt, ".ps.gz") == 0)
					pnt = last_dot = ".psz";
			}

			if (!chars_before_dot && !allow_leading_dots) {
				/*
				 * DOS can't read files with dot first
				 */
				chars_before_dot++;
				*result++ = '_'; /* Substitute underscore */

			} else if (pnt == last_dot) {
				if (seen_dot) {
					ignore++;
					continue;
				}
				*result++ = '.';
				seen_dot++;
			} else if (allow_multidot) {
				if (chars_before_dot < before_dot) {
					chars_before_dot++;
					*result++ = '.';
				}
			} else {
				/*
				 * If this isn't the dot that we use
				 * for the extension, then change the
				 * character into a '_' instead.
				 */
				if (chars_before_dot < before_dot) {
					chars_before_dot++;
					*result++ = '_';
				}
			}
		} else {
			if ((seen_dot && (chars_after_dot < after_dot) &&
						++chars_after_dot) ||
			    (!seen_dot && (chars_before_dot < before_dot) &&
			    ++chars_before_dot)) {

				c = *pnt;
				if (c & 0x80) {
					/*
					 * We allow 8 bit chars if -iso-level
					 * is at least 4
					 *
					 * XXX We should check if the output
					 * XXX character set is a 7 Bit ASCI
					 * extension.
					 */
					if (iso9660_level >= 4) {
						size_t	flen = 1;
						size_t	tlen = 1;

						/*
						 * XXX This currently only works for
						 * XXX non iconv() based locales.
						 */
						if (in_nls->sic_cd2uni == NULL) {
							conv_charset(
							    (Uchar *)&c, &tlen,
							    (Uchar *)&c, &flen,
							    in_nls, out_nls);
						}
					} else {
						c = '_';
					}
				} else if (!allow_lowercase) {
					c = islower((unsigned char)c) ?
						toupper((unsigned char)c) : c;
				}
				if (relaxed_filenames) {
					/*
					 * Here we allow a more relaxed syntax.
					 */
					if (c == '/')
						c = '_';
					*result++ = c;
				} else switch (c) {
					/*
					 * Dos style filenames.
					 * We really restrict the names here.
					 */

				default:
					*result++ = c;
					break;

				/*
				 * Descriptions of DOS's 'Parse Filename'
				 * (function 29H) describes V1 and V2.0+
				 * separator and terminator characters. These
				 * characters in a DOS name make the file
				 * visible but un-manipulable (all useful
				 * operations error off.
				 */
				/* separators */
				case '+':
				case '=':
				case '%': /* not legal DOS */
						/* filename */
				case ':':
				case ';': /* already handled */
				case '.': /* already handled */
				case ',': /* already handled */
				case '\t':
				case ' ':
				/* V1 only separators */
				case '/':
				case '"':
				case '[':
				case ']':
				/* terminators */
				case '>':
				case '<':
				case '|':
				/*
				 * Other characters that are not valid ISO-9660
				 * characters.
				 */
				case '!':
/*				case '#':*/
				case '$':
				case '&':
				case '\'':
				case '(':
				case ')':
				case '*':
/*				case '-':*/
				case '?':
				case '@':
				case '\\':
				case '^':
				case '`':
				case '{':
				case '}':
/*				case '~':*/
				/*
				 * All characters below 32 (space) are not
				 * allowed too.
				 */
				case 1: case 2: case 3: case 4:
				case 5: case 6: case 7: case 8:
				/* case 9: */
				case 10: case 11: case 12:
				case 13: case 14: case 15:
				case 16: case 17: case 18:
				case 19: case 20: case 21:
				case 22: case 23: case 24:
				case 25: case 26: case 27:
				case 28: case 29: case 30:
				case 31:

					/*
					 * Hmm - what to do here? Skip? Win95
					 * looks like it substitutes '_'
					 */
					*result++ = '_';
					break;

				case '#':
				case '-':
				case '~':
					/*
					 * Check if we should allow these
					 * illegal characters used by
					 * Microsoft.
					 */
					if (iso_translate)
						*result++ = '_';
					else
						*result++ = c;
					break;
				}	/* switch (*pnt) */
			} else {	/* if (chars_{after,before}_dot) ... */
				pnt++;
				continue;
			}
		}	/* else *pnt == '.' */
		current_length++;
		pnt++;
	}	/* while (*pnt) */

	/*
	 * OK, that wraps up the scan of the name.  Now tidy up a few other
	 * things.
	 * Look for emacs style of numbered backups, like foo.c.~3~.  If we
	 * see this, convert the version number into the priority number.
	 * In case of name conflicts, this is what would end up being used as
	 * the 'extension'.
	 */
#ifdef		Eric_code_does_not_work
	if (tildes == 2) {
		int	prio1 = 0;

		pnt = name;
		while (*pnt && *pnt != '~') {
			pnt++;
		}
		if (*pnt) {
			pnt++;
		}
		while (*pnt && *pnt != '~') {
			prio1 = 10 * prio1 + *pnt - '0';
			pnt++;
		}
		priority = prio1;
	}
#endif
	/*
	 * If this is not a directory, force a '.' in case we haven't seen one,
	 * and add a version number if we haven't seen one of those either.
	 */
	if (!dirflag) {
		if (!seen_dot && !omit_period) {
			if (chars_before_dot >= (iso9660_namelen-1)) {
				chars_before_dot--;
				result--;
			}
			*result++ = '.';
			extra++;
		}
		if (!omit_version_number && !seen_semic) {
			*result++ = ';';
			*result++ = '1';
			extra += 2;
		}
	}
	*result++ = 0;
	sresult->priority = priority;

/*#define	DEBBUG*/
#ifdef	DEBBUG
	error("NAME: '%s'\n", sresult->isorec.name);
	error("chars_before_dot %d chars_after_dot %d seen_dot %d extra %d\n",
		chars_before_dot, chars_after_dot, seen_dot, extra);
#endif
	return (chars_before_dot + chars_after_dot + seen_dot + extra);
}
