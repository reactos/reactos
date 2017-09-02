/* @(#)joliet.c	1.68 15/12/30 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)joliet.c	1.68 15/12/30 joerg";
#endif
/*
 * File joliet.c - handle Win95/WinNT long file/unicode extensions for iso9660.
 *
 * Copyright 1997 Eric Youngdale.
 * APPLE_HYB James Pearson j.pearson@ge.ucl.ac.uk 22/2/2000
 * Copyright (c) 1999-2015 J. Schilling
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

/*
 * Joliet extensions for ISO9660.  These are spottily documented by
 * Microsoft.  In their infinite stupidity, they completely ignored
 * the possibility of using an SUSP record with the long filename
 * in it, and instead wrote out a duplicate directory tree with the
 * long filenames in it.
 *
 * I am not sure why they did this.  One reason is that they get the path
 * tables with the long filenames in them.
 *
 * There are two basic principles to Joliet, and the non-Unicode variant
 * known as Romeo.  Long filenames seem to be the main one, and the second
 * is that the character set and a few other things is substantially relaxed.
 *
 * The SVD is identical to the PVD, except:
 *
 *	Id is 2, not 1 (indicates SVD).
 *	escape_sequences contains UCS-2 indicator (levels 1, 2 or 3).
 *	The root directory record points to a different extent (with different
 *		size).
 *	There are different path tables for the two sets of directory trees.
 *
 * The Unicode level is coded in the SVD as follows:
 *
 *	Standard	Level	ASCII escape code
 *	UCS-2		Level-1	%/@
 *	UCS-2		Level-2	%/C
 *	UCS-2		Level-3	%/E
 *
 * The following fields are recorded in Unicode:
 *	system_id
 *	volume_id
 *	volume_set_id
 *	publisher_id
 *	preparer_id
 *	application_id
 *	copyright_file_id
 *	abstract_file_id
 *	bibliographic_file_id
 *
 * Unicode strings are always encoded in big-endian format.
 *
 * In a directory record, everything is the same as with iso9660, except
 * that the name is recorded in unicode.  The name length is specified in
 * total bytes, not in number of unicode characters.
 *
 * The character set used for the names is different with UCS - the
 * restrictions are that the following are not allowed:
 *
 *	Characters (00)(00) through (00)(1f) (control chars)
 *	(00)(2a) '*'
 *	(00)(2f) '/'
 *	(00)(3a) ':'
 *	(00)(3b) ';'
 *	(00)(3f) '?'
 *	(00)(5c) '\'
 */
#include "mkisofs.h"
#include <schily/time.h>
#include <schily/utypes.h>
#include <schily/intcvt.h>
#include <schily/schily.h>
#include <schily/errno.h>

LOCAL	Uint		jpath_table_index;
LOCAL	struct directory **jpathlist;
LOCAL	int		next_jpath_index = 1;
LOCAL	int		jsort_goof;
LOCAL	int		jsort_glen;

LOCAL	char	ucs_codes[] = {
		'\0',		/* UCS-level 0 is illegal	*/
		'@',		/* UCS-level 1			*/
		'C',		/* UCS-level 2			*/
		'E',		/* UCS-level 3			*/
};

#ifdef	UDF
EXPORT	void	convert_to_unicode	__PR((unsigned char *buffer,
						int size, char *source,
						siconvt_t *inls));
EXPORT	int	joliet_strlen		__PR((const char *string, size_t maxlen,
						siconvt_t *inls));
#else
LOCAL	void	convert_to_unicode	__PR((unsigned char *buffer,
						int size, char *source,
						siconvt_t *inls));
LOCAL	int	joliet_strlen		__PR((const char *string, size_t maxlen,
						siconvt_t *inls));
#endif
LOCAL	void	get_joliet_vol_desc	__PR((struct iso_primary_descriptor *jvol_desc));
LOCAL	void	assign_joliet_directory_addresses __PR((struct directory *node));
LOCAL	void	build_jpathlist		__PR((struct directory *node));
LOCAL	int	joliet_compare_paths	__PR((void const *r, void const *l));
LOCAL	int	generate_joliet_path_tables __PR((void));
LOCAL	void	generate_one_joliet_directory __PR((struct directory *dpnt,
						FILE *outfile));
LOCAL	int	joliet_sort_n_finish	__PR((struct directory *this_dir));

LOCAL	int	joliet_compare_dirs	__PR((const void *rr, const void *ll));

LOCAL	int	joliet_sort_directory	__PR((struct directory_entry **sort_dir));
EXPORT	int	joliet_sort_tree	__PR((struct directory *node));
LOCAL	void	generate_joliet_directories __PR((struct directory *node,
						FILE *outfile));
LOCAL	int	jpathtab_write		__PR((FILE *outfile));
LOCAL	int	jdirtree_size		__PR((UInt32_t starting_extent));
LOCAL	int	jroot_gen		__PR((void));
LOCAL	int	jdirtree_write		__PR((FILE *outfile));
LOCAL	int	jvd_write		__PR((FILE *outfile));
LOCAL	int	jpathtab_size		__PR((UInt32_t starting_extent));

/*
 *	conv_charset: convert to/from charsets via Unicode.
 *
 *	Any unknown character is set to '_'
 *
 */
EXPORT void
conv_charset(to, tosizep, from, fromsizep, inls, onls)
	unsigned char	*to;
	size_t		*tosizep;
	unsigned char	*from;
	size_t		*fromsizep;
	siconvt_t	*inls;
	siconvt_t	*onls;
{
	UInt16_t	unichar;
	size_t		fromsize = *fromsizep;
	size_t		tosize   = *tosizep;
	Uchar		ob[2];			/* 2 octets (16 Bit) UCS-2 */

	if (fromsize == 0 || tosize == 0)
		return;

	/*
	 * If we have a null mapping, just return the input character
	 */
	if (inls->sic_name == onls->sic_name) {
		*to = *from;
		(*fromsizep)--;
		(*tosizep)--;
		return;
	}
#ifdef	USE_ICONV
#ifdef	HAVE_ICONV_CONST
#define	__IC_CONST	const
#else
#define	__IC_CONST
#endif
	if (use_iconv(inls)) {
		char	*obuf = (char *)ob;
		size_t	osize = 2;		/* UCS-2 character size */

		if (iconv(inls->sic_cd2uni, (__IC_CONST char **)&from,
					fromsizep,
					&obuf, &osize) == -1) {
			int	err = geterrno();

			if ((err == EINVAL || err == EILSEQ) &&
			    *fromsizep == fromsize) {
				ob[0] = 0; ob[1] = '_';
				(*fromsizep)--;
			}
		}
		unichar = ob[0] * 256 + ob[1];	/* Compute 16 Bit UCS-2 char */
	} else
#endif
	{
		unsigned char c = *from;

		unichar = sic_c2uni(inls, c);	/* Get the UNICODE char */
		(*fromsizep)--;

		if (unichar == 0)
			unichar = '_';

		ob[0] = unichar >> 8 & 0xFF;	/* Compute 2 octet variant */
		ob[1] = unichar & 0xFF;
	}

#ifdef	USE_ICONV
	if (use_iconv(onls)) {
		char	*ibuf = (char *)ob;
		size_t	isize = 2;		/* UCS-2 character size */

		if (iconv(onls->sic_uni2cd, (__IC_CONST char **)&ibuf, &isize,
					(char **)&to, tosizep) == -1) {
			int	err = geterrno();

			if ((err == EINVAL || err == EILSEQ) &&
			    *tosizep == tosize) {
				*to = '_';
				(*tosizep)--;
			}
		}
	} else
#endif
	{
		*to = sic_uni2c(onls, unichar);	/* Get the backconverted char */
		(*tosizep)--;
	}
}


/*
 * Function:		convert_to_unicode
 *
 * Purpose:		Perform a unicode conversion on a text string
 *			using the supplied input character set.
 *
 * Notes:
 */
#ifdef	UDF
EXPORT void
#else
LOCAL void
#endif
convert_to_unicode(buffer, size, source, inls)
	unsigned char	*buffer;
	int		size;
	char		*source;
	siconvt_t	*inls;
{
	unsigned char	*tmpbuf;
	int		i;
	int		j;
	UInt16_t	unichar;
	unsigned char	uc;
	int		jsize = size;

	/*
	 * If we get a NULL pointer for the source, it means we have an
	 * inplace copy, and we need to make a temporary working copy first.
	 */
	if (source == NULL) {
		tmpbuf = (Uchar *) e_malloc(size);
		memcpy(tmpbuf, buffer, size);
	} else {
		tmpbuf = (Uchar *) source;
	}

	/*
	 * Now start copying characters.  If the size was specified to be 0,
	 * then assume the input was 0 terminated.
	 */
	j = 0;
	for (i = 0; (i + 1) < size; i += 2, j++) {	/* Size may be odd! */
		/*
		 * Let all valid unicode characters pass
		 * through (according to charset). Others are set to '_' .
		 */
		if (j < jsize)
			uc = tmpbuf[j];		/* temporary copy */
		else
			uc = '\0';
		if (uc == '\0') {
			jsize = j;
			unichar = 0;
		} else {			/* must be converted */
#ifdef	USE_ICONV
			if (use_iconv(inls)) {
				Uchar		ob[2];
				__IC_CONST char	*inbuf = (char *)&tmpbuf[j];
				size_t	isize = 3;
				char	*obuf = (char *)ob;
				size_t	osize = 2;

				/*
				 * iconv() from glibc ignores osize and thus
				 * may try to access more than a single multi
				 * byte character from the input and read from
				 * non-existent memory.
				 */
				if (iconv(inls->sic_cd2uni, &inbuf, &isize,
							&obuf, &osize) == -1) {
					int	err = geterrno();

					if ((err == EINVAL || err == EILSEQ) &&
					    isize == 3) {
						ob[0] = ob[1] = 0;
						isize--;
					}
				}
				unichar = ob[0] * 256 + ob[1];
				j += 2 - isize;
			} else
#endif
			unichar = sic_c2uni(inls, uc);	/* Get the UNICODE */

			/*
			 * This code is currently also used for UDF formatting.
			 * Do not enforce silly Microsoft limitations in case
			 * that we only create UDF extensions.
			 */
			if (!use_Joliet)
				goto all_chars;

			if (unichar <= 0x1f || unichar == 0x7f)
				unichar = '\0';	/* control char */

			switch (unichar) {	/* test special characters */

			case '*':
			case '/':
			case ':':
			case ';':
			case '?':
			case '\\':
			case '\0':		/* illegal char mark */
				/*
				 * Even Joliet has some standards as to what is
				 * allowed in a pathname. Pretty tame in
				 * comparison to what DOS restricts you to.
				 */
				unichar = '_';
			}
		all_chars:
			;
		}
		buffer[i] = unichar >> 8 & 0xFF; /* final UNICODE */
		buffer[i + 1] = unichar & 0xFF;	/* conversion */
	}

	if (size & 1) {	/* beautification */
		buffer[size - 1] = 0;
	}
	if (source == NULL) {
		free(tmpbuf);
	}
}

/*
 * Function:	joliet_strlen
 *
 * Purpose:	Return length in bytes of string after conversion to unicode.
 *
 * Notes:	This is provided mainly as a convenience so that when more
 * 		intelligent Unicode conversion for either Multibyte or 8-bit
 *		codes is available that we can easily adapt.
 */
#ifdef	UDF
EXPORT int
#else
LOCAL int
#endif
joliet_strlen(string, maxlen, inls)
	const char	*string;
	size_t		maxlen;
	siconvt_t	*inls;
{
	int	rtn = 0;

#ifdef	USE_ICONV
	if (use_iconv(inls)) {
		int	j = 0;

		while (string[j] != '\0') {
			Uchar		ob[2];
			__IC_CONST char	*inbuf = (char *)&string[j];
			size_t	isize = 3;
			char	*obuf = (char *)ob;
			size_t	osize = 2;

			/*
			 * iconv() from glibc ignores osize and thus
			 * may try to access more than a single multi
			 * byte character from the input and read from
			 * non-existent memory.
			 */
			if (iconv(inls->sic_cd2uni, &inbuf, &isize,
						&obuf, &osize) == -1) {
				int	err = geterrno();

				if ((err == EINVAL || err == EILSEQ) &&
				    isize == 3) {
					ob[0] = ob[1] = 0;
					isize--;
				}
			}
			j += 3 - isize;
			rtn += 2;
		}
	} else
#endif
	rtn = strlen(string) << 1;

	/*
	 * We do clamp the maximum length of a Joliet or UDF string to be the
	 * maximum path size.
	 */
	if (rtn > 2*maxlen) {
		rtn = 2*maxlen;
	}
	return (rtn);
}

/*
 * Function:		get_joliet_vol_desc
 *
 * Purpose:		generate a Joliet compatible volume desc.
 *
 * Notes:		Assume that we have the non-joliet vol desc
 *			already present in the buffer.  Just modifiy the
 *			appropriate fields.
 */
LOCAL void
get_joliet_vol_desc(jvol_desc)
	struct iso_primary_descriptor	*jvol_desc;
{
	jvol_desc->type[0] = ISO_VD_SUPPLEMENTARY;
	jvol_desc->version[0] = 1;
	jvol_desc->file_structure_version[0] = 1;
	/*
	 * For now, always do Unicode level 3.
	 * I don't really know what 1 and 2 are - perhaps a more limited
	 * Unicode set.
	 * FIXME(eric) - how does Romeo fit in here?
	 */
	sprintf(jvol_desc->escape_sequences, "%%/%c", ucs_codes[ucs_level]);

	/* Until we have Unicode path tables, leave these unset. */
	set_733((char *)jvol_desc->path_table_size, jpath_table_size);
	set_731(jvol_desc->type_l_path_table, jpath_table[0]);
	set_731(jvol_desc->opt_type_l_path_table, jpath_table[1]);
	set_732(jvol_desc->type_m_path_table, jpath_table[2]);
	set_732(jvol_desc->opt_type_m_path_table, jpath_table[3]);

	/* Set this one up. */
	memcpy(jvol_desc->root_directory_record, &jroot_record,
		offsetof(struct iso_directory_record, name[0]) + 1);

	/*
	 * Finally, we have a bunch of strings to convert to Unicode.
	 * FIXME(eric) - I don't know how to do this in general,
	 * so we will just be really lazy and do a char -> short conversion.
	 *  We probably will want to filter any characters >= 0x80.
	 */
	convert_to_unicode((Uchar *)jvol_desc->system_id,
			sizeof (jvol_desc->system_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->volume_id,
			sizeof (jvol_desc->volume_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->volume_set_id,
			sizeof (jvol_desc->volume_set_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->publisher_id,
			sizeof (jvol_desc->publisher_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->preparer_id,
			sizeof (jvol_desc->preparer_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->application_id,
			sizeof (jvol_desc->application_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->copyright_file_id,
			sizeof (jvol_desc->copyright_file_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->abstract_file_id,
			sizeof (jvol_desc->abstract_file_id), NULL, in_nls);
	convert_to_unicode((Uchar *)jvol_desc->bibliographic_file_id,
			sizeof (jvol_desc->bibliographic_file_id), NULL, in_nls);
}

/*
 * Asssign Joliet & UDF addresses
 * We ignore all files that are neither in the Joliet nor in the UDF tree
 */
LOCAL void
assign_joliet_directory_addresses(node)
	struct directory	*node;
{
	int		dir_size;
	struct directory *dpnt;

	dpnt = node;

	while (dpnt) {
		if ((dpnt->dir_flags & INHIBIT_JOLIET_ENTRY) == 0) {
			/*
			 * If we already have an extent for this
			 * (i.e. it came from a multisession disc), then
			 * don't reassign a new extent.
			 */
			dpnt->jpath_index = next_jpath_index++;
			if (dpnt->jextent == 0) {
				dpnt->jextent = last_extent;
				dir_size = ISO_BLOCKS(dpnt->jsize);
				last_extent += dir_size;
			}
		}
		/* skip if hidden - but not for the rr_moved dir */
		if (dpnt->subdir &&
		    ((dpnt->dir_flags & INHIBIT_JOLIET_ENTRY) == 0 ||
		    dpnt == reloc_dir)) {
			assign_joliet_directory_addresses(dpnt->subdir);
		}
		dpnt = dpnt->next;
	}
}

LOCAL void
build_jpathlist(node)
	struct directory	*node;
{
	struct directory	*dpnt;

	dpnt = node;

	while (dpnt) {
		if ((dpnt->dir_flags & INHIBIT_JOLIET_ENTRY) == 0) {
			jpathlist[dpnt->jpath_index] = dpnt;
		}
		if (dpnt->subdir)
			build_jpathlist(dpnt->subdir);
		dpnt = dpnt->next;
	}
} /* build_jpathlist(... */

LOCAL int
joliet_compare_paths(r, l)
	void const	*r;
	void const	*l;
{
	struct directory const *ll = *(struct directory * const *) l;
	struct directory const *rr = *(struct directory * const *) r;
	int		rparent,
			lparent;
	char		*rpnt,
			*lpnt;
	unsigned char	rtmp[2],
			ltmp[2];
	siconvt_t	*rinls, *linls;

	/* make sure root directory is first */
	if (rr == root)
		return (-1);

	if (ll == root)
		return (1);

	rparent = rr->parent->jpath_index;
	lparent = ll->parent->jpath_index;
	if (rr->parent == reloc_dir) {
		rparent = rr->self->parent_rec->filedir->jpath_index;
	}
	if (ll->parent == reloc_dir) {
		lparent = ll->self->parent_rec->filedir->jpath_index;
	}
	if (rparent < lparent) {
		return (-1);
	}
	if (rparent > lparent) {
		return (1);
	}
#ifdef APPLE_HYB
	/*
	 * we may be using the HFS name - so select the correct input
	 * charset
	 */
	if (USE_MAC_NAME(rr->self)) {
		rpnt = rr->self->hfs_ent->name;
		rinls = hfs_inls;
	} else {
		rpnt = rr->self->name;
		rinls = in_nls;
	}

	if (USE_MAC_NAME(ll->self)) {
		lpnt = ll->self->hfs_ent->name;
		linls = hfs_inls;
	} else {
		lpnt = ll->self->name;
		linls = in_nls;
	}
#else
	rpnt = rr->self->name;
	lpnt = ll->self->name;
	linls = rinls = in_nls;
#endif	/* APPLE_HYB */

	/* compare the Unicode names */

	while (*rpnt && *lpnt) {
		convert_to_unicode(rtmp, 2, rpnt, rinls);
		convert_to_unicode(ltmp, 2, lpnt, linls);

		if (a_to_u_2_byte(rtmp) < a_to_u_2_byte(ltmp))
			return (-1);
		if (a_to_u_2_byte(rtmp) > a_to_u_2_byte(ltmp))
			return (1);

		rpnt++;
		lpnt++;
	}

	if (*rpnt)
		return (1);
	if (*lpnt)
		return (-1);

	return (0);

} /* compare_paths(... */

LOCAL int
generate_joliet_path_tables()
{
	struct directory_entry *de;
	struct directory *dpnt;
	int		fix;
	int		j;
	int		namelen;
	char		*npnt;
	char		*npnt1;
	int		tablesize;
	unsigned int	jpindex;

	/* First allocate memory for the tables and initialize the memory */
	tablesize = jpath_blocks << 11;
	jpath_table_m = (char *)e_malloc(tablesize);
	jpath_table_l = (char *)e_malloc(tablesize);
	memset(jpath_table_l, 0, tablesize);
	memset(jpath_table_m, 0, tablesize);

	/* Now start filling in the path tables.  Start with root directory */
	jpath_table_index = 0;
	jpathlist = (struct directory **)e_malloc(sizeof (struct directory *)
		* next_jpath_index);
	memset(jpathlist, 0, sizeof (struct directory *) * next_jpath_index);
	build_jpathlist(root);

	do {
		fix = 0;
#ifdef	PROTOTYPES
		qsort(&jpathlist[1], next_jpath_index - 1, sizeof (struct directory *),
			(int (*) (const void *, const void *)) joliet_compare_paths);
#else
		qsort(&jpathlist[1], next_jpath_index - 1, sizeof (struct directory *),
			joliet_compare_paths);
#endif

		for (j = 1; j < next_jpath_index; j++) {
			if (jpathlist[j]->jpath_index != j) {
				jpathlist[j]->jpath_index = j;
				fix++;
			}
		}
	} while (fix);

	for (j = 1; j < next_jpath_index; j++) {
		dpnt = jpathlist[j];
		if (!dpnt) {
			comerrno(EX_BAD, _("Entry %d not in path tables\n"), j);
		}
		npnt = dpnt->de_name;

		npnt1 = strrchr(npnt, PATH_SEPARATOR);
		if (npnt1) {
			npnt = npnt1 + 1;
		}
		de = dpnt->self;
		if (!de) {
			comerrno(EX_BAD,
			_("Fatal Joliet goof - directory has amnesia\n"));
		}
#ifdef APPLE_HYB
		if (USE_MAC_NAME(de))
			namelen = joliet_strlen(de->hfs_ent->name, jlen, hfs_inls);
		else
#endif	/* APPLE_HYB */
			namelen = joliet_strlen(de->name, jlen, in_nls);

		if (dpnt == root) {
			jpath_table_l[jpath_table_index] = 1;
			jpath_table_m[jpath_table_index] = 1;
		} else {
			jpath_table_l[jpath_table_index] = namelen;
			jpath_table_m[jpath_table_index] = namelen;
		}
		jpath_table_index += 2;

		set_731(jpath_table_l + jpath_table_index, dpnt->jextent);
		set_732(jpath_table_m + jpath_table_index, dpnt->jextent);
		jpath_table_index += 4;


		if (dpnt->parent != reloc_dir) {
			set_721(jpath_table_l + jpath_table_index,
				dpnt->parent->jpath_index);
			set_722(jpath_table_m + jpath_table_index,
				dpnt->parent->jpath_index);
			jpindex = dpnt->parent->jpath_index;
		} else {
			set_721(jpath_table_l + jpath_table_index,
				dpnt->self->parent_rec->filedir->jpath_index);
			set_722(jpath_table_m + jpath_table_index,
				dpnt->self->parent_rec->filedir->jpath_index);
			jpindex = dpnt->self->parent_rec->filedir->jpath_index;
		}

		if (jpindex > 0xffff) {
			static int warned = 0;

			if (!warned) {
				warned++;
				errmsgno(EX_BAD,
			_("Unable to generate sane Joliet path tables - too many directories (%u)\n"),
					jpindex);
				if (!nolimitpathtables)
					errmsgno(EX_BAD,
					_("Try to use the option -no-limit-pathtables\n"));
			}
			if (!nolimitpathtables)
				exit(EX_BAD);
			/*
			 * Let it point to the root directory instead.
			 */
			set_721(jpath_table_l + jpath_table_index, 1);
			set_722(jpath_table_m + jpath_table_index, 1);
		}

		jpath_table_index += 2;

		/*
		 * The root directory is still represented in non-unicode
		 * fashion.
		 */
		if (dpnt == root) {
			jpath_table_l[jpath_table_index] = 0;
			jpath_table_m[jpath_table_index] = 0;
			jpath_table_index++;
		} else {
#ifdef APPLE_HYB
			if (USE_MAC_NAME(de)) {
				convert_to_unicode((Uchar *) jpath_table_l +
					jpath_table_index,
					namelen, de->hfs_ent->name, hfs_inls);
				convert_to_unicode((Uchar *) jpath_table_m +
					jpath_table_index,
					namelen, de->hfs_ent->name, hfs_inls);
			} else {
#endif	/* APPLE_HYB */
				convert_to_unicode((Uchar *) jpath_table_l +
					jpath_table_index,
					namelen, de->name, in_nls);
				convert_to_unicode((Uchar *) jpath_table_m +
					jpath_table_index,
					namelen, de->name, in_nls);
#ifdef APPLE_HYB
			}
#endif	/* APPLE_HYB */

			jpath_table_index += namelen;
		}

		if (jpath_table_index & 1) {
			jpath_table_index++;	/* For odd lengths we pad */
		}
	}

	free(jpathlist);
	if (jpath_table_index != jpath_table_size) {
		errmsgno(EX_BAD,
		_("Joliet path table lengths do not match %d expected: %d\n"),
			jpath_table_index,
			jpath_table_size);
	}
	return (0);
} /* generate_path_tables(... */

LOCAL void
generate_one_joliet_directory(dpnt, outfile)
	struct directory	*dpnt;
	FILE			*outfile;
{
	unsigned int		dir_index;
	char			*directory_buffer;
	int			new_reclen;
	struct directory_entry *s_entry;
	struct directory_entry *s_entry1;
	struct iso_directory_record jrec;
	unsigned int	total_size;
	int			cvt_len;
	struct directory	*finddir;

	total_size = ISO_ROUND_UP(dpnt->jsize);
	directory_buffer = (char *)e_malloc(total_size);
	memset(directory_buffer, 0, total_size);
	dir_index = 0;

	s_entry = dpnt->jcontents;
	while (s_entry) {
		if (s_entry->de_flags & INHIBIT_JOLIET_ENTRY) {
			s_entry = s_entry->jnext;
			continue;
		}
		/*
		 * If this entry was a directory that was relocated,
		 * we have a bit of trouble here.  We need to dig out the real
		 * thing and put it back here.  In the Joliet tree, there is
		 * no relocated rock ridge, as there are no depth limits to a
		 * directory tree.
		 */
		if ((s_entry->de_flags & RELOCATED_DIRECTORY) != 0) {
			for (s_entry1 = reloc_dir->contents; s_entry1;
						s_entry1 = s_entry1->next) {
				if (s_entry1->parent_rec == s_entry) {
					break;
				}
			}
			if (s_entry1 == NULL) {
				/* We got trouble. */
				comerrno(EX_BAD,
				_("Unable to locate relocated directory\n"));
			}
		} else {
			s_entry1 = s_entry;
		}

		/*
		 * We do not allow directory entries to cross sector
		 * boundaries. Simply pad, and then start the next entry at
		 * the next sector
		 */
		new_reclen = s_entry1->jreclen;
		if ((dir_index & (SECTOR_SIZE - 1)) + new_reclen >= SECTOR_SIZE) {
			dir_index = ISO_ROUND_UP(dir_index);
		}
		memcpy(&jrec, &s_entry1->isorec, offsetof(struct iso_directory_record, name[0]));

#ifdef APPLE_HYB
		/* Use the HFS name if it exists */
		if (USE_MAC_NAME(s_entry1))
			cvt_len = joliet_strlen(s_entry1->hfs_ent->name, jlen, hfs_inls);
		else
#endif	/* APPLE_HYB */
			cvt_len = joliet_strlen(s_entry1->name, jlen, in_nls);

		/*
		 * Fix the record length
		 * - this was the non-Joliet version we were seeing.
		 */
		jrec.name_len[0] = cvt_len;
		jrec.length[0] = s_entry1->jreclen;

		/*
		 * If this is a directory,
		 * fix the correct size and extent number.
		 */
		if ((jrec.flags[0] & ISO_DIRECTORY) != 0) {
			if (strcmp(s_entry1->name, ".") == 0) {
				jrec.name_len[0] = 1;
				set_733((char *)jrec.extent, dpnt->jextent);
				set_733((char *)jrec.size, ISO_ROUND_UP(dpnt->jsize));
			} else if (strcmp(s_entry1->name, "..") == 0) {
				jrec.name_len[0] = 1;
				if (dpnt->parent == reloc_dir) {
					set_733((char *)jrec.extent, dpnt->self->parent_rec->filedir->jextent);
					set_733((char *)jrec.size, ISO_ROUND_UP(dpnt->self->parent_rec->filedir->jsize));
				} else {
					set_733((char *)jrec.extent, dpnt->parent->jextent);
					set_733((char *)jrec.size, ISO_ROUND_UP(dpnt->parent->jsize));
				}
			} else {
				if ((s_entry->de_flags & RELOCATED_DIRECTORY) != 0) {
					finddir = reloc_dir->subdir;
				} else {
					finddir = dpnt->subdir;
				}
				while (finddir && finddir->self != s_entry1) {
					finddir = finddir->next;
				}
				if (!finddir) {
					comerrno(EX_BAD,
						_("Fatal goof - unable to find directory location\n"));
				}
				set_733((char *)jrec.extent, finddir->jextent);
				set_733((char *)jrec.size,
						ISO_ROUND_UP(finddir->jsize));
			}
		}
		memcpy(directory_buffer + dir_index, &jrec,
			offsetof(struct iso_directory_record, name[0]));

		dir_index += offsetof(struct iso_directory_record, name[0]);

		/*
		 * Finally dump the Unicode version of the filename.
		 * Note - . and .. are the same as with non-Joliet discs.
		 */
		if ((jrec.flags[0] & ISO_DIRECTORY) != 0 &&
			strcmp(s_entry1->name, ".") == 0) {
			directory_buffer[dir_index++] = 0;
		} else if ((jrec.flags[0] & ISO_DIRECTORY) != 0 &&
			strcmp(s_entry1->name, "..") == 0) {
			directory_buffer[dir_index++] = 1;
		} else {
#ifdef APPLE_HYB
			if (USE_MAC_NAME(s_entry1)) {
				/* Use the HFS name if it exists */
				convert_to_unicode(
					(Uchar *) directory_buffer+dir_index,
					cvt_len,
					s_entry1->hfs_ent->name, hfs_inls);
			} else
#endif	/* APPLE_HYB */
			{
				convert_to_unicode(
					(Uchar *) directory_buffer+dir_index,
					cvt_len,
					s_entry1->name, in_nls);
			}
			dir_index += cvt_len;
		}

		if (dir_index & 1) {
			directory_buffer[dir_index++] = 0;
		}
		s_entry = s_entry->jnext;
	}

	if (dpnt->jsize != dir_index) {
		errmsgno(EX_BAD,
		_("Unexpected joliet directory length %d expected: %d '%s'\n"),
			dpnt->jsize,
			dir_index, dpnt->de_name);
	}
	xfwrite(directory_buffer, total_size, 1, outfile, 0, FALSE);
	last_extent_written += total_size >> 11;
	free(directory_buffer);
} /* generate_one_joliet_directory(... */

LOCAL int
joliet_sort_n_finish(this_dir)
	struct directory	*this_dir;
{
	struct directory_entry	*s_entry;
	int			status = 0;

	/*
	 * don't want to skip this directory if it's the reloc_dir
	 * at the moment
	 */
	if (this_dir != reloc_dir &&
				this_dir->dir_flags & INHIBIT_JOLIET_ENTRY) {
		return (0);
	}
	for (s_entry = this_dir->contents; s_entry; s_entry = s_entry->next) {
		/* skip hidden entries */
		if ((s_entry->de_flags & INHIBIT_JOLIET_ENTRY) != 0) {
			continue;
		}
		/*
		 * First update the path table sizes for directories.
		 *
		 * Finally, set the length of the directory entry if Joliet is
		 * used. The name is longer, but no Rock Ridge is ever used
		 * here, so depending upon the options the entry size might
		 * turn out to be about the same.  The Unicode name is always
		 * a multiple of 2 bytes, so we always add 1 to make it an
		 * even number.
		 */
		if (s_entry->isorec.flags[0] & ISO_DIRECTORY) {
			if (strcmp(s_entry->name, ".") != 0 &&
					strcmp(s_entry->name, "..") != 0) {
#ifdef APPLE_HYB
				if (USE_MAC_NAME(s_entry))
					/* Use the HFS name if it exists */
					jpath_table_size +=
						joliet_strlen(s_entry->hfs_ent->name, jlen, hfs_inls) +
						offsetof(struct iso_path_table, name[0]);
				else
#endif	/* APPLE_HYB */
					jpath_table_size +=
						joliet_strlen(s_entry->name, jlen, in_nls) +
						offsetof(struct iso_path_table, name[0]);
				if (jpath_table_size & 1) {
					jpath_table_size++;
				}
			} else {
				if (this_dir == root &&
						strlen(s_entry->name) == 1) {

					jpath_table_size += 1 + offsetof(struct iso_path_table, name[0]);
					if (jpath_table_size & 1)
						jpath_table_size++;
				}
			}
		}
		if (strcmp(s_entry->name, ".") != 0 &&
					strcmp(s_entry->name, "..") != 0) {
#ifdef APPLE_HYB
			if (USE_MAC_NAME(s_entry))
				/* Use the HFS name if it exists */
				s_entry->jreclen =
				offsetof(struct iso_directory_record, name[0])
					+ joliet_strlen(s_entry->hfs_ent->name, jlen, hfs_inls)
					+ 1;
			else
#endif	/* APPLE_HYB */
				s_entry->jreclen =
				offsetof(struct iso_directory_record, name[0])
					+ joliet_strlen(s_entry->name, jlen, in_nls)
					+ 1;
		} else {
			/*
			 * Special - for '.' and '..' we generate the same
			 * records we did for non-Joliet discs.
			 */
			s_entry->jreclen =
			offsetof(struct iso_directory_record, name[0])
				+ 1;
		}


	}

	if ((this_dir->dir_flags & INHIBIT_JOLIET_ENTRY) != 0) {
		return (0);
	}
	this_dir->jcontents = this_dir->contents;
	status = joliet_sort_directory(&this_dir->jcontents);

	/*
	 * Now go through the directory and figure out how large this one will
	 * be. Do not split a directory entry across a sector boundary
	 */
	s_entry = this_dir->jcontents;
	/*
	 * XXX Is it ok to comment this out?
	 */
/*XXX JS  this_dir->ce_bytes = 0;*/
	for (s_entry = this_dir->jcontents; s_entry;
						s_entry = s_entry->jnext) {
		int	jreclen;

		if ((s_entry->de_flags & INHIBIT_JOLIET_ENTRY) != 0) {
			continue;
		}
		jreclen = s_entry->jreclen;

		if ((this_dir->jsize & (SECTOR_SIZE - 1)) + jreclen >=
								SECTOR_SIZE) {
			this_dir->jsize = ISO_ROUND_UP(this_dir->jsize);
		}
		this_dir->jsize += jreclen;
	}
	return (status);
}

/*
 * Similar to the iso9660 case,
 * except here we perform a full sort based upon the
 * regular name of the file, not the 8.3 version.
 */
LOCAL int
joliet_compare_dirs(rr, ll)
	const void	*rr;
	const void	*ll;
{
	char		*rpnt,
			*lpnt;
	struct directory_entry **r,
			**l;
	unsigned char	rtmp[2],
			ltmp[2];
	siconvt_t	*linls, *rinls;

	r = (struct directory_entry **)rr;
	l = (struct directory_entry **)ll;

#ifdef APPLE_HYB
	/*
	 * we may be using the HFS name - so select the correct input
	 * charset
	 */
	if (USE_MAC_NAME(*r)) {
		rpnt = (*r)->hfs_ent->name;
		rinls = hfs_inls;
	} else {
		rpnt = (*r)->name;
		rinls = in_nls;
	}

	if (USE_MAC_NAME(*l)) {
		lpnt = (*l)->hfs_ent->name;
		linls = hfs_inls;
	} else {
		lpnt = (*l)->name;
		linls = in_nls;
	}
#else
	rpnt = (*r)->name;
	lpnt = (*l)->name;
	rinls = linls = in_nls;
#endif	/* APPLE_HYB */

	/*
	 * If the entries are the same, this is an error.
	 * Joliet specs allow for a maximum of 64 characters.
	 * If we see different multi extent parts, it is OK to
	 * have the same name more than once.
	 */
	if (strncmp(rpnt, lpnt, jlen) == 0) {
#ifdef USE_LARGEFILES
		if ((*r)->mxpart == (*l)->mxpart)
#endif
		{
			errmsgno(EX_BAD,
				_("Error: %s and %s have the same Joliet name\n"),
				(*r)->whole_name, (*l)->whole_name);
			jsort_goof++;
			{
				char	*p1 = rpnt;
				char	*p2 = lpnt;
				int	len = 0;

				for (; *p1 == *p2; p1++, p2++, len++) {
					if (*p1 == '\0')
						break;
				}
				if (len > jsort_glen)
					jsort_glen = len;
			}
		}
	}
	/*
	 * Put the '.' and '..' entries on the head of the sorted list.
	 * For normal ASCII, this always happens to be the case, but out of
	 * band characters cause this not to be the case sometimes.
	 */
	if (strcmp(rpnt, ".") == 0)
		return (-1);
	if (strcmp(lpnt, ".") == 0)
		return (1);

	if (strcmp(rpnt, "..") == 0)
		return (-1);
	if (strcmp(lpnt, "..") == 0)
		return (1);

#ifdef DVD_AUD_VID
	/*
	 * There're rumors claiming that some players assume VIDEO_TS.IFO
	 * to be the first file in VIDEO_TS/ catalog. Well, it's basically
	 * the only file a player has to actually look for, as the whole
	 * video content can be "rolled down" from this file alone.
	 *				<appro@fy.chalmers.se>
	 */
	/*
	 * XXX This code has to be moved from the Joliet implementation
	 * XXX to the UDF implementation if we implement decent UDF support
	 * XXX with a separate name space for the UDF file tree.
	 */
	if (dvd_aud_vid_flag & DVD_SPEC_VIDEO) {
		if (strcmp(rpnt, "VIDEO_TS.IFO") == 0)
			return (-1);
		if (strcmp(lpnt, "VIDEO_TS.IFO") == 0)
			return (1);
	}
#endif

	while (*rpnt && *lpnt) {
		if (*rpnt == ';' && *lpnt != ';')
			return (-1);
		if (*rpnt != ';' && *lpnt == ';')
			return (1);

		if (*rpnt == ';' && *lpnt == ';')
			return (0);

		/*
		 * Extensions are not special here.
		 * Don't treat the dot as something that must be bumped to
		 * the start of the list.
		 */
#if 0
		if (*rpnt == '.' && *lpnt != '.')
			return (-1);
		if (*rpnt != '.' && *lpnt == '.')
			return (1);
#endif

		convert_to_unicode(rtmp, 2, rpnt, rinls);
		convert_to_unicode(ltmp, 2, lpnt, linls);

		if (a_to_u_2_byte(rtmp) < a_to_u_2_byte(ltmp))
			return (-1);
		if (a_to_u_2_byte(rtmp) > a_to_u_2_byte(ltmp))
			return (1);

		rpnt++;
		lpnt++;
	}
	if (*rpnt)
		return (1);
	if (*lpnt)
		return (-1);
#ifdef USE_LARGEFILES
	/*
	 * (*r)->mxpart == (*l)->mxpart cannot happen here
	 */
	if ((*r)->mxpart < (*l)->mxpart)
		return (-1);
	else if ((*r)->mxpart > (*l)->mxpart)
		return (1);
#endif
	return (0);
}


/*
 * Function:		sort_directory
 *
 * Purpose:		Sort the directory in the appropriate ISO9660
 *			order.
 *
 * Notes:		Returns 0 if OK, returns > 0 if an error occurred.
 */
LOCAL int
joliet_sort_directory(sort_dir)
	struct directory_entry	**sort_dir;
{
	int			dcount = 0;
	int			i;
	struct directory_entry	*s_entry;
	struct directory_entry	**sortlist;

	s_entry = *sort_dir;
	while (s_entry) {
		/*
		 * only colletc non-hidden entries
		 */
		if ((s_entry->de_flags & (INHIBIT_JOLIET_ENTRY|INHIBIT_UDF_ENTRY)) !=
					(INHIBIT_JOLIET_ENTRY|INHIBIT_UDF_ENTRY))
			dcount++;
		s_entry = s_entry->next;
	}

	/* OK, now we know how many there are.  Build a vector for sorting. */
	sortlist = (struct directory_entry **)
		e_malloc(sizeof (struct directory_entry *) * dcount);

	dcount = 0;
	s_entry = *sort_dir;
	while (s_entry) {
		/*
		 * only collect non-hidden entries
		 */
		if ((s_entry->de_flags & (INHIBIT_JOLIET_ENTRY|INHIBIT_UDF_ENTRY)) !=
					(INHIBIT_JOLIET_ENTRY|INHIBIT_UDF_ENTRY)) {
			sortlist[dcount] = s_entry;
			dcount++;
		}
		s_entry = s_entry->next;
	}

	jsort_goof = 0;
	jsort_glen = 0;
#ifdef	PROTOTYPES
	qsort(sortlist, dcount, sizeof (struct directory_entry *),
		(int (*) (const void *, const void *)) joliet_compare_dirs);
#else
	qsort(sortlist, dcount, sizeof (struct directory_entry *),
		joliet_compare_dirs);
#endif

	if (jsort_goof) {
		errmsgno(EX_BAD,
			_("Joliet file names differ after %d chars\n"),
			jsort_glen);
		if (jsort_glen > JLONGMAX) {
			errmsgno(EX_BAD,
			_("Cannot use Joliet, please remove -J from the option list.\n"));
		} else if (jsort_glen > JMAX) {
			errmsgno(EX_BAD,
			_("Try to use the option -joliet-long\n"));
		}
	}

	/* Now reassemble the linked list in the proper sorted order */
	for (i = 0; i < dcount - 1; i++) {
		sortlist[i]->jnext = sortlist[i + 1];
	}

	sortlist[dcount - 1]->jnext = NULL;
	*sort_dir = sortlist[0];

	free(sortlist);
	return (jsort_goof);
}

EXPORT int
joliet_sort_tree(node)
	struct directory	*node;
{
	struct directory	*dpnt;
	int			ret = 0;

	dpnt = node;

	while (dpnt) {
		ret = joliet_sort_n_finish(dpnt);
		if (ret) {
			break;
		}
		if (dpnt->subdir)
			ret = joliet_sort_tree(dpnt->subdir);
		if (ret) {
			break;
		}
		dpnt = dpnt->next;
	}
	return (ret);
}

LOCAL void
generate_joliet_directories(node, outfile)
	struct directory	*node;
	FILE			*outfile;
{
	struct directory *dpnt;

	dpnt = node;

	while (dpnt) {
		if ((dpnt->dir_flags & INHIBIT_JOLIET_ENTRY) == 0) {
			/*
			 * In theory we should never reuse a directory, so this
			 * doesn't make much sense.
			 */
			if (dpnt->jextent > session_start) {
				generate_one_joliet_directory(dpnt, outfile);
			}
		}
		/* skip if hidden - but not for the rr_moved dir */
		if (dpnt->subdir &&
		    (!(dpnt->dir_flags & INHIBIT_JOLIET_ENTRY) ||
		    dpnt == reloc_dir)) {
			generate_joliet_directories(dpnt->subdir, outfile);
		}
		dpnt = dpnt->next;
	}
}


/*
 * Function to write the EVD for the disc.
 */
LOCAL int
jpathtab_write(outfile)
	FILE	*outfile;
{
	/* Next we write the path tables */
	xfwrite(jpath_table_l, jpath_blocks << 11, 1, outfile, 0, FALSE);
	xfwrite(jpath_table_m, jpath_blocks << 11, 1, outfile, 0, FALSE);
	last_extent_written += 2 * jpath_blocks;
	free(jpath_table_l);
	free(jpath_table_m);
	jpath_table_l = NULL;
	jpath_table_m = NULL;
	return (0);
}

LOCAL int
jdirtree_size(starting_extent)
	UInt32_t	starting_extent;
{
	assign_joliet_directory_addresses(root);
	return (0);
}

LOCAL int
jroot_gen()
{
	jroot_record.length[0] =
			1 + offsetof(struct iso_directory_record, name[0]);
	jroot_record.ext_attr_length[0] = 0;
	set_733((char *)jroot_record.extent, root->jextent);
	set_733((char *)jroot_record.size, ISO_ROUND_UP(root->jsize));
	iso9660_date(jroot_record.date, root_statbuf.st_mtime);
	jroot_record.flags[0] = ISO_DIRECTORY;
	jroot_record.file_unit_size[0] = 0;
	jroot_record.interleave[0] = 0;
	set_723(jroot_record.volume_sequence_number, volume_sequence_number);
	jroot_record.name_len[0] = 1;
	return (0);
}

LOCAL int
jdirtree_write(outfile)
	FILE	*outfile;
{
	generate_joliet_directories(root, outfile);
	return (0);
}

/*
 * Function to write the EVD for the disc.
 */
LOCAL int
jvd_write(outfile)
	FILE	*outfile;
{
	struct iso_primary_descriptor jvol_desc;

	/* Next we write out the boot volume descriptor for the disc */
	jvol_desc = vol_desc;
	get_joliet_vol_desc(&jvol_desc);
	xfwrite(&jvol_desc, SECTOR_SIZE, 1, outfile, 0, FALSE);
	last_extent_written++;
	return (0);
}

/*
 * Functions to describe padding block at the start of the disc.
 */
LOCAL int
jpathtab_size(starting_extent)
	UInt32_t	starting_extent;
{
	jpath_table[0] = starting_extent;
	jpath_table[1] = 0;
	jpath_table[2] = jpath_table[0] + jpath_blocks;
	jpath_table[3] = 0;

	last_extent += 2 * jpath_blocks;
	return (0);
}

struct output_fragment joliet_desc = {NULL, oneblock_size, jroot_gen, jvd_write, "Joliet Volume Descriptor" };
struct output_fragment jpathtable_desc = {NULL, jpathtab_size, generate_joliet_path_tables, jpathtab_write, "Joliet path table" };
struct output_fragment jdirtree_desc = {NULL, jdirtree_size, NULL, jdirtree_write, "Joliet directory tree" };
