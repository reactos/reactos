/* @(#)multi.c	1.104 16/01/06 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)multi.c	1.104 16/01/06 joerg";
#endif
/*
 * File multi.c - scan existing iso9660 image and merge into
 * iso9660 filesystem.  Used for multisession support.
 *
 * Written by Eric Youngdale (1996).
 * Copyright (c) 1999-2016 J. Schilling
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
#include <schily/time.h>
#include <schily/errno.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/ctype.h>		/* Needed for printasc()	*/

#ifdef VMS

#include <sys/file.h>
#include <vms/fabdef.h>
#include "vms.h"
#endif

#ifndef howmany
#define	howmany(x, y)   (((x)+((y)-1))/(y))
#endif
#ifndef roundup
#define	roundup(x, y)   ((((x)+((y)-1))/(y))*(y))
#endif

/*
 * Cannot debug memset() with gdb on Linux, so use fillbytes()
 */
/*#define	memset(s, c, n)	fillbytes(s, n, c)*/

#define	TF_CREATE 1
#define	TF_MODIFY 2
#define	TF_ACCESS 4
#define	TF_ATTRIBUTES 8

LOCAL	void	printasc	__PR((char *txt, unsigned char *p, int len));
LOCAL	void	prbytes		__PR((char *txt, unsigned char *p, int len));
unsigned char	*parse_xa	__PR((unsigned char *pnt, int *lenp,
					struct directory_entry *dpnt));
EXPORT	int	rr_flags	__PR((struct iso_directory_record *idr));
LOCAL	int	parse_rrflags	__PR((Uchar *pnt, int len, int cont_flag));
LOCAL	BOOL	find_rr		__PR((struct iso_directory_record *idr, Uchar **pntp, int *lenp));
LOCAL	 int	parse_rr	__PR((unsigned char *pnt, int len,
					struct directory_entry *dpnt));
LOCAL	int	check_rr_dates	__PR((struct directory_entry *dpnt,
					struct directory_entry *current,
					struct stat *statbuf,
					struct stat *lstatbuf));
LOCAL	BOOL 	valid_iso_directory __PR((struct iso_directory_record *idr,
					int idr_off,
					size_t space_left));
LOCAL	struct directory_entry **
		read_merging_directory __PR((struct iso_directory_record *, int *));
LOCAL	int	free_mdinfo	__PR((struct directory_entry **, int len));
LOCAL	void	free_directory_entry __PR((struct directory_entry *dirp));
LOCAL	int	iso_dir_ents	__PR((struct directory_entry *de));
LOCAL	void	copy_mult_extent __PR((struct directory_entry *se1,
					struct directory_entry *se2));
LOCAL	void	merge_remaining_entries __PR((struct directory *,
					struct directory_entry **, int));

LOCAL	int	merge_old_directory_into_tree __PR((struct directory_entry *,
							struct directory *));
LOCAL	void	check_rr_relocation __PR((struct directory_entry *de));

FILE	*in_image = NULL;
BOOL	ignerr = FALSE;
int	su_version = -1;
int	rr_version = -1;
int	aa_version = -1;
char	er_id[256];

#ifndef	USE_SCG
/*
 * Don't define readsecs if mkisofs is linked with
 * the SCSI library.
 * readsecs() will be implemented as SCSI command in this case.
 *
 * Use global var in_image directly in readsecs()
 * the SCSI equivalent will not use a FILE* for I/O.
 *
 * The main point of this pointless abstraction is that Solaris won't let
 * you read 2K sectors from the cdrom driver.  The fact that 99.9% of the
 * discs out there have a 2K sectorsize doesn't seem to matter that much.
 * Anyways, this allows the use of a scsi-generics type of interface on
 * Solaris.
 */
#ifdef	PROTOTYPES
EXPORT int
readsecs(UInt32_t startsecno, void *buffer, int sectorcount)
#else
EXPORT int
readsecs(startsecno, buffer, sectorcount)
	UInt32_t	startsecno;
	void		*buffer;
	int		sectorcount;
#endif
{
	int		f = fileno(in_image);

	if (lseek(f, (off_t)startsecno * SECTOR_SIZE, SEEK_SET) == (off_t)-1) {
		comerr(_("Seek error on old image\n"));
	}
	if (read(f, buffer, (sectorcount * SECTOR_SIZE))
		!= (sectorcount * SECTOR_SIZE)) {
		comerr(_("Read error on old image\n"));
	}
	return (sectorcount * SECTOR_SIZE);
}

#endif

LOCAL void
printasc(txt, p, len)
	char		*txt;
	unsigned char	*p;
	int		len;
{
	int		i;

	error("%s ", txt);
	for (i = 0; i < len; i++) {
		if (isprint(p[i]))
			error("%c", p[i]);
		else
			error(".");
	}
	error("\n");
}

LOCAL void
prbytes(txt, p, len)
		char	*txt;
	register Uchar	*p;
	register int	len;
{
	error("%s", txt);
	while (--len >= 0)
		error(" %02X", *p++);
	error("\n");
}

unsigned char *
parse_xa(pnt, lenp, dpnt)
	unsigned char	*pnt;
	int		*lenp;
	struct directory_entry *dpnt;
{
	struct iso_xa_dir_record *xadp;
	int		len = *lenp;
static	int		did_xa = 0;

/*error("len: %d\n", len);*/

	if (len >= 14) {
		xadp = (struct iso_xa_dir_record *)pnt;

/*		if (dpnt) prbytes("XA ", pnt, len);*/
		if (xadp->signature[0] == 'X' && xadp->signature[1] == 'A' &&
				xadp->reserved[0] == '\0') {
			len -= 14;
			pnt += 14;
			*lenp = len;
			if (!did_xa) {
				did_xa = 1;
				errmsgno(EX_BAD, _("Found XA directory extension record.\n"));
			}
		} else if (pnt[2] == 0) {
			char *cp = NULL;

			if (dpnt)
				cp = (char *)&dpnt->isorec;
			if (cp) {
				prbytes("ISOREC:", (Uchar *)cp, 33+cp[32]);
				printasc("ISOREC:", (Uchar *)cp, 33+cp[32]);
				prbytes("XA REC:", pnt, len);
				printasc("XA REC:", pnt, len);
			}
			if (no_rr == 0) {
				errmsgno(EX_BAD, _("Disabling RR / XA / AA.\n"));
				no_rr = 1;
			}
			*lenp = 0;
			if (cp) {
				errmsgno(EX_BAD, _("Problems with old ISO directory entry for file: '%s'.\n"), &cp[33]);
			}
			errmsgno(EX_BAD, _("Illegal extended directory attributes found (bad XA disk?).\n"));
/*			errmsgno(EX_BAD, _("Disabling Rock Ridge for old session.\n"));*/
			comerrno(EX_BAD, _("Try again using the -no-rr option.\n"));
		}
	}
	if (len >= 4 && pnt[3] != 1 && pnt[3] != 2) {
		prbytes("BAD RR ATTRIBUTES:", pnt, len);
		printasc("BAD RR ATTRIBUTES:", pnt, len);
	}
	return (pnt);
}

LOCAL BOOL
find_rr(idr, pntp, lenp)
	struct iso_directory_record *idr;
	Uchar		**pntp;
	int		*lenp;
{
	struct iso_xa_dir_record *xadp;
	int		len;
	unsigned char	*pnt;
	BOOL		ret = FALSE;

	len = idr->length[0] & 0xff;
	len -= sizeof (struct iso_directory_record);
	len += sizeof (idr->name);
	len -= idr->name_len[0];

	pnt = (unsigned char *) idr;
	pnt += sizeof (struct iso_directory_record);
	pnt -= sizeof (idr->name);
	pnt += idr->name_len[0];
	if ((idr->name_len[0] & 1) == 0) {
		pnt++;
		len--;
	}
	if (len >= 14) {
		xadp = (struct iso_xa_dir_record *)pnt;

		if (xadp->signature[0] == 'X' && xadp->signature[1] == 'A' &&
				xadp->reserved[0] == '\0') {
			len -= 14;
			pnt += 14;
			ret = TRUE;
		}
	}
	*pntp = pnt;
	*lenp = len;
	return (ret);
}

LOCAL int
parse_rrflags(pnt, len, cont_flag)
	Uchar	*pnt;
	int	len;
	int	cont_flag;
{
	int		ncount;
	UInt32_t	cont_extent;
	UInt32_t	cont_offset;
	UInt32_t	cont_size;
	int		flag1;
	int		flag2;

	cont_extent = cont_offset = cont_size = 0;

	ncount = 0;
	flag1 = -1;
	flag2 = 0;
	while (len >= 4) {
		if (pnt[3] != 1 && pnt[3] != 2) {
			errmsgno(EX_BAD,
				_("**BAD RRVERSION (%d) in '%c%c' field (%2.2X %2.2X).\n"),
				pnt[3], pnt[0], pnt[1], pnt[0], pnt[1]);
			return (0);	/* JS ??? Is this right ??? */
		}
		if (pnt[2] < 4) {
			errmsgno(EX_BAD,
				_("**BAD RRLEN (%d) in '%2.2s' field %2.2X %2.2X.\n"),
				pnt[2], pnt, pnt[0], pnt[1]);
			return (0);	/* JS ??? Is this right ??? */
		}
		ncount++;
		if (pnt[0] == 'R' && pnt[1] == 'R')
			flag1 = pnt[4] & 0xff;

		if (strncmp((char *)pnt, "PX", 2) == 0)		/* POSIX attributes */
			flag2 |= RR_FLAG_PX;
		if (strncmp((char *)pnt, "PN", 2) == 0)		/* POSIX device number */
			flag2 |= RR_FLAG_PN;
		if (strncmp((char *)pnt, "SL", 2) == 0)		/* Symlink */
			flag2 |= RR_FLAG_SL;
		if (strncmp((char *)pnt, "NM", 2) == 0)		/* Alternate Name */
			flag2 |= RR_FLAG_NM;
		if (strncmp((char *)pnt, "CL", 2) == 0)		/* Child link */
			flag2 |= RR_FLAG_CL;
		if (strncmp((char *)pnt, "PL", 2) == 0)		/* Parent link */
			flag2 |= RR_FLAG_PL;
		if (strncmp((char *)pnt, "RE", 2) == 0)		/* Relocated Direcotry */
			flag2 |= RR_FLAG_RE;
		if (strncmp((char *)pnt, "TF", 2) == 0)		/* Time stamp */
			flag2 |= RR_FLAG_TF;
		if (strncmp((char *)pnt, "SP", 2) == 0) {	/* SUSP record */
			flag2 |= RR_FLAG_SP;
			if (su_version < 0)
				su_version = pnt[3] & 0xff;
		}
		if (strncmp((char *)pnt, "AA", 2) == 0) {	/* Apple Signature record */
			flag2 |= RR_FLAG_AA;
			if (aa_version < 0)
				aa_version = pnt[3] & 0xff;
		}
		if (strncmp((char *)pnt, "ER", 2) == 0) {
			flag2 |= RR_FLAG_ER;				/* ER record */
			if (rr_version < 0)
				rr_version = pnt[7] & 0xff;		/* Ext Version */
			strlcpy(er_id, (char *)&pnt[8], (pnt[4] & 0xFF) + 1);
		}

		if (strncmp((char *)pnt, "CE", 2) == 0) {	/* Continuation Area */
			cont_extent = get_733(pnt+4);
			cont_offset = get_733(pnt+12);
			cont_size = get_733(pnt+20);
		}
		if (strncmp((char *)pnt, "ST", 2) == 0) {		/* Terminate SUSP */
			break;
		}

		len -= pnt[2];
		pnt += pnt[2];
	}
	if (cont_extent) {
		unsigned char   sector[SECTOR_SIZE];

		readsecs(cont_extent, sector, 1);
		flag2 |= parse_rrflags(&sector[cont_offset], cont_size, 1);
	}
	return (flag2);
}

int
rr_flags(idr)
	struct iso_directory_record *idr;
{
	int		len;
	unsigned char	*pnt;
	int		ret = 0;

	if (find_rr(idr, &pnt, &len))
		ret |= RR_FLAG_XA;
	ret |= parse_rrflags(pnt, len, 0);
	return (ret);
}

/*
 * Parse the RR attributes so we can find the file name.
 */
LOCAL int
parse_rr(pnt, len, dpnt)
	unsigned char	*pnt;
	int		len;
	struct directory_entry *dpnt;
{
	UInt32_t	cont_extent;
	UInt32_t	cont_offset;
	UInt32_t	cont_size;
	char		name_buf[256];

	cont_extent = cont_offset = cont_size = 0;

	pnt = parse_xa(pnt, &len, dpnt /* 0 */);

	while (len >= 4) {
		if (pnt[3] != 1 && pnt[3] != 2) {
			errmsgno(EX_BAD,
				_("**BAD RRVERSION (%d) in '%c%c' field (%2.2X %2.2X).\n"),
				pnt[3], pnt[0], pnt[1], pnt[0], pnt[1]);
			return (-1);
		}
		if (pnt[2] < 4) {
			errmsgno(EX_BAD,
				_("**BAD RRLEN (%d) in '%2.2s' field %2.2X %2.2X.\n"),
				pnt[2], pnt, pnt[0], pnt[1]);
			return (-1);
		}
		if (strncmp((char *)pnt, "NM", 2) == 0) {
			strncpy(name_buf, (char *)pnt + 5, pnt[2] - 5);
			name_buf[pnt[2] - 5] = 0;
			if (dpnt->name) {
				size_t nlen = strlen(dpnt->name);

				/*
				 * append to name from previous NM records
				 */
				dpnt->name = realloc(dpnt->name, nlen +
							strlen(name_buf) + 1);
				strcpy(dpnt->name + nlen, name_buf);
			} else {
				dpnt->name = e_strdup(name_buf);
				dpnt->got_rr_name = 1;
			}
			/* continue searching for more NM records */
		} else if (strncmp((char *)pnt, "CE", 2) == 0) {
			cont_extent = get_733(pnt + 4);
			cont_offset = get_733(pnt + 12);
			cont_size = get_733(pnt + 20);
		} else if (strncmp((char *)pnt, "ST", 2) == 0) {
			break;
		}

		len -= pnt[2];
		pnt += pnt[2];
	}
	if (cont_extent) {
		unsigned char   sector[SECTOR_SIZE];

		readsecs(cont_extent, sector, 1);
		if (parse_rr(&sector[cont_offset], cont_size, dpnt) == -1)
			return (-1);
	}

	/* Fall back to the iso name if no RR name found */
	if (dpnt->name == NULL) {
		char	*cp;

		strlcpy(name_buf, dpnt->isorec.name, sizeof (name_buf));
		cp = strchr(name_buf, ';');
		if (cp != NULL) {
			*cp = '\0';
		}
		dpnt->name = e_strdup(name_buf);
	}
	return (0);
} /* parse_rr */


/*
 * Returns 1 if the two files are identical
 * Returns 0 if the two files differ
 */
LOCAL int
check_rr_dates(dpnt, current, statbuf, lstatbuf)
	struct directory_entry *dpnt;
	struct directory_entry *current;
	struct stat	*statbuf;
	struct stat	*lstatbuf;
{
	UInt32_t	cont_extent;
	UInt32_t	cont_offset;
	UInt32_t	cont_size;
	UInt32_t	offset;
	unsigned char	*pnt;
	int		len;
	int		same_file;
	int		same_file_type;
	mode_t		mode;
	char		time_buf[7];


	cont_extent = cont_offset = cont_size = 0;
	same_file = 1;
	same_file_type = 1;

	pnt = dpnt->rr_attributes;
	len = dpnt->rr_attr_size;
	/*
	 * We basically need to parse the rr attributes again, and dig out the
	 * dates and file types.
	 */
	pnt = parse_xa(pnt, &len, /* dpnt */ 0);
	while (len >= 4) {
		if (pnt[3] != 1 && pnt[3] != 2) {
			errmsgno(EX_BAD,
				_("**BAD RRVERSION (%d) in '%c%c' field (%2.2X %2.2X).\n"),
				pnt[3], pnt[0], pnt[1], pnt[0], pnt[1]);
			return (-1);
		}
		if (pnt[2] < 4) {
			errmsgno(EX_BAD,
				_("**BAD RRLEN (%d) in '%2.2s' field %2.2X %2.2X.\n"),
				pnt[2], pnt, pnt[0], pnt[1]);
			return (-1);
		}

		/*
		 * If we have POSIX file modes, make sure that the file type is
		 * the same.  If it isn't, then we must always write the new
		 * file.
		 */
		if (strncmp((char *)pnt, "PX", 2) == 0) {
			mode = get_733(pnt + 4);
			if ((lstatbuf->st_mode & S_IFMT) != (mode & S_IFMT)) {
				same_file_type = 0;
				same_file = 0;
			}
		}
		if (strncmp((char *)pnt, "TF", 2) == 0) {
			offset = 5;
			if (pnt[4] & TF_CREATE) {
				iso9660_date((char *)time_buf,
							lstatbuf->st_ctime);
				if (memcmp(time_buf, pnt + offset, 7) != 0)
					same_file = 0;
				offset += 7;
			}
			if (pnt[4] & TF_MODIFY) {
				iso9660_date((char *)time_buf,
							lstatbuf->st_mtime);
				if (memcmp(time_buf, pnt + offset, 7) != 0)
					same_file = 0;
				offset += 7;
			}
		}
		if (strncmp((char *)pnt, "CE", 2) == 0) {
			cont_extent = get_733(pnt + 4);
			cont_offset = get_733(pnt + 12);
			cont_size = get_733(pnt + 20);
		}
		if (strncmp((char *)pnt, "ST", 2) == 0) {		/* Terminate SUSP */
			break;
		}

		len -= pnt[2];
		pnt += pnt[2];
	}
	if (cont_extent) {
		unsigned char   sector[SECTOR_SIZE];

		readsecs(cont_extent, sector, 1);
		/*
		 * Continue to scan the extension record.
		 * Note that this has not been tested yet, but it is
		 * definitely more correct that calling parse_rr()
		 * as done in Eric's old code.
		 */
		pnt = &sector[cont_offset];
		len = cont_size;
		/*
		 * Clear the "pending extension record" state as
		 * we did already read it now.
		 */
		cont_extent = cont_offset = cont_size = 0;
	}

	/*
	 * If we have the same fundamental file type, then it is clearly safe
	 * to reuse the TRANS.TBL entry.
	 */
	if (same_file_type) {
		current->de_flags |= SAFE_TO_REUSE_TABLE_ENTRY;
	}
	return (same_file);
}

LOCAL BOOL
valid_iso_directory(idr, idr_off, space_left)
	struct iso_directory_record	*idr;
	int				idr_off;
	size_t				space_left;
{
	size_t	idr_length	= idr->length[0] & 0xFF;
	size_t	idr_ext_length	= idr->ext_attr_length[0] & 0xFF;
	size_t	idr_namelength	= idr->name_len[0] & 0xFF;
	int	namelimit	= space_left -
				offsetof(struct iso_directory_record, name[0]);
	int	nlimit		= (idr_namelength < namelimit) ?
					idr_namelength : namelimit;

	/*
	 * Check for sane length entries.
	 */
	if (idr_length > space_left) {
		comerrno(EX_BAD,
		    _("Bad directory length %zu (> %d available) for '%.*s'.\n"),
				idr_length, namelimit, nlimit, idr->name);
	}

	if (idr_length == 0) {
		if ((idr_off % SECTOR_SIZE) != 0) {
			/*
			 * It marks a valid continuation entry.
			 */
			return (TRUE);
		} else {
			comerrno(EX_BAD,
				_("Zero directory length for '%.*s'.\n"),
				nlimit, idr->name);
		}
	}
	if (idr_length <= offsetof(struct iso_directory_record, name[0])) {
		comerrno(EX_BAD, _("Bad directory length %zu (< %zu minimum).\n"),
				idr_length, 1 + offsetof(struct iso_directory_record, name[0]));
	}
	if ((idr_length & 1) != 0) {
		comerrno(EX_BAD, _("Odd directory length %zu for '%.*s'.\n"),
				idr_length, nlimit, idr->name);
	}

	if (idr_namelength == 0) {
		comerrno(EX_BAD, _("Zero filename length.\n"));
	}

	if (!(idr_namelength & 1)) {
		/*
		 * if nam_len[0] is even, there has to be a pad byte at the end
		 * to make the directory length even
		 */
		idr_namelength++;
	}
	if ((offsetof(struct iso_directory_record, name[0]) +
	    idr_namelength) > idr_length) {
		int	xlimit = idr_length -
			offsetof(struct iso_directory_record, name[0]) -
			idr_ext_length;

		if (xlimit < 0)
			xlimit = 0;
		if (nlimit < xlimit)
			xlimit = nlimit;
		comerrno(EX_BAD, _("Bad filename length %zu (> %d) for '%.*s'.\n"),
				idr_namelength, xlimit, xlimit, idr->name);
	}
	if ((offsetof(struct iso_directory_record, name[0]) +
	    idr_namelength + idr_ext_length) > idr_length) {
		int	xlimit = idr_length -
			offsetof(struct iso_directory_record, name[0]) -
			idr_namelength;

		comerrno(EX_BAD, _("Bad extended attribute length %zu (> %d) for '%.*s'.\n"),
				idr_ext_length, xlimit, nlimit, idr->name);
	}

#ifdef	__do_rr_
	/* check for rock ridge extensions */

	if (no_rr) {
		/*
		 * Rock Ridge extensions are not present or manually disabled.
		 */
		return (TRUE);
	} else {
		int	rlen =  idr_length -
				offsetof(struct iso_directory_record, name[0]) -
				idr_namelength;

		/* Check for the minimum of Rock Ridge extensions. */
	}
#endif
	return (TRUE);
}

LOCAL struct directory_entry **
read_merging_directory(mrootp, nentp)
	struct iso_directory_record *mrootp;
	int		*nentp;
{
	unsigned char	*cpnt;
	unsigned char	*cpnt1;
	char		*p;
	char		*dirbuff;
	int		i;
	struct iso_directory_record *idr;
	UInt32_t	len;
	UInt32_t	nbytes;
	int		nent;
	int		nmult;		/* # of multi extent root entries */
	int		mx;
	struct directory_entry **pnt;
	UInt32_t	rlen;
	struct directory_entry **rtn;
	int		seen_rockridge;
	unsigned char	*tt_buf;
	UInt32_t	tt_extent;
	UInt32_t		tt_size;

	static int	warning_given = 0;

	/*
	 * This is the number of sectors we will need to read.  We need to
	 * round up to get the last fractional sector - we are asking for the
	 * data in terms of a number of sectors.
	 */
	nbytes = roundup(get_733(mrootp->size), SECTOR_SIZE);

	/*
	 * First, allocate a buffer large enough to read in the entire
	 * directory.
	 */
	dirbuff = (char *)e_malloc(nbytes);

	readsecs(get_733(mrootp->extent), dirbuff, nbytes / SECTOR_SIZE);

	/*
	 * Next look over the directory, and count up how many entries we have.
	 */
	len = get_733(mrootp->size);
	i = 0;
	*nentp = 0;
	nent = 0;
	nmult = 0;
	mx = 0;
	while ((i + offsetof(struct iso_directory_record, name[0])) < len) {
		idr = (struct iso_directory_record *)&dirbuff[i];

		if (!valid_iso_directory(idr, i, len - i))
			break;

		if (idr->length[0] == 0) {
			i = ISO_ROUND_UP(i);
			continue;
		}
		nent++;
		if ((mx & ISO_MULTIEXTENT) == 0 &&
		    (idr->flags[0] & ISO_MULTIEXTENT) != 0) {
			nmult++;	/* Need a multi extent root entry */
		}
		mx = idr->flags[0];
		i += idr->length[0];
	}

	/*
	 * Now allocate the buffer which will hold the array we are about to
	 * return. We need one entry per real directory entry and in addition
	 * one multi-extent root entry per multi-extent file.
	 */
	rtn = (struct directory_entry **)e_malloc((nent+nmult) * sizeof (*rtn));

	/*
	 * Finally, scan the directory one last time, and pick out the relevant
	 * bits of information, and store it in the relevant bits of the
	 * structure.
	 */
	i = 0;
	pnt = rtn;
	tt_extent = 0;
	seen_rockridge = 0;
	tt_size = 0;
	mx = 0;
	while ((i + offsetof(struct iso_directory_record, name[0])) < len) {
		idr = (struct iso_directory_record *)&dirbuff[i];

		if (!valid_iso_directory(idr, i, len - i))
			break;

		if (idr->length[0] == 0) {
			i = ISO_ROUND_UP(i);
			continue;
		}
		*pnt = (struct directory_entry *)e_malloc(sizeof (**rtn));
		(*pnt)->next = NULL;
#ifdef	DEBUG
		error("IDR name: '%s' ist: %d soll: %d\n",
			idr->name, strlen(idr->name), idr->name_len[0]);
#endif
		movebytes(idr, &(*pnt)->isorec, idr->length[0] & 0xFF);
		(*pnt)->starting_block =
				get_733(idr->extent);
		(*pnt)->size = get_733(idr->size);
		if ((*pnt)->size == 0) {
			/*
			 * Find lowest used inode number for zero sized files
			 */
			if (((UInt32_t)(*pnt)->starting_block) <= null_inodes) {
				null_inodes = (UInt32_t)(*pnt)->starting_block;
				null_inodes--;
			}
		}
		(*pnt)->priority = 0;
		(*pnt)->name = NULL;
		(*pnt)->got_rr_name = 0;
		(*pnt)->table = NULL;
		(*pnt)->whole_name = NULL;
		(*pnt)->filedir = NULL;
		(*pnt)->parent_rec = NULL;
		/*
		 * Set this information so that we correctly cache previous
		 * session bits of information.
		 */
		(*pnt)->inode = (*pnt)->starting_block;
		(*pnt)->dev = PREV_SESS_DEV;
		(*pnt)->rr_attributes = NULL;
		(*pnt)->rr_attr_size = 0;
		(*pnt)->total_rr_attr_size = 0;
		(*pnt)->de_flags = SAFE_TO_REUSE_TABLE_ENTRY;
#ifdef APPLE_HYB
		(*pnt)->assoc = NULL;
		(*pnt)->hfs_ent = NULL;
#endif	/* APPLE_HYB */

		/*
		 * Check for and parse any RR attributes for the file. All we
		 * are really looking for here is the original name of the
		 * file.
		 */
		rlen = idr->length[0] & 0xff;
		cpnt = (unsigned char *) idr;

		rlen -= offsetof(struct iso_directory_record, name[0]);
		cpnt += offsetof(struct iso_directory_record, name[0]);

		rlen -= idr->name_len[0];
		cpnt += idr->name_len[0];

		if ((idr->name_len[0] & 1) == 0) {
			cpnt++;
			rlen--;
		}

		if (no_rr)
			rlen = 0;
		if (rlen > 0) {
			(*pnt)->total_rr_attr_size =
						(*pnt)->rr_attr_size = rlen;
			(*pnt)->rr_attributes = e_malloc(rlen);
			memcpy((*pnt)->rr_attributes, cpnt, rlen);
			seen_rockridge = 1;
		}
#ifdef	DEBUG
		error("INT name: '%s' ist: %d soll: %d\n",
			(*pnt)->isorec.name, strlen((*pnt)->isorec.name),
			idr->name_len[0]);
#endif

		if (idr->name_len[0] < sizeof ((*pnt)->isorec.name)) {
			/*
			 * Now zero out the remainder of the name field.
			 */
			cpnt = (unsigned char *) (*pnt)->isorec.name;
			cpnt += idr->name_len[0];
			memset(cpnt, 0,
				sizeof ((*pnt)->isorec.name) - idr->name_len[0]);
		} else {
			/*
			 * Simple sanity work to make sure that we have no
			 * illegal data structures in our tree.
			 */
			(*pnt)->isorec.name[MAX_ISONAME] = '\0';
			(*pnt)->isorec.name_len[0] = MAX_ISONAME;
		}
		/*
		 * If the filename len from the old session is more
		 * then 31 chars, there is a high risk of hard violations
		 * of the ISO9660 standard.
		 * Run it through our name canonication machine....
		 */
		if (idr->name_len[0] > LEN_ISONAME || check_oldnames) {
			iso9660_check(idr, *pnt);
		}

		if (parse_rr((*pnt)->rr_attributes, rlen, *pnt) == -1) {
			comerrno(EX_BAD,
			    _("Cannot parse Rock Ridge attributes for '%s'.\n"),
								idr->name);
		}
		if (((*pnt)->isorec.name_len[0] == 1) &&
		    (((*pnt)->isorec.name[0] == 0) ||	/* "."  entry */
		    ((*pnt)->isorec.name[0] == 1))) {	/* ".." entry */

			if ((*pnt)->name != NULL) {
				free((*pnt)->name);
			}
			if ((*pnt)->whole_name != NULL) {
				free((*pnt)->whole_name);
			}
			if ((*pnt)->isorec.name[0] == 0) {
				(*pnt)->name = e_strdup(".");
			} else {
				(*pnt)->name = e_strdup("..");
			}
		}
#ifdef DEBUG
		fprintf(stderr, "got DE name: %s\n", (*pnt)->name);
#endif

		if (strncmp(idr->name, trans_tbl, strlen(trans_tbl)) == 0) {
			if ((*pnt)->name != NULL) {
				free((*pnt)->name);
			}
			if ((*pnt)->whole_name != NULL) {
				free((*pnt)->whole_name);
			}
/*			(*pnt)->name = e_strdup("<translation table>");*/
			(*pnt)->name = e_strdup(trans_tbl);
			tt_extent = get_733(idr->extent);
			tt_size = get_733(idr->size);
			if (tt_extent == 0)
				tt_size = 0;
		}
		/*
		 * The beginning of a new multi extent directory chain is when
		 * the last directory had no ISO_MULTIEXTENT flag set and the
		 * current entry did set ISO_MULTIEXTENT.
		 */
		if ((mx & ISO_MULTIEXTENT) == 0 &&
		    (idr->flags[0] & ISO_MULTIEXTENT) != 0) {
			struct directory_entry		*s_entry;
			struct iso_directory_record	*idr2 = idr;
			int				i2 = i;
			off_t				tsize = 0;

			/*
			 * Sum up the total file size for the multi extent file
			 */
			while ((i2 + offsetof(struct iso_directory_record, name[0])) < len) {
				idr2 = (struct iso_directory_record *)&dirbuff[i2];
				if (idr2->length[0] == 0) {
					i2 = ISO_ROUND_UP(i2);
					continue;
				}

				tsize += get_733(idr2->size);
				if ((idr2->flags[0] & ISO_MULTIEXTENT) == 0)
					break;
				i2 += idr2->length[0];
			}

			s_entry = dup_directory_entry(*pnt);	/* dup first for mxroot */
			s_entry->de_flags |= MULTI_EXTENT;
			s_entry->de_flags |= INHIBIT_ISO9660_ENTRY|INHIBIT_JOLIET_ENTRY;
			s_entry->size = tsize;
			s_entry->starting_block = (*pnt)->starting_block;
			s_entry->mxroot = s_entry;
			s_entry->mxpart = 0;
			s_entry->next = *pnt;	/* Next in list		*/
			pnt[1] = pnt[0];	/* Move to next slot	*/
			*pnt = s_entry;		/* First slot is mxroot	*/
			pnt++;			/* Point again to cur.	*/
		}
		if ((mx & ISO_MULTIEXTENT) != 0 ||
		    (idr->flags[0] & ISO_MULTIEXTENT) != 0) {
			(*pnt)->de_flags |= MULTI_EXTENT;
			(*pnt)->de_flags |= INHIBIT_UDF_ENTRY;
			(pnt[-1])->next = *pnt;
			(*pnt)->mxroot = (pnt[-1])->mxroot;
			(*pnt)->mxpart = (pnt[-1])->mxpart + 1;
		}
		pnt++;
		mx = idr->flags[0];
		i += idr->length[0];
	}
#ifdef APPLE_HYB
	/*
	 * If we find an associated file, check if there is a file
	 * with same ISO name and link it to this entry
	 */
	for (pnt = rtn, i = 0; i < nent; i++, pnt++) {
		int	j;

		rlen = get_711((*pnt)->isorec.name_len);
		if ((*pnt)->isorec.flags[0] & ISO_ASSOCIATED) {
			for (j = 0; j < nent; j++) {
				if (strncmp(rtn[j]->isorec.name,
				    (*pnt)->isorec.name, rlen) == 0 &&
				    (rtn[j]->isorec.flags[0] & ISO_ASSOCIATED) == 0) {
					rtn[j]->assoc = *pnt;

					/*
					 * don't want this entry to be
					 * in the Joliet tree
					 */
					(*pnt)->de_flags |= INHIBIT_JOLIET_ENTRY;
					/*
					 * XXX Is it correct to exclude UDF too?
					 */
					(*pnt)->de_flags |= INHIBIT_UDF_ENTRY;

					/*
					 * as we have associated files, then
					 * assume we are are dealing with
					 * Apple's extensions - if not already
					 * set
					 */
					if (apple_both == 0) {
						apple_both = apple_ext = 1;
					}
					break;
				}
			}
		}
	}
#endif	/* APPLE_HYB */

	/*
	 * If there was a TRANS.TBL;1 entry, then grab it, read it, and use it
	 * to get the filenames of the files.  Also, save the table info, just
	 * in case we need to use it.
	 *
	 * The entries look something like: F ISODUMP.;1 isodump
	 */
	if (tt_extent != 0 && tt_size != 0) {
		nbytes = roundup(tt_size, SECTOR_SIZE);
		tt_buf = (unsigned char *) e_malloc(nbytes);
		readsecs(tt_extent, tt_buf, nbytes / SECTOR_SIZE);

		/*
		 * Loop through the file, examine each entry, and attempt to
		 * attach it to the correct entry.
		 */
		cpnt = tt_buf;
		cpnt1 = tt_buf;
		while (cpnt - tt_buf < tt_size) {
			/* Skip to a line terminator, or end of the file. */
			while ((cpnt1 - tt_buf < tt_size) &&
				(*cpnt1 != '\n') &&
				(*cpnt1 != '\0')) {
				cpnt1++;
			}
			/* Zero terminate this particular line. */
			if (cpnt1 - tt_buf < tt_size) {
				*cpnt1 = '\0';
			}
			/*
			 * Now dig through the actual directories, and try and
			 * find the attachment for this particular filename.
			 */
			for (pnt = rtn, i = 0; i < nent; i++, pnt++) {
				rlen = get_711((*pnt)->isorec.name_len);

				/*
				 * If this filename is so long that it would
				 * extend past the end of the file, it cannot
				 * be the one we want.
				 */
				if (cpnt + 2 + rlen - tt_buf >= tt_size) {
					continue;
				}
				/*
				 * Now actually compare the name, and make sure
				 * that the character at the end is a ' '.
				 */
				if (strncmp((char *)cpnt + 2,
					(*pnt)->isorec.name, rlen) == 0 &&
					cpnt[2 + rlen] == ' ' &&
					(p = strchr((char *)&cpnt[2 + rlen], '\t'))) {
					p++;
					/*
					 * This is a keeper. Now determine the
					 * correct table entry that we will
					 * use on the new image.
					 */
					if (strlen(p) > 0) {
						(*pnt)->table =
						    e_malloc(strlen(p) + 4);
						sprintf((*pnt)->table,
							"%c\t%s\n",
							*cpnt, p);
					}
					if (!(*pnt)->got_rr_name) {
						if ((*pnt)->name != NULL) {
							free((*pnt)->name);
						}
						(*pnt)->name = e_strdup(p);
					}
					break;
				}
			}
			cpnt = cpnt1 + 1;
			cpnt1 = cpnt;
		}

		free(tt_buf);
	} else if (!seen_rockridge && !warning_given) {
		/*
		 * Warn the user that iso-9660 names were used because neither
		 * Rock Ridge (-R) nor TRANS.TBL (-T) name translations were
		 * found.
		 */
		fprintf(stderr,
		    _("Warning: Neither Rock Ridge (-R) nor TRANS.TBL (-T) \n"));
		fprintf(stderr,
		    _("name translations were found on previous session.\n"));
		fprintf(stderr,
		    _("ISO-9660 file names have been used instead.\n"));
		warning_given = 1;
	}
	if (dirbuff != NULL) {
		free(dirbuff);
	}
	*nentp = nent + nmult;
	return (rtn);
} /* read_merging_directory */

/*
 * Free any associated data related to the structures.
 */
LOCAL int
free_mdinfo(ptr, len)
	struct directory_entry **ptr;
	int		len;
{
	int		i;
	struct directory_entry **p;

	p = ptr;
	for (i = 0; i < len; i++, p++) {
		/*
		 * If the tree-handling code decided that it needed an entry, it
		 * will have removed it from the list.  Thus we must allow for
		 * null pointers here.
		 */
		if (*p == NULL) {
			continue;
		}
		free_directory_entry(*p);
	}

	free(ptr);
	return (0);
}

LOCAL void
free_directory_entry(dirp)
	struct directory_entry *dirp;
{
	if (dirp->name != NULL)
		free(dirp->name);

	if (dirp->whole_name != NULL)
		free(dirp->whole_name);

	if (dirp->rr_attributes != NULL)
		free(dirp->rr_attributes);

	if (dirp->table != NULL)
		free(dirp->table);

	free(dirp);
}

/*
 * Search the list to see if we have any entries from the previous
 * session that match this entry.  If so, copy the extent number
 * over so we don't bother to write it out to the new session.
 */
int
check_prev_session(ptr, len, curr_entry, statbuf, lstatbuf, odpnt)
	struct directory_entry	**ptr;
	int		len;
	struct directory_entry *curr_entry;
	struct stat	*statbuf;
	struct stat	*lstatbuf;
	struct directory_entry **odpnt;
{
	int		i;
	int		rr;
	int		retcode = -2;	/* Default not found */

	for (i = 0; i < len; i++) {
		if (ptr[i] == NULL) {	/* Used or empty entry skip */
			continue;
		}
#if 0
		if (ptr[i]->name != NULL && ptr[i]->isorec.name_len[0] == 1 &&
		    ptr[i]->name[0] == '\0') {
			continue;
		}
		if (ptr[i]->name != NULL && ptr[i]->isorec.name_len[0] == 1 &&
		    ptr[i]->name[0] == 1) {
			continue;
		}
#else
		if (ptr[i]->name != NULL && strcmp(ptr[i]->name, ".") == 0) {
			continue;
		}
		if (ptr[i]->name != NULL && strcmp(ptr[i]->name, "..") == 0) {
			continue;
		}
#endif

		if (ptr[i]->name != NULL &&
		    strcmp(ptr[i]->name, curr_entry->name) != 0) {
			/* Not the same name continue */
			continue;
		}
		/*
		 * It's a directory so we must always merge it with the new
		 * session. Never ever reuse directory extents.  See comments
		 * in tree.c for an explaination of why this must be the case.
		 */
		if ((curr_entry->isorec.flags[0] & ISO_DIRECTORY) != 0) {
			retcode = i;
			goto found_it;
		}
		/*
		 * We know that the files have the same name.  If they also
		 * have the same file type (i.e. file, dir, block, etc), then
		 * we can safely reuse the TRANS.TBL entry for this file. The
		 * check_rr_dates() function will do this for us.
		 *
		 * Verify that the file type and dates are consistent. If not,
		 * we probably have a different file, and we need to write it
		 * out again.
		 */
		retcode = i;

		if (ptr[i]->rr_attributes != NULL) {
			if ((rr = check_rr_dates(ptr[i], curr_entry, statbuf,
							lstatbuf)) == -1)
				return (-1);

			if (rr == 0) {	/* Different files */
				goto found_it;
			}
		}
		/*
		 * Verify size and timestamp.  If rock ridge is in use, we
		 * need to compare dates from RR too.  Directories are special,
		 * we calculate their size later.
		 */
		if (ptr[i]->size != curr_entry->size) {
			/* Different files */
			goto found_it;
		}
		if (memcmp(ptr[i]->isorec.date,
					curr_entry->isorec.date, 7) != 0) {
			/* Different files */
			goto found_it;
		}
		/* We found it and we can reuse the extent */
		memcpy(curr_entry->isorec.extent, ptr[i]->isorec.extent, 8);
		curr_entry->starting_block = get_733(ptr[i]->isorec.extent);
		curr_entry->de_flags |= SAFE_TO_REUSE_TABLE_ENTRY;

		if ((curr_entry->isorec.flags[0] & ISO_MULTIEXTENT) ||
		    (ptr[i]->isorec.flags[0] & ISO_MULTIEXTENT)) {
			copy_mult_extent(curr_entry, ptr[i]);
		}
		goto found_it;
	}
	return (retcode);

found_it:
	if (ptr[i]->mxroot == ptr[i]) {	/* Remove all multi ext. entries   */
		int	j = i + 1;	/* First one will be removed below */

		while (j < len && ptr[j] && ptr[j]->mxroot == ptr[i]) {
			free(ptr[j]);
			ptr[j++] = NULL;
		}
	}
	if (odpnt != NULL) {
		*odpnt = ptr[i];
	} else {
		free(ptr[i]);
	}
	ptr[i] = NULL;
	return (retcode);
}

/*
 * Return the number of directory entries for a file. This is usually 1
 * but may be 3 or more in case of multi extent files.
 */
LOCAL int
iso_dir_ents(de)
	struct directory_entry	*de;
{
	struct directory_entry	*de2;
	int	ret = 0;

	if (de->mxroot == NULL)
		return (1);
	de2 = de;
	while (de2 != NULL && de2->mxroot == de->mxroot) {
		ret++;
		de2 = de2->next;
	}
	return (ret);
}

/*
 * Copy old multi-extent directory information from the previous session.
 * If both the old session and the current session are created by mkisofs
 * then this code could be extremely simple as the information is only copied
 * in case that the file did not change since the last session was made.
 * As we don't know the other ISO formatter program, any combination of
 * multi-extent files and even a single extent file could be possible.
 * We need to handle all files the same way ad the old session was created as
 * we reuse the data extents from the file in the old session.
 */
LOCAL void
copy_mult_extent(se1, se2)
	struct directory_entry	*se1;
	struct directory_entry	*se2;
{
	struct directory_entry	*curr_entry = se1;
	int			len1;
	int			len2;
	int			mxpart = 0;

	len1 = iso_dir_ents(se1);
	len2 = iso_dir_ents(se2);

	if (len1 == 1) {
		/*
		 * Convert single-extent to multi-extent.
		 * If *se1 is not multi-extent, *se2 definitely is
		 * and we need to set up a MULTI_EXTENT directory header.
		 */
		se1->de_flags |= MULTI_EXTENT;
		se1->isorec.flags[0] |= ISO_MULTIEXTENT;
		se1->mxroot = curr_entry;
		se1->mxpart = 0;
		se1 = dup_directory_entry(se1);
		curr_entry->de_flags |= INHIBIT_ISO9660_ENTRY|INHIBIT_JOLIET_ENTRY;
		se1->de_flags |= INHIBIT_UDF_ENTRY;
		se1->next = curr_entry->next;
		curr_entry->next = se1;
		se1 = curr_entry;
		len1 = 2;
	}

	while (se2->isorec.flags[0] & ISO_MULTIEXTENT) {
		len1--;
		len2--;
		if (len1 <= 0) {
			struct directory_entry *sex = dup_directory_entry(se1);

			sex->mxroot = curr_entry;
			sex->next = se1->next;
			se1->next = sex;
			len1++;
		}
		memcpy(se1->isorec.extent, se2->isorec.extent, 8);
		se1->starting_block = get_733(se2->isorec.extent);
		se1->de_flags |= SAFE_TO_REUSE_TABLE_ENTRY;
		se1->de_flags |= MULTI_EXTENT;
		se1->isorec.flags[0] |= ISO_MULTIEXTENT;
		se1->mxroot = curr_entry;
		se1->mxpart = mxpart++;

		se1 = se1->next;
		se2 = se2->next;
	}
	memcpy(se1->isorec.extent, se2->isorec.extent, 8);
	se1->starting_block = get_733(se2->isorec.extent);
	se1->de_flags |= SAFE_TO_REUSE_TABLE_ENTRY;
	se1->isorec.flags[0] &= ~ISO_MULTIEXTENT;	/* Last entry */
	se1->mxpart = mxpart;
	while (len1 > 1) {				/* Drop other entries */
		struct directory_entry	*sex;

		sex = se1->next;
		se1->next = sex->next;
		free(sex);
		len1--;
	}
}

/*
 * open_merge_image:  Open an existing image.
 */
int
open_merge_image(path)
	char	*path;
{
#ifndef	USE_SCG
	in_image = fopen(path, "rb");
	if (in_image == NULL) {
		return (-1);
	}
#else
	in_image = fopen(path, "rb");
	if (in_image == NULL) {
		if (scsidev_open(path) < 0)
			return (-1);
	}
#endif
	return (0);
}

/*
 * close_merge_image:  Close an existing image.
 */
int
close_merge_image()
{
#ifdef	USE_SCG
	return (scsidev_close());
#else
	return (fclose(in_image));
#endif
}

/*
 * merge_isofs:  Scan an existing image, and return a pointer
 * to the root directory for this image.
 */
struct iso_directory_record *
merge_isofs(path)
	char	*path;
{
	char		buffer[SECTOR_SIZE];
	int		file_addr;
	int		i;
	int		sum = 0;
	char		*p = buffer;
	struct iso_primary_descriptor *pri = NULL;
	struct iso_directory_record *rootp;
	struct iso_volume_descriptor *vdp;

	/*
	 * Start by searching for the volume header. Ultimately, we need to
	 * search for volume headers in multiple places because we might be
	 * starting with a multisession image. FIXME(eric).
	 */
	get_session_start(&file_addr);

	for (i = 0; i < 100; i++) {
		if (readsecs(file_addr, buffer,
				sizeof (buffer) / SECTOR_SIZE) != sizeof (buffer)) {
			comerr(_("Read error on old image %s\n"), path);
		}
		vdp = (struct iso_volume_descriptor *)buffer;

		if ((strncmp(vdp->id, ISO_STANDARD_ID, sizeof (vdp->id)) == 0) &&
		    (get_711(vdp->type) == ISO_VD_PRIMARY)) {
			break;
		}
		file_addr += 1;
	}

	if (i == 100) {
		return (NULL);
	}
	for (i = 0; i < 2048-3; i++) {
		sum += p[i] & 0xFF;
	}
	pri = (struct iso_primary_descriptor *)vdp;

	/* Check the blocksize of the image to make sure it is compatible. */
	if (get_723(pri->logical_block_size) != SECTOR_SIZE) {
		errmsgno(EX_BAD,
			_("Previous session has incompatible sector size %u.\n"),
			get_723(pri->logical_block_size));
		return (NULL);
	}
	if (get_723(pri->volume_set_size) != 1) {
		errmsgno(EX_BAD,
			_("Previous session has volume set size %u (must be 1).\n"),
			get_723(pri->volume_set_size));
		return (NULL);
	}
	/* Get the location and size of the root directory. */
	rootp = (struct iso_directory_record *)
		e_malloc(sizeof (struct iso_directory_record));

	memcpy(rootp, pri->root_directory_record,
				sizeof (pri->root_directory_record));

	for (i = 0; i < 100; i++) {
		if (readsecs(file_addr, buffer,
				sizeof (buffer) / SECTOR_SIZE) != sizeof (buffer)) {
			comerr(_("Read error on old image %s\n"), path);
		}
		if (strncmp(buffer, "MKI ", 4) == 0) {
			int	sum2;

			sum2  = p[2045] & 0xFF;
			sum2 *= 256;
			sum2 += p[2046] & 0xFF;
			sum2 *= 256;
			sum2 += p[2047] & 0xFF;
			if (sum == sum2) {
				error(_("ISO-9660 image includes checksum signature for correct inode numbers.\n"));
			} else {
				correct_inodes = FALSE;
				rrip112 = FALSE;
			}
			break;
		}
		file_addr += 1;
	}

	return (rootp);
}

LOCAL void
merge_remaining_entries(this_dir, pnt, n_orig)
	struct directory *this_dir;
	struct directory_entry **pnt;
	int		n_orig;
{
	int		i;
	struct directory_entry *s_entry;
	UInt32_t	ttbl_extent = 0;
	unsigned int	ttbl_index = 0;
	char		whole_path[PATH_MAX];

	/*
	 * Whatever is leftover in the list needs to get merged back into the
	 * directory.
	 */
	for (i = 0; i < n_orig; i++) {
		if (pnt[i] == NULL) {
			continue;
		}
		if (pnt[i]->name != NULL && pnt[i]->whole_name == NULL) {
			/* Set the name for this directory. */
			strlcpy(whole_path, this_dir->de_name,
							sizeof (whole_path));
			strcat(whole_path, SPATH_SEPARATOR);
			strcat(whole_path, pnt[i]->name);

			pnt[i]->whole_name = e_strdup(whole_path);
		}
		if (pnt[i]->name != NULL &&
/*			strcmp(pnt[i]->name, "<translation table>") == 0 )*/
			strcmp(pnt[i]->name, trans_tbl) == 0) {
			ttbl_extent =
			    get_733(pnt[i]->isorec.extent);
			ttbl_index = i;
			continue;
		}

		/*
		 * Skip directories for now - these need to be treated
		 * differently.
		 */
		if ((pnt[i]->isorec.flags[0] & ISO_DIRECTORY) != 0) {
			/*
			 * FIXME - we need to insert this directory into the
			 * tree, so that the path tables we generate will be
			 * correct.
			 */
			if ((strcmp(pnt[i]->name, ".") == 0) ||
				(strcmp(pnt[i]->name, "..") == 0)) {
				free_directory_entry(pnt[i]);
				pnt[i] = NULL;
				continue;
			} else {
				merge_old_directory_into_tree(pnt[i], this_dir);
			}
		}
		pnt[i]->next = this_dir->contents;
		pnt[i]->filedir = this_dir;
		this_dir->contents = pnt[i];
		pnt[i] = NULL;
	}


	/*
	 * If we don't have an entry for the translation table, then don't
	 * bother trying to copy the starting extent over. Note that it is
	 * possible that if we are copying the entire directory, the entry for
	 * the translation table will have already been inserted into the
	 * linked list and removed from the old entries list, in which case we
	 * want to leave the extent number as it was before.
	 */
	if (ttbl_extent == 0) {
		return;
	}
	/*
	 * Finally, check the directory we are creating to see whether there
	 * are any new entries in it.  If there are not, we can reuse the same
	 * translation table.
	 */
	for (s_entry = this_dir->contents; s_entry; s_entry = s_entry->next) {
		/*
		 * Don't care about '.' or '..'.  They are never in the table
		 * anyways.
		 */
		if (s_entry->name != NULL && strcmp(s_entry->name, ".") == 0) {
			continue;
		}
		if (s_entry->name != NULL && strcmp(s_entry->name, "..") == 0) {
			continue;
		}
/*		if (s_entry->name != NULL &&*/
/*		    strcmp(s_entry->name, "<translation table>") == 0)*/
		if (s_entry->name != NULL &&
		    strcmp(s_entry->name, trans_tbl) == 0) {
			continue;
		}
		if ((s_entry->de_flags & SAFE_TO_REUSE_TABLE_ENTRY) == 0) {
			return;
		}
	}

	/*
	 * Locate the translation table, and re-use the same extent. It isn't
	 * clear that there should ever be one in there already so for now we
	 * try and muddle through the best we can.
	 */
	for (s_entry = this_dir->contents; s_entry; s_entry = s_entry->next) {
/*		if (strcmp(s_entry->name, "<translation table>") == 0)*/
		if (strcmp(s_entry->name, trans_tbl) == 0) {
			fprintf(stderr, "Should never get here\n");
			set_733(s_entry->isorec.extent, ttbl_extent);
			return;
		}
	}

	pnt[ttbl_index]->next = this_dir->contents;
	pnt[ttbl_index]->filedir = this_dir;
	this_dir->contents = pnt[ttbl_index];
	pnt[ttbl_index] = NULL;
}


/*
 * Here we have a case of a directory that has completely disappeared from
 * the face of the earth on the tree we are mastering from.  Go through and
 * merge it into the tree, as well as everything beneath it.
 *
 * Note that if a directory has been moved for some reason, this will
 * incorrectly pick it up and attempt to merge it back into the old
 * location.  FIXME(eric).
 */
LOCAL int
merge_old_directory_into_tree(dpnt, parent)
	struct directory_entry	*dpnt;
	struct directory *parent;
{
	struct directory_entry **contents = NULL;
	int		i;
	int		n_orig;
	struct directory *this_dir,
			*next_brother;
	char		whole_path[PATH_MAX];

	this_dir = (struct directory *)e_malloc(sizeof (struct directory));
	memset(this_dir, 0, sizeof (struct directory));
	this_dir->next = NULL;
	this_dir->subdir = NULL;
	this_dir->self = dpnt;
	this_dir->contents = NULL;
	this_dir->size = 0;
	this_dir->extent = 0;
	this_dir->depth = parent->depth + 1;
	this_dir->parent = parent;
	if (!parent->subdir)
		parent->subdir = this_dir;
	else {
		next_brother = parent->subdir;
		while (next_brother->next)
			next_brother = next_brother->next;
		next_brother->next = this_dir;
	}

	/* Set the name for this directory. */
	if (strlcpy(whole_path, parent->de_name, sizeof (whole_path)) >= sizeof (whole_path) ||
	    strlcat(whole_path, SPATH_SEPARATOR, sizeof (whole_path)) >= sizeof (whole_path) ||
	    strlcat(whole_path, dpnt->name, sizeof (whole_path)) >= sizeof (whole_path))
		comerrno(EX_BAD, _("Path name '%s%s%s' exceeds max length %zd\n"),
					parent->de_name,
					SPATH_SEPARATOR,
					dpnt->name,
					sizeof (whole_path));
	this_dir->de_name = e_strdup(whole_path);
	this_dir->whole_name = e_strdup(whole_path);

	/*
	 * Now fill this directory using information from the previous session.
	 */
	contents = read_merging_directory(&dpnt->isorec, &n_orig);
	/*
	 * Start by simply copying the '.', '..' and non-directory entries to
	 * this directory.  Technically we could let merge_remaining_entries
	 * handle this, but it gets rather confused by the '.' and '..' entries
	 */
	for (i = 0; i < n_orig; i++) {
		/*
		 * We can always reuse the TRANS.TBL in this particular case.
		 */
		contents[i]->de_flags |= SAFE_TO_REUSE_TABLE_ENTRY;

		if (((contents[i]->isorec.flags[0] & ISO_DIRECTORY) != 0) &&
							(i >= 2)) {
			continue;
		}
		/* If we have a directory, don't reuse the extent number. */
		if ((contents[i]->isorec.flags[0] & ISO_DIRECTORY) != 0) {
			memset(contents[i]->isorec.extent, 0, 8);

			if (strcmp(contents[i]->name, ".") == 0)
				this_dir->dir_flags |= DIR_HAS_DOT;

			if (strcmp(contents[i]->name, "..") == 0)
				this_dir->dir_flags |= DIR_HAS_DOTDOT;
		}
		/*
		 * for regilar files, we do it here.
		 * If it has CL or RE attributes, remember its extent
		 */
		check_rr_relocation(contents[i]);

		/*
		 * Set the whole name for this file.
		 */
		if (strlcpy(whole_path, this_dir->whole_name, sizeof (whole_path)) >= sizeof (whole_path) ||
		    strlcat(whole_path, SPATH_SEPARATOR, sizeof (whole_path)) >= sizeof (whole_path) ||
		    strlcat(whole_path, contents[i]->name, sizeof (whole_path)) >= sizeof (whole_path))
			comerrno(EX_BAD, _("Path name '%s%s%s' exceeds max length %zd\n"),
						this_dir->whole_name,
						SPATH_SEPARATOR,
						contents[i]->name,
						sizeof (whole_path));

		contents[i]->whole_name = e_strdup(whole_path);

		contents[i]->next = this_dir->contents;
		contents[i]->filedir = this_dir;
		this_dir->contents = contents[i];
		contents[i] = NULL;
	}

	/*
	 * and for directories, we do it here.
	 * If it has CL or RE attributes, remember its extent
	 */
	check_rr_relocation(dpnt);

	/*
	 * Zero the extent number for ourselves.
	 */
	memset(dpnt->isorec.extent, 0, 8);

	/*
	 * Anything that is left are other subdirectories that need to be
	 * merged.
	 */
	merge_remaining_entries(this_dir, contents, n_orig);
	free_mdinfo(contents, n_orig);
#if 0
	/*
	 * This is no longer required.  The post-scan sort will handle all of
	 * this for us.
	 */
	sort_n_finish(this_dir);
#endif

	return (0);
}


char	*cdrecord_data = NULL;

int
get_session_start(file_addr)
	int		*file_addr;
{
	char		*pnt;

#ifdef CDRECORD_DETERMINES_FIRST_WRITABLE_ADDRESS
	/*
	 * FIXME(eric).  We need to coordinate with cdrecord to obtain the
	 * parameters.  For now, we assume we are writing the 2nd session, so
	 * we start from the session that starts at 0.
	 */
	if (file_addr != NULL)
		*file_addr = 16;

	/*
	 * We need to coordinate with cdrecord to get the next writable address
	 * from the device.  Here is where we use it.
	 */
	session_start = last_extent = last_extent_written = cdrecord_result();
#else

	if (file_addr != NULL)
		*file_addr = 0L;
	session_start = last_extent = last_extent_written = 0L;
	if (check_session && cdrecord_data == NULL)
		return (0);

	if (cdrecord_data == NULL) {
		comerrno(EX_BAD,
		    _("Special parameters for cdrecord not specified with -C\n"));
	}
	/*
	 * Next try and find the ',' in there which delimits the two numbers.
	 */
	pnt = strchr(cdrecord_data, ',');
	if (pnt == NULL) {
		comerrno(EX_BAD, _("Malformed cdrecord parameters\n"));
	}

	*pnt = '\0';
	if (file_addr != NULL) {
		*file_addr = atol(cdrecord_data);
	}
	pnt++;

	session_start = last_extent = last_extent_written = atol(pnt);

	pnt--;
	*pnt = ',';

#endif
	return (0);
}

/*
 * This function scans the directory tree, looking for files, and it makes
 * note of everything that is found.  We also begin to construct the ISO9660
 * directory entries, so that we can determine how large each directory is.
 */
int
merge_previous_session(this_dir, mrootp, reloc_root, reloc_old_root)
	struct directory *this_dir;
	struct iso_directory_record *mrootp;
	char *reloc_root;
	char *reloc_old_root;
{
	struct directory_entry **orig_contents = NULL;
	struct directory_entry *odpnt = NULL;
	int		n_orig;
	struct directory_entry *s_entry;
	int		status;
	int		lstatus;
	struct stat	statbuf,
			lstatbuf;
	int		retcode;

	/* skip leading slash */
	while (reloc_old_root && reloc_old_root[0] == PATH_SEPARATOR) {
		reloc_old_root++;
	}
	while (reloc_root && reloc_root[0] == PATH_SEPARATOR) {
		reloc_root++;
	}

	/*
	 * Parse the same directory in the image that we are merging for
	 * multisession stuff.
	 */
	orig_contents = read_merging_directory(mrootp, &n_orig);
	if (orig_contents == NULL) {
		if (reloc_old_root) {
			comerrno(EX_BAD,
			_("Reading old session failed, cannot execute -old-root.\n"));
		}
		return (0);
	}

	if (reloc_old_root && reloc_old_root[0]) {
		struct directory_entry	**new_orig_contents = orig_contents;
		int			new_n_orig = n_orig;

		/* decend until we reach the original root */
		while (reloc_old_root[0]) {
			int	i;
			char	*next;
			int	last;

			for (next = reloc_old_root; *next && *next != PATH_SEPARATOR; next++);
			if (*next) {
				last = 0;
				*next = 0;
				next++;
			} else {
				last = 1;
			}
			while (*next == PATH_SEPARATOR) {
				next++;
			}

			for (i = 0; i < new_n_orig; i++) {
				struct iso_directory_record subroot;

				if (new_orig_contents[i]->name != NULL &&
				    strcmp(new_orig_contents[i]->name, reloc_old_root) != 0) {
					/* Not the same name continue */
					continue;
				}
				/*
				 * enter directory, free old one only if not the top level,
				 * which is still needed
				 */
				subroot = new_orig_contents[i]->isorec;
				if (new_orig_contents != orig_contents) {
					free_mdinfo(new_orig_contents, new_n_orig);
				}
				new_orig_contents = read_merging_directory(&subroot, &new_n_orig);

				if (!new_orig_contents) {
					comerrno(EX_BAD,
					_("Reading directory %s in old session failed, cannot execute -old-root.\n"),
							reloc_old_root);
				}
				i = -1;
				break;
			}

			if (i == new_n_orig) {
				comerrno(EX_BAD,
				_("-old-root (sub)directory %s not found in old session.\n"),
						reloc_old_root);
			}

			/* restore string, proceed to next sub directory */
			if (!last) {
				reloc_old_root[strlen(reloc_old_root)] = PATH_SEPARATOR;
			}
			reloc_old_root = next;
		}

		/*
		 * preserve the old session, skipping those dirs/files that are found again
		 * in the new root
		 */
		for (s_entry = this_dir->contents; s_entry; s_entry = s_entry->next) {
			status = stat_filter(s_entry->whole_name, &statbuf);
			lstatus = lstat_filter(s_entry->whole_name, &lstatbuf);

			/*
			 * check_prev_session() will search for s_entry and remove it from
			 * orig_contents if found
			 */
			retcode = check_prev_session(orig_contents, n_orig, s_entry,
			    &statbuf, &lstatbuf, NULL);
			if (retcode == -1)
				return (-1);
			/*
			 * Skip other directory entries for multi-extent files
			 */
			if (s_entry->de_flags & MULTI_EXTENT) {
				struct directory_entry	*s_e;

				for (s_e = s_entry->mxroot;
					s_e && s_e->mxroot == s_entry->mxroot;
						s_e = s_e->next) {
					s_entry = s_e;
					;
				}
			}
		}
		merge_remaining_entries(this_dir, orig_contents, n_orig);

		/* use new directory */
		free_mdinfo(orig_contents, n_orig);
		orig_contents = new_orig_contents;
		n_orig = new_n_orig;

		if (reloc_root && reloc_root[0]) {
			/* also decend into new root before searching for files */
			this_dir = find_or_create_directory(this_dir, reloc_root, NULL, TRUE);
			if (!this_dir) {
				return (-1);
			}
		}
	}


	/*
	 * Now we scan the directory itself, and look at what is inside of it.
	 */
	for (s_entry = this_dir->contents; s_entry; s_entry = s_entry->next) {
		status = stat_filter(s_entry->whole_name, &statbuf);
		lstatus = lstat_filter(s_entry->whole_name, &lstatbuf);

		/*
		 * We always should create an entirely new directory tree
		 * whenever we generate a new session, unless there were
		 * *no* changes whatsoever to any of the directories, in which
		 * case it would be kind of pointless to generate a new
		 * session.
		 * I believe it is possible to rigorously prove that any change
		 * anywhere in the filesystem will force the entire tree to be
		 * regenerated because the modified directory will get a new
		 * extent number.  Since each subdirectory of the changed
		 * directory has a '..' entry, all of them will need to be
		 * rewritten too, and since the parent directory of the
		 * modified directory will have an extent pointer to the
		 * directory it too will need to be rewritten.  Thus we will
		 * never be able to reuse any directory information when
		 * writing new sessions.
		 *
		 * We still check the previous session so we can mark off the
		 * equivalent entry in the list we got from the original disc,
		 * however.
		 */

		/*
		 * The check_prev_session function looks for an identical
		 * entry in the previous session.  If we see it, then we copy
		 * the extent number to s_entry, and cross it off the list.
		 */
		retcode = check_prev_session(orig_contents, n_orig, s_entry,
			&statbuf, &lstatbuf, &odpnt);
		if (retcode == -1)
			return (-1);

		if (odpnt != NULL &&
		    (s_entry->isorec.flags[0] & ISO_DIRECTORY) != 0) {
			int	dflag;

			if (strcmp(s_entry->name, ".") != 0 &&
					strcmp(s_entry->name, "..") != 0) {
				struct directory *child;

				/*
				 * XXX It seems that the tree that has been
				 * XXX read from the previous session does not
				 * XXX carry whole_name entries. We provide a
				 * XXX hack in
				 * XXX multi.c:find_or_create_directory()
				 * XXX that should be removed when a
				 * XXX reasonable method could be found.
				 */
				child = find_or_create_directory(this_dir,
					s_entry->whole_name,
					s_entry, 1);
				dflag = merge_previous_session(child,
					&odpnt->isorec,
					NULL, reloc_old_root);
				if (dflag == -1) {
					return (-1);
				}
				free(odpnt);
				odpnt = NULL;
			}
		}
		if (odpnt) {
			free(odpnt);
			odpnt = NULL;
		}
		/*
		 * Skip other directory entries for multi-extent files
		 */
		if (s_entry->de_flags & MULTI_EXTENT) {
			struct directory_entry	*s_e;

			for (s_e = s_entry->mxroot;
				s_e && s_e->mxroot == s_entry->mxroot;
					s_e = s_e->next) {
				s_entry = s_e;
				;
			}
		}
	}

	if (!reloc_old_root) {
		/*
		 * Whatever is left over, are things which are no longer in the tree on
		 * disk. We need to also merge these into the tree.
		 */
		merge_remaining_entries(this_dir, orig_contents, n_orig);
	}
	free_mdinfo(orig_contents, n_orig);
	return (1);
}

/*
 * This code deals with relocated directories which may exist
 * in the previous session.
 */
struct dir_extent_link  {
	unsigned int		extent;
	struct directory_entry	*de;
	struct dir_extent_link	*next;
};

static struct dir_extent_link	*cl_dirs = NULL;
static struct dir_extent_link	*re_dirs = NULL;

LOCAL void
check_rr_relocation(de)
	struct directory_entry *de;
{
	unsigned char	sector[SECTOR_SIZE];
	unsigned char	*pnt = de->rr_attributes;
		int	len = de->rr_attr_size;
		UInt32_t cont_extent = 0,
			cont_offset = 0,
			cont_size = 0;

	pnt = parse_xa(pnt, &len, /* dpnt */ 0);
	while (len >= 4) {
		if (pnt[3] != 1 && pnt[3] != 2) {
			errmsgno(EX_BAD,
				_("**BAD RRVERSION (%d) in '%c%c' field (%2.2X %2.2X).\n"),
				pnt[3], pnt[0], pnt[1], pnt[0], pnt[1]);
		}
		if (pnt[2] < 4) {
			errmsgno(EX_BAD,
				_("**BAD RRLEN (%d) in '%2.2s' field %2.2X %2.2X.\n"),
				pnt[2], pnt, pnt[0], pnt[1]);
			break;
		}
		if (strncmp((char *)pnt, "CL", 2) == 0) {
			struct dir_extent_link *dlink = e_malloc(sizeof (*dlink));

			dlink->extent = get_733(pnt + 4);
			dlink->de = de;
			dlink->next = cl_dirs;
			cl_dirs = dlink;

		} else if (strncmp((char *)pnt, "RE", 2) == 0) {
			struct dir_extent_link *dlink = e_malloc(sizeof (*dlink));

			dlink->extent = de->starting_block;
			dlink->de = de;
			dlink->next = re_dirs;
			re_dirs = dlink;

		} else if (strncmp((char *)pnt, "CE", 2) == 0) {
			cont_extent = get_733(pnt + 4);
			cont_offset = get_733(pnt + 12);
			cont_size = get_733(pnt + 20);

		} else if (strncmp((char *)pnt, "ST", 2) == 0) {
			len = pnt[2];
		}
		len -= pnt[2];
		pnt += pnt[2];
		if (len <= 3 && cont_extent) {
			/* ??? What if cont_offset+cont_size > SECTOR_SIZE */
			readsecs(cont_extent, sector, 1);
			pnt = sector + cont_offset;
			len = cont_size;
			cont_extent = cont_offset = cont_size = 0;
		}
	}

}

void
match_cl_re_entries()
{
	struct dir_extent_link *re = re_dirs;

	/* for each relocated directory */
	for (; re; re = re->next) {
		struct dir_extent_link *cl = cl_dirs;

		for (; cl; cl = cl->next) {
			/* find a place where it was relocated from */
			if (cl->extent == re->extent) {
				/* set link to that place */
				re->de->parent_rec = cl->de;
				re->de->filedir = cl->de->filedir;

				/*
				 * see if it is in rr_moved
				 */
				if (reloc_dir != NULL) {
					struct directory_entry *rr_moved_e = reloc_dir->contents;

					for (; rr_moved_e; rr_moved_e = rr_moved_e->next) {
						/* yes it is */
						if (re->de == rr_moved_e) {
							/* forget it */
							re->de = NULL;
						}
					}
				}
				break;
			}
		}
	}
}

void
finish_cl_pl_for_prev_session()
{
	struct dir_extent_link *re = re_dirs;

	/* for those that were relocated, but NOT to rr_moved */
	re = re_dirs;
	for (; re; re = re->next) {
		if (re->de != NULL) {
			/*
			 * here we have hypothetical case when previous session
			 * was not created by mkisofs and contains relocations
			 */
			struct directory_entry *s_entry = re->de;
			struct directory_entry *s_entry1;
			struct directory *d_entry = reloc_dir->subdir;

			/* do the same as finish_cl_pl_entries */
			if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY) {
				continue;
			}
			while (d_entry) {
				if (d_entry->self == s_entry)
					break;
				d_entry = d_entry->next;
			}
			if (!d_entry) {
				comerrno(EX_BAD, _("Unable to locate directory parent\n"));
			}

			if (s_entry->filedir != NULL && s_entry->parent_rec != NULL) {
				char	*rr_attr;

				/*
				 * First fix the PL pointer in the directory in the
				 * rr_reloc dir
				 */
				s_entry1 = d_entry->contents->next;
				rr_attr = find_rr_attribute(s_entry1->rr_attributes,
					s_entry1->total_rr_attr_size, "PL");
				if (rr_attr != NULL)
					set_733(rr_attr + 4, s_entry->filedir->extent);

				/* Now fix the CL pointer */
				s_entry1 = s_entry->parent_rec;

				rr_attr = find_rr_attribute(s_entry1->rr_attributes,
					s_entry1->total_rr_attr_size, "CL");
				if (rr_attr != NULL)
					set_733(rr_attr + 4, d_entry->extent);
			}
		}
	}
	/* free memory */
	re = re_dirs;
	while (re) {
		struct dir_extent_link *next = re->next;

		free(re);
		re = next;
	}
	re = cl_dirs;
	while (re) {
		struct dir_extent_link *next = re->next;

		free(re);
		re = next;
	}
}
