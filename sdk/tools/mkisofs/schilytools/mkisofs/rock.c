/* @(#)rock.c	1.66 12/12/02 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)rock.c	1.66 12/12/02 joerg";
#endif
/*
 * File rock.c - generate RRIP  records for iso9660 filesystems.
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999,2000-2012 J. Schilling
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
#include "rock.h"
#include <schily/device.h>
#include <schily/schily.h>

#define	SU_VERSION 1

#define	SL_ROOT    8
#define	SL_PARENT  4
#define	SL_CURRENT 2
#define	SL_CONTINUE 1

#define	CE_SIZE 28	/* SUSP	Continuation aerea			*/
#define	CL_SIZE 12	/* RR	Child Link for deep dir relocation	*/
#define	ER_SIZE 8	/* RR	Extension record for RR signature	*/
#define	NM_SIZE 5	/* RR	Real name				*/
#define	PL_SIZE 12	/* RR	Paren Link for deep dir relocation	*/
#define	PN_SIZE 20	/* RR	POSIX device modes (Major/Minor)	*/
#define	PX_OLD_SIZE 36	/* RR	POSIX Extensions (mode/nlink/uid/gid)	*/
#define	PX_SIZE 44	/* RR	POSIX Extensions (mode/nlink/uid/gid/ino) */
#define	RE_SIZE 4	/* RR	Relocated directory			*/
#define	RR_SIZE 5	/* RR	RR Signature in every file		*/
#define	SL_SIZE 20	/* RR	Symlink					*/
#define	ZF_SIZE 16	/* RR*	Linux compression extension		*/
#ifdef APPLE_HYB
#define	AA_SIZE 14	/* size of Apple extension */
#endif	/* APPLE_HYB */
#if defined(__QNX__) && !defined(__QNXNTO__)	/* Not on Neutrino! never OK? */
#define	TF_SIZE (5 + 4 * 7)	/* RR	Time field			*/
#define	TF_SIZE_LONG (5 + 4 * 17) /* RR	Time field			*/
#else
#define	TF_SIZE (5 + 3 * 7)
#define	TF_SIZE_LONG (5 + 3 * 17)
#endif

LOCAL	void	rstrncpy			__PR((char *t, char *f, size_t tlen,
							siconvt_t *inls,
							siconvt_t *onls));
LOCAL	void	add_CE_entry			__PR((char *field, int line));
LOCAL	int	gen_xa_attr			__PR((mode_t attr));
LOCAL	void	gen_xa				__PR((struct stat *lstatbuf));
EXPORT	int	generate_xa_rr_attributes	__PR((char *whole_name,
							char *name,
							struct directory_entry *s_entry,
							struct stat *statbuf,
							struct stat *lstatbuf,
							int deep_opt));
	char	*generate_rr_extension_record	__PR((char *id,
							char *descriptor,
							char *source,
							int *size));
/*
 * If we need to store this number of bytes, make sure we
 * do not box ourselves in so that we do not have room for
 * a CE entry for the continuation record
 */
#define	RR_CUR_USE	(CE_SIZE + currlen + (ipnt - recstart))

#define	MAYBE_ADD_CE_ENTRY(BYTES) \
	(((int)(BYTES)) + CE_SIZE + currlen + (ipnt - recstart) > reclimit ? 1 : 0)

/*
 * Buffer to build RR attributes
 */
LOCAL	Uchar	Rock[16384];
LOCAL	Uchar	symlink_buff[PATH_MAX+1];
LOCAL	int	ipnt = 0;	/* Current "write" offset in Rock[]	*/
LOCAL	int	recstart = 0;	/* Start offset in Rock[] for this area	*/
LOCAL	int	currlen = 0;	/* # of non RR bytes used in this area	*/
LOCAL	int	mainrec = 0;	/* # of RR bytes use in main dir area	*/
LOCAL	int	reclimit;	/* Max. # of bytes usable in this area	*/

/* if we are using converted filenames, we don't want the '/' character */
LOCAL void
rstrncpy(t, f, tlen, inls, onls)
	char	*t;
	char	*f;
	size_t	tlen;		/* The to-length */
	siconvt_t *inls;
	siconvt_t *onls;
{
	size_t	flen = strlen(f);

	while (tlen > 0 && *f) {
		size_t	ofl = flen;
		size_t	otl = tlen;

		conv_charset((Uchar *)t, &tlen, (Uchar *)f, &flen, inls, onls);
		if (*t == '/') {
			*t = '_';
		}
		t += otl - tlen;
		f += ofl - flen;
	}
}

LOCAL void
add_CE_entry(field, line)
	char	*field;
	int	line;
{
	if (MAYBE_ADD_CE_ENTRY(0)) {
		errmsgno(EX_BAD,
		_("Panic: no space, cannot add RR CE entry (%d bytes mising) for %s line %d.\n"),
		(CE_SIZE + currlen + (ipnt - recstart) - reclimit),
		field, line);
		errmsgno(EX_BAD, _("currlen: %d ipnt: %d, recstart: %d\n"),
				currlen, ipnt, recstart);
		errmsgno(EX_BAD, _("Send  bug report to the maintainer.\n"));
		comerrno(EX_BAD, _("Aborting.\n"));
	}

	if (recstart)
		set_733((char *)Rock + recstart - 8, ipnt + 28 - recstart);
	Rock[ipnt++] = 'C';
	Rock[ipnt++] = 'E';
	Rock[ipnt++] = CE_SIZE;
	Rock[ipnt++] = SU_VERSION;
	set_733((char *)Rock + ipnt, 0);
	ipnt += 8;
	set_733((char *)Rock + ipnt, 0);
	ipnt += 8;
	set_733((char *)Rock + ipnt, 0);
	ipnt += 8;
	recstart = ipnt;
	currlen = 0;
	if (!mainrec)
		mainrec = ipnt;
	reclimit = SECTOR_SIZE - 8;	/* Limit to one sector */
}

#ifdef	PROTOTYPES
LOCAL int
gen_xa_attr(mode_t attr)
#else
LOCAL int
gen_xa_attr(attr)
	mode_t	attr;
#endif
{
	int	ret = 0;

	if (attr & S_IRUSR)
		ret |= XA_O_READ;
	if (attr & S_IXUSR)
		ret |= XA_O_EXEC;

	if (attr & S_IRGRP)
		ret |= XA_G_READ;
	if (attr & S_IXGRP)
		ret |= XA_G_EXEC;

	if (attr & S_IROTH)
		ret |= XA_W_READ;
	if (attr & S_IXOTH)
		ret |= XA_W_EXEC;

	ret |= XA_FORM1;

	if (S_ISDIR(attr))
		ret |= XA_DIR;

	return (ret);
}

LOCAL void
gen_xa(lstatbuf)
	struct stat	*lstatbuf;
{
		/*
		 * Group ID
		 */
		set_722((char *)Rock + ipnt, lstatbuf->st_gid);
		ipnt += 2;
		/*
		 * User ID
		 */
		set_722((char *)Rock + ipnt, lstatbuf->st_uid);
		ipnt += 2;
		/*
		 * Attributes
		 */
		set_722((char *)Rock + ipnt, gen_xa_attr(lstatbuf->st_mode));
		ipnt += 2;

		Rock[ipnt++] = 'X';	/* XA Signature */
		Rock[ipnt++] = 'A';
		Rock[ipnt++] = 0;	/* File number (we always use '0' */

		Rock[ipnt++] = 0;	/* Reserved (5 Byte) */
		Rock[ipnt++] = 0;
		Rock[ipnt++] = 0;
		Rock[ipnt++] = 0;
		Rock[ipnt++] = 0;

}

#ifdef PROTOTYPES
EXPORT int
generate_xa_rr_attributes(char *whole_name, char *name,
			struct directory_entry *s_entry,
			struct stat *statbuf,
			struct stat *lstatbuf,
			int deep_opt)
#else
EXPORT int
generate_xa_rr_attributes(whole_name, name,
			s_entry,
			statbuf,
			lstatbuf,
			deep_opt)
	char		*whole_name;
	char		*name;
	struct directory_entry *s_entry;
	struct stat	*statbuf,
			*lstatbuf;
	int		deep_opt;
#endif
{
	int		flagpos;
	int		flagval;
	int		need_ce;

	statbuf = statbuf;	/* this shuts up unreferenced compiler */
				/* warnings */
	mainrec = recstart = ipnt = 0;

	if (use_XA) {
		gen_xa(lstatbuf);
	}

/*	reclimit = 0xf8; XXX we now use 254 == 0xfe */
	reclimit = MAX_ISODIR;

	/* no need to fill in the RR stuff if we won't see the file */
	if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY)
		return (0);

	/*
	 * Obtain the amount of space that is currently used for the directory
	 * record.  We may safely use the current name length; because if name
	 * confilcts force us to change the ISO-9660 name later, the name will
	 * never become longer than now.
	 */
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		s_entry->isorec.name_len[0] = 1;
	} else {
		s_entry->isorec.name_len[0] = strlen(s_entry->isorec.name);
	}
	currlen = s_entry->isorec.length[0] = s_entry->isorec.name_len[0] +
				offsetof(struct iso_directory_record, name[0]);
	if (currlen & 1)
		s_entry->isorec.length[0] = ++currlen;

	if (currlen < 33+37) {
		/*
		 * If the ISO-9660 name length is less than 37, we may use
		 * ISO-9660:1988 name rules and for this reason, the name len
		 * may later increase from adding e.g. ".;1"; in this case
		 * just use the upper limit.
		 */
		currlen = 33+37;
	}

#ifdef APPLE_HYB
	/* if we have regular file, then add Apple extensions */
	if (S_ISREG(lstatbuf->st_mode) && apple_ext && s_entry->hfs_ent) {
		if (MAYBE_ADD_CE_ENTRY(AA_SIZE))
			add_CE_entry("AA", __LINE__);
		Rock[ipnt++] = 'A';	/* AppleSignature */
		Rock[ipnt++] = 'A';
		Rock[ipnt++] = AA_SIZE;	/* includes AppleSignature bytes */
		Rock[ipnt++] = 0x02;	/* SystemUseID */
		Rock[ipnt++] = s_entry->hfs_ent->u.file.type[0];
		Rock[ipnt++] = s_entry->hfs_ent->u.file.type[1];
		Rock[ipnt++] = s_entry->hfs_ent->u.file.type[2];
		Rock[ipnt++] = s_entry->hfs_ent->u.file.type[3];
		Rock[ipnt++] = s_entry->hfs_ent->u.file.creator[0];
		Rock[ipnt++] = s_entry->hfs_ent->u.file.creator[1];
		Rock[ipnt++] = s_entry->hfs_ent->u.file.creator[2];
		Rock[ipnt++] = s_entry->hfs_ent->u.file.creator[3];
		Rock[ipnt++] = (s_entry->hfs_ent->fdflags >> 8) & 0xff;
		Rock[ipnt++] = s_entry->hfs_ent->fdflags & 0xff;
	}
#endif	/* APPLE_HYB */

	if (!use_RockRidge)
		goto xa_only;

	/* Identify that we are using the SUSP protocol */
	if (deep_opt & NEED_SP) {
		/*
		 * We may not use a CE record here but we never will need to
		 * do so, as this SP record is only used for the "." entry
		 * of the root directory.
		 */
		Rock[ipnt++] = 'S';
		Rock[ipnt++] = 'P';
		Rock[ipnt++] = 7;
		Rock[ipnt++] = SU_VERSION;
		Rock[ipnt++] = 0xbe;
		Rock[ipnt++] = 0xef;
		if (use_XA)
			Rock[ipnt++] = sizeof (struct iso_xa_dir_record);
		else
			Rock[ipnt++] = 0;
	}

	/* First build the posix name field */
	if (MAYBE_ADD_CE_ENTRY(RR_SIZE))
		add_CE_entry("RR", __LINE__);
	Rock[ipnt++] = 'R';
	Rock[ipnt++] = 'R';
	Rock[ipnt++] = 5;
	Rock[ipnt++] = SU_VERSION;
	flagpos = ipnt;
	flagval = 0;
	Rock[ipnt++] = 0;	/* We go back and fix this later */

	if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0) {
		char		*npnt;
		int		remain;	/* Remaining name length  */
		int		use;	/* Current name part used */

#ifdef APPLE_HYB
		/* use the HFS name if it exists */
		if (USE_MAC_NAME(s_entry)) {
			remain = strlen(s_entry->hfs_ent->name);
			npnt = s_entry->hfs_ent->name;
		} else {
#endif	/* APPLE_HYB */

			remain = strlen(name);
			npnt = name;
#ifdef APPLE_HYB
		}
#endif	/* APPLE_HYB */

		if (MAYBE_ADD_CE_ENTRY(NM_SIZE+1))
			add_CE_entry("NM", __LINE__);
		while (remain) {
			use = remain;
			need_ce = 0;
			/* Can we fit this SUSP and a CE entry? */
			if (MAYBE_ADD_CE_ENTRY(NM_SIZE+use)) {
				use = reclimit - NM_SIZE - RR_CUR_USE;
				need_ce++;
			}
			/* Only room for 256 per SUSP field */
			if (use > 0xf8) {
				use = 0xf8;
				need_ce++;
			}
			if (use < 0) {
				comerrno(EX_BAD,
				_("Negative RR name length residual: %d\n"),
					use);
			}

			/* First build the posix name field */
			Rock[ipnt++] = 'N';
			Rock[ipnt++] = 'M';
			Rock[ipnt++] = NM_SIZE + use;
			Rock[ipnt++] = SU_VERSION;
			Rock[ipnt++] = (remain != use ? 1 : 0);
			flagval |= (1 << 3);

			/*
			 * Convert charsets as required
			 * XXX If we are using iconv() based locales and in/out
			 * XXX locale differ, then strlen(&Rock[ipnt]) may
			 * XXX increase. We would need to compute the length
			 * XXX early enough.
			 */
#ifdef APPLE_HYB
			if (USE_MAC_NAME(s_entry))
				rstrncpy((char *)&Rock[ipnt], npnt, use,
							hfs_inls, out_nls);
			else
#endif	/* APPLE_HYB */
				rstrncpy((char *)&Rock[ipnt], npnt, use,
							in_nls, out_nls);
			npnt += use;
			ipnt += use;
			remain -= use;
			if (remain && need_ce)
				add_CE_entry("NM", __LINE__);
		}
	}

	/* Add the posix modes */
	if (MAYBE_ADD_CE_ENTRY(PX_SIZE))
		add_CE_entry("PX", __LINE__);
	Rock[ipnt++] = 'P';
	Rock[ipnt++] = 'X';
	if (rrip112) {
		Rock[ipnt++] = PX_SIZE;
	} else {
		Rock[ipnt++] = PX_OLD_SIZE;
	}
	Rock[ipnt++] = SU_VERSION;
	flagval |= (1 << 0);
	set_733((char *)Rock + ipnt, lstatbuf->st_mode);
	ipnt += 8;
	set_733((char *)Rock + ipnt, lstatbuf->st_nlink);
	ipnt += 8;
	set_733((char *)Rock + ipnt, lstatbuf->st_uid);
	ipnt += 8;
	set_733((char *)Rock + ipnt, lstatbuf->st_gid);
	ipnt += 8;
	if (rrip112) {
		/*
		 * We will set up correct inode numbers later
		 * after we did assign them.
		 */
		set_733((char *)Rock + ipnt, 0);
		ipnt += 8;
	}

	/* Check for special devices */
#if	defined(S_IFCHR) || defined(S_IFBLK)
	/*
	 * The code in this if statement used to be #ifdef'd with NON_UNIXFS.
	 * But as schily/stat.h always provides the macros S_ISCHR() & S_ISBLK()
	 * and schily/device.h always provides major()/minor() it is not needed
	 * anymore.
	 */
	if (S_ISCHR(lstatbuf->st_mode) || S_ISBLK(lstatbuf->st_mode)) {
		if (MAYBE_ADD_CE_ENTRY(PN_SIZE))
			add_CE_entry("PN", __LINE__);
		Rock[ipnt++] = 'P';
		Rock[ipnt++] = 'N';
		Rock[ipnt++] = PN_SIZE;
		Rock[ipnt++] = SU_VERSION;
		flagval |= (1 << 1);
#if 1
		/* This is the new and only code which uses <schily/device.h> */
		set_733((char *)Rock + ipnt, major(lstatbuf->st_rdev));
		ipnt += 8;
		set_733((char *)Rock + ipnt, minor(lstatbuf->st_rdev));
		ipnt += 8;
#else
		/*
		 * If we don't have sysmacros.h, then we have to guess as to
		 * how best to pick apart the device number for major/minor.
		 * Note: this may very well be wrong for many systems, so it
		 * is always best to use the major/minor macros if the system
		 * supports it.
		 */
		if (sizeof (dev_t) <= 2) {
			set_733((char *)Rock + ipnt, (lstatbuf->st_rdev >> 8));
			ipnt += 8;
			set_733((char *)Rock + ipnt, lstatbuf->st_rdev & 0xff);
			ipnt += 8;
		} else if (sizeof (dev_t) <= 4) {
			set_733((char *)Rock + ipnt,
						(lstatbuf->st_rdev >> 8) >> 8);
			ipnt += 8;
			set_733((char *)Rock + ipnt,
						lstatbuf->st_rdev & 0xffff);
			ipnt += 8;
		} else {
			set_733((char *)Rock + ipnt,
						(lstatbuf->st_rdev >> 16)>>16);
			ipnt += 8;
			set_733((char *)Rock + ipnt, lstatbuf->st_rdev);
			ipnt += 8;
		}
#endif
	}
#endif	/* defined(S_IFCHR) || defined(S_IFBLK) */

	/* Check for and symbolic links.  VMS does not have these. */
#ifdef S_IFLNK
	if (S_ISLNK(lstatbuf->st_mode)) {
		int		lenpos;
		int		lenval;
		int		j0;
		int		j1;
		int		nchar;
		Uchar		*cpnt;
		Uchar		*cpnt1;
		BOOL		last_sl = FALSE; /* Don't suppress last '/' */

#ifdef	HAVE_READLINK
		nchar = readlink(deep_opt&DID_CHDIR?name:whole_name,
						(char *)symlink_buff,
						sizeof (symlink_buff)-1);
		if (nchar < 0)
			errmsg(_("Cannot read link '%s'.\n"), whole_name);
#else
		nchar = -1;
#endif	/* HAVE_READLINK */
		symlink_buff[nchar < 0 ? 0 : nchar] = 0;
		nchar = strlen((char *)symlink_buff);
		set_733(s_entry->isorec.size, 0);
		cpnt = &symlink_buff[0];
		flagval |= (1 << 2);

		if (!split_SL_field) {
			int		sl_bytes = 0;

			for (cpnt1 = cpnt; *cpnt1 != '\0'; cpnt1++) {
				if (*cpnt1 == '/') {
					sl_bytes += 4;
				} else {
					sl_bytes += 1;
				}
			}
			if (sl_bytes > 250) {
				/*
				 * the symbolic link won't fit into one
				 * SL System Use Field print an error message
				 * and continue with splited one
				 */
				fprintf(stderr,
				_("symbolic link ``%s'' to long for one SL System Use Field, splitting"),
								cpnt);
			}
			if (MAYBE_ADD_CE_ENTRY(SL_SIZE + sl_bytes))
				add_CE_entry("SL+", __LINE__);
		}
		while (nchar) {
			if (MAYBE_ADD_CE_ENTRY(SL_SIZE))
				add_CE_entry("SL", __LINE__);
			Rock[ipnt++] = 'S';
			Rock[ipnt++] = 'L';
			lenpos = ipnt;
			Rock[ipnt++] = SL_SIZE;
			Rock[ipnt++] = SU_VERSION;
			Rock[ipnt++] = 0;	/* Flags */
			lenval = 5;
			while (*cpnt || last_sl) {
				cpnt1 = (Uchar *)
						strchr((char *)cpnt, '/');
				if (cpnt1) {
					nchar--;
					*cpnt1 = 0;
				}

				/*
				 * We treat certain components in a special
				 * way.
				 */
				if (cpnt[0] == '.' && cpnt[1] == '.' &&
								cpnt[2] == 0) {
					if (MAYBE_ADD_CE_ENTRY(2)) {
						add_CE_entry("SL-parent", __LINE__);
						if (cpnt1) {
							*cpnt1 = '/';
							nchar++;
							/*
							 * A kluge so that we
							 * can restart properly
							 */
							cpnt1 = NULL;
						}
						break;
					}
					Rock[ipnt++] = SL_PARENT;
					Rock[ipnt++] = 0; /* length is zero */
					lenval += 2;
					nchar -= 2;
				} else if (cpnt[0] == '.' && cpnt[1] == 0) {
					if (MAYBE_ADD_CE_ENTRY(2)) {
						add_CE_entry("SL-current", __LINE__);
						if (cpnt1) {
							*cpnt1 = '/';
							nchar++;
							/*
							 * A kluge so that we
							 * can restart properly
							 */
							cpnt1 = NULL;
						}
						break;
					}
					Rock[ipnt++] = SL_CURRENT;
					Rock[ipnt++] = 0; /* length is zero */
					lenval += 2;
					nchar -= 1;
				} else if (cpnt[0] == 0) {
					if (MAYBE_ADD_CE_ENTRY(2)) {
						add_CE_entry("SL-root", __LINE__);
						if (cpnt1) {
							*cpnt1 = '/';
							nchar++;
							/*
							 * A kluge so that we
							 * can restart properly
							 */
							cpnt1 = NULL;
						}
						break;
					}
					if (cpnt == &symlink_buff[0])
						Rock[ipnt++] = SL_ROOT;
					else
						Rock[ipnt++] = 0;
					Rock[ipnt++] = 0; /* length is zero */
					lenval += 2;
				} else {
					/*
					 * If we do not have enough room for a
					 * component, start a new continuations
					 * segment now
					 */
					if (split_SL_component ?
						MAYBE_ADD_CE_ENTRY(6) :
						MAYBE_ADD_CE_ENTRY(6 + strlen((char *)cpnt))) {
						add_CE_entry("SL++", __LINE__);
						if (cpnt1) {
							*cpnt1 = '/';
							nchar++;
							/*
							 * A kluge so that we
							 * can restart properly
							 */
							cpnt1 = NULL;
						}
						break;
					}
					j0 = strlen((char *)cpnt);
					while (j0) {
						j1 = j0;
						if (j1 > 0xf8)
							j1 = 0xf8;
						need_ce = 0;
						if (j1 + currlen + 2 + CE_SIZE +
						    (ipnt - recstart) >
								reclimit) {

							j1 = reclimit -
							    (currlen + 2) -
							    CE_SIZE -
							    (ipnt - recstart);
							need_ce++;
						}
						Rock[ipnt++] =
							(j1 != j0 ?
							SL_CONTINUE : 0);
						Rock[ipnt++] = j1;
						strncpy((char *)Rock + ipnt,
							(char *)cpnt, j1);
						ipnt += j1;
						lenval += j1 + 2;
						cpnt += j1;
						/*
						 * Number we processed
						 * this time
						 */
						nchar -= j1;
						j0 -= j1;
						if (need_ce) {
							add_CE_entry(
							    "SL-path-split",
							    __LINE__);
							if (cpnt1) {
								*cpnt1 = '/';
								nchar++;
								/*
								 * A kluge so
								 * that we can
								 * restart
								 * properly
								 */
								cpnt1 = NULL;
							}
							break;
						}
					}
				}
				if (cpnt1) {
					*cpnt1 = '/';
					if (cpnt1 > symlink_buff && !last_sl &&
					    cpnt1[1] == '\0') {
						last_sl = TRUE;
					}
					cpnt = cpnt1 + 1;
				} else
					break;
			}
			Rock[lenpos] = lenval;
			if (nchar) {
				/* We need another SL entry */
				Rock[lenpos + 2] = SL_CONTINUE;
			}
		}	/* while nchar */
	}	/* Is a symbolic link */
#endif	/* S_IFLNK */

	/* Add in the Rock Ridge TF time field */
	if (MAYBE_ADD_CE_ENTRY(long_rr_time ? TF_SIZE_LONG:TF_SIZE))
		add_CE_entry("TF", __LINE__);
	Rock[ipnt++] = 'T';
	Rock[ipnt++] = 'F';
	Rock[ipnt++] = long_rr_time ? TF_SIZE_LONG:TF_SIZE;
	Rock[ipnt++] = SU_VERSION;
#if defined(__QNX__) && !defined(__QNXNTO__)	/* Not on Neutrino! never OK? */
	Rock[ipnt++] = long_rr_time ? 0x8f:0x0f;
#else
	Rock[ipnt++] = long_rr_time ? 0x8e:0x0e;
#endif
	flagval |= (1 << 7);

#if defined(__QNX__) && !defined(__QNXNTO__)	/* Not on Neutrino! never OK? */
	if (long_rr_time) {
		iso9660_date((char *)&Rock[ipnt], lstatbuf->st_ftime);
		ipnt += 7;
	} else {
		/*
		 * XXX Do we have nanoseconds on QNX?
		 */
		iso9660_ldate((char *)&Rock[ipnt], lstatbuf->st_ftime, 0, -100);
		ipnt += 17;
	}
#endif
	if (long_rr_time) {
		iso9660_ldate((char *)&Rock[ipnt],
				lstatbuf->st_mtime, stat_mnsecs(lstatbuf), -100);
		ipnt += 17;
		iso9660_ldate((char *)&Rock[ipnt],
				lstatbuf->st_atime, stat_ansecs(lstatbuf), -100);
		ipnt += 17;
		iso9660_ldate((char *)&Rock[ipnt],
				lstatbuf->st_ctime, stat_cnsecs(lstatbuf), -100);
		ipnt += 17;
	} else {
		iso9660_date((char *)&Rock[ipnt], lstatbuf->st_mtime);
		ipnt += 7;
		iso9660_date((char *)&Rock[ipnt], lstatbuf->st_atime);
		ipnt += 7;
		iso9660_date((char *)&Rock[ipnt], lstatbuf->st_ctime);
		ipnt += 7;
	}

	/* Add in the Rock Ridge RE (relocated dir) field */
	if (deep_opt & NEED_RE) {
		if (MAYBE_ADD_CE_ENTRY(RE_SIZE))
			add_CE_entry("RE", __LINE__);
		Rock[ipnt++] = 'R';
		Rock[ipnt++] = 'E';
		Rock[ipnt++] = RE_SIZE;
		Rock[ipnt++] = SU_VERSION;
		flagval |= (1 << 6);
	}
	/* Add in the Rock Ridge PL record, if required. */
	if (deep_opt & NEED_PL) {
		if (MAYBE_ADD_CE_ENTRY(PL_SIZE))
			add_CE_entry("PL", __LINE__);
		Rock[ipnt++] = 'P';
		Rock[ipnt++] = 'L';
		Rock[ipnt++] = PL_SIZE;
		Rock[ipnt++] = SU_VERSION;
		set_733((char *)Rock + ipnt, 0);
		ipnt += 8;
		flagval |= (1 << 5);
	}

	/* Add in the Rock Ridge CL field, if required. */
	if (deep_opt & NEED_CL) {
		if (MAYBE_ADD_CE_ENTRY(CL_SIZE))
			add_CE_entry("CL", __LINE__);
		Rock[ipnt++] = 'C';
		Rock[ipnt++] = 'L';
		Rock[ipnt++] = CL_SIZE;
		Rock[ipnt++] = SU_VERSION;
		set_733((char *)Rock + ipnt, 0);
		ipnt += 8;
		flagval |= (1 << 4);
	}

#ifndef VMS
	/*
	 * If transparent compression was requested, fill in the correct field
	 * for this file, if (and only if) it is actually a compressed file!
	 * This relies only on magic number, but it should in general not
	 * be an issue since if you're using -z odds are most of your
	 * files are already compressed.
	 *
	 * In the future it would be nice if mkisofs actually did the
	 * compression.
	 */
	if (transparent_compression && S_ISREG(lstatbuf->st_mode)) {
		static const Uchar zisofs_magic[8] =
			{ 0x37, 0xE4, 0x53, 0x96, 0xC9, 0xDB, 0xD6, 0x07 };
		FILE		*zffile;
		unsigned int	file_size;
		Uchar		header[16];
		int		OK_flag;
		int		blocksize;
		int		headersize;

		/*
		 * First open file and verify that the correct algorithm was
		 * used
		 */
		file_size = 0;
		OK_flag = 1;

		memset(header, 0, sizeof (header));

		zffile = fopen(whole_name, "rb");
		if (zffile != NULL) {
			if (fread(header, 1, sizeof (header), zffile) != sizeof (header))
				OK_flag = 0;

			/* Check magic number */
			if (memcmp(header, zisofs_magic, sizeof (zisofs_magic)))
				OK_flag = 0;

			/* Get the real size of the file */
			file_size = get_731((char *)header+8);

			/* Get the header size (>> 2) */
			headersize = header[12];

			/* Get the block size (log2) */
			blocksize = header[13];

			fclose(zffile);
		} else {
			OK_flag = 0;
			blocksize = headersize = 0; /* Make silly GCC quiet */
		}

		if (OK_flag) {
			if (MAYBE_ADD_CE_ENTRY(ZF_SIZE))
				add_CE_entry("ZF", __LINE__);
			Rock[ipnt++] = 'Z';
			Rock[ipnt++] = 'F';
			Rock[ipnt++] = ZF_SIZE;
			Rock[ipnt++] = SU_VERSION;
			Rock[ipnt++] = 'p'; /* Algorithm: "paged zlib" */
			Rock[ipnt++] = 'z';
			/* 2 bytes for algorithm-specific information */
			Rock[ipnt++] = headersize;
			Rock[ipnt++] = blocksize;
			set_733((char *)Rock + ipnt, file_size); /* Real file size */
			ipnt += 8;
		}
	}
#endif
	/*
	 * Add in the Rock Ridge CE field, if required.  We use  this for the
	 * extension record that is stored in the root directory.
	 */
	if (deep_opt & NEED_CE)
		add_CE_entry("ER", __LINE__);

	/*
	 * Done filling in all of the fields.  Now copy it back to a buffer
	 * for the file in question.
	 */
	/* Now copy this back to the buffer for the file */
	Rock[flagpos] = flagval;

	/* If there was a CE, fill in the size field */
	if (recstart)
		set_733((char *)Rock + recstart - 8, ipnt - recstart);

xa_only:
	s_entry->rr_attributes = (Uchar *) e_malloc(ipnt);
	s_entry->total_rr_attr_size = ipnt;
	s_entry->rr_attr_size = (mainrec ? mainrec : ipnt);
	memcpy(s_entry->rr_attributes, Rock, ipnt);
	return (ipnt);
}

/*
 * Guaranteed to  return a single sector with the relevant info
 */
EXPORT char *
generate_rr_extension_record(id, descriptor, source, size)
	char	*id;
	char	*descriptor;
	char	*source;
	int	*size;
{
	int		lipnt = 0;
	char		*pnt;
	int		len_id;
	int		len_des;
	int		len_src;

	len_id = strlen(id);
	len_des = strlen(descriptor);
	len_src = strlen(source);
	Rock[lipnt++] = 'E';
	Rock[lipnt++] = 'R';
	Rock[lipnt++] = ER_SIZE + len_id + len_des + len_src;
	Rock[lipnt++] = 1;
	Rock[lipnt++] = len_id;
	Rock[lipnt++] = len_des;
	Rock[lipnt++] = len_src;
	Rock[lipnt++] = 1;

	memcpy(Rock + lipnt, id, len_id);
	lipnt += len_id;

	memcpy(Rock + lipnt, descriptor, len_des);
	lipnt += len_des;

	memcpy(Rock + lipnt, source, len_src);
	lipnt += len_src;

	if (lipnt > SECTOR_SIZE) {
		comerrno(EX_BAD, _("Extension record too long\n"));
	}
	pnt = (char *)e_malloc(SECTOR_SIZE);
	memset(pnt, 0, SECTOR_SIZE);
	memcpy(pnt, Rock, lipnt);
	*size = lipnt;
	return (pnt);
}
