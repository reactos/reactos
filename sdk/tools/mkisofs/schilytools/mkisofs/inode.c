/* @(#)inode.c	1.19 16/11/14 Copyright 2006-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)inode.c	1.19 16/11/14 Copyright 2006-2015 J. Schilling";
#endif
/*
 *	Inode and link count handling for ISO-9660/RR
 *
 *	This module computes and sets up a RR link count that reflects
 *	the name-count for files/directories in the ISO-9660/RR image.
 *	This module also assigns inode numbers tp all files/directories
 *	using either the RRip-112 protocol or a mkisofs specific method
 *	of asigning the related number to the "extent" field in the ISO
 *	directory record.
 *
 *	Copyright (c) 2006-2016 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "mkisofs.h"
#include <schily/schily.h>

/*
 * Highest inode value we assign in this session.
 */
LOCAL	UInt32_t null_ino_high;

EXPORT	void	do_inode		__PR((struct directory *dpnt));
EXPORT	void	do_dir_nlink		__PR((struct directory *dpnt));
LOCAL	void	assign_inodes		__PR((struct directory *dpnt));
LOCAL	void	compute_linkcount	__PR((struct directory *dpnt));
LOCAL	void	assign_linkcount	__PR((struct directory *dpnt));
LOCAL	void	update_inode		__PR((struct directory_entry *s_entry, int value));
LOCAL	void	update_nlink		__PR((struct directory_entry *s_entry, int value));
LOCAL	int	update_dir_nlink	__PR((struct directory *dpnt));

/*
 * Inode/hard link related stuff for non-directory type files.
 */
EXPORT void
do_inode(dpnt)
	struct directory	*dpnt;
{
	null_ino_high = null_inodes;

	if (correct_inodes)
		assign_inodes(root);

#ifdef	UDF
	if (!use_RockRidge && !use_udf)
		return;
#else
	if (!use_RockRidge)
		return;
#endif
	if (!cache_inodes)		/* Never FALSE if correct_inodes TRUE */
		return;

	compute_linkcount(dpnt);
	if (use_RockRidge)		/* If we have Rock Ridge extensions, */
		assign_linkcount(dpnt);	/* reassign computed linkcount in RR */

	if (null_inodes < last_extent)
		comerrno(EX_BAD, _("Inode number overflow, too many files in file system.\n"));
}

/*
 * Set the link count for directories to 2 + number of sub-directories.
 */
EXPORT void
do_dir_nlink(dpnt)
	struct directory	*dpnt;
{
	int	rootlinks;

	if (!use_RockRidge)
		return;

	/*
	 * Update everything except "/..".
	 */
	rootlinks = update_dir_nlink(dpnt);
	if (reloc_dir)
		rootlinks--;	/* rr_moved is hidden */
	/*
	 * Update "/." now.
	 */
	update_nlink(dpnt->contents, rootlinks);
	/*
	 * Update "/.." now.
	 */
	update_nlink(dpnt->contents->next, rootlinks);
}

/*
 * Assign inode numbers to files of zero size and to symlinks.
 */
LOCAL void
assign_inodes(dpnt)
	struct directory	*dpnt;
{
	struct directory_entry	*s_entry;
	struct file_hash	*s_hash;

	while (dpnt) {
		s_entry = dpnt->contents;
		for (s_entry = dpnt->contents; s_entry; s_entry = s_entry->next) {
			if (s_entry->starting_block == 0) {
				s_hash = find_hash(s_entry);
				/* find_directory_hash() ? */
				if (s_hash)
					s_entry->starting_block = s_hash->starting_block;
			}
			if (s_entry->starting_block == 0 && s_entry->size != 0) {
				unsigned int e = get_733((char *)s_entry->isorec.extent);

				if (e != 0) {
					errmsgno(EX_BAD,
					_("Implementation botch, fetching extend %d for %s from dir entry.\n"),
					e, s_entry->whole_name);
				}
			}
			if (use_RockRidge && s_entry->starting_block > 0)
				update_inode(s_entry, s_entry->starting_block);

			/*
			 * Be careful: UDF Symlinks have size != 0, then
			 * s_hash->starting_block is a valid inode number.
			 */
			if (s_entry->size != 0)
				continue;
#ifdef	UDF
			if ((s_entry->de_flags & IS_SYMLINK) != 0 &&
			    create_udfsymlinks)
				continue;
#else
			if ((s_entry->de_flags & IS_SYMLINK) != 0)
				continue;
#endif

			if (s_entry->isorec.flags[0] & ISO_DIRECTORY)
				continue;

			/*
			 * Assign inodes to symbolic links.
			 */
			if (s_entry->dev == UNCACHED_DEVICE && s_entry->inode == UNCACHED_INODE) {
				s_entry->dev = PREV_SESS_DEV;
				s_entry->inode = null_inodes;
			}
			s_hash = find_hash(s_entry);
			if (s_hash) {
				/*
				 * Paranoia: Check for hashed files without proper inode #.
				 */
				if (s_hash->starting_block <= last_extent)
					comerrno(EX_BAD,
					_("Implementation botch: Hashed file '%s' has illegal inode %u.\n"),
					s_entry->whole_name ?
					s_entry->whole_name : s_entry->name,
					s_hash->starting_block);
				set_733((char *)s_entry->isorec.extent, s_hash->starting_block);
				s_entry->starting_block = s_hash->starting_block;
			} else {
				s_entry->starting_block = null_inodes--;
				set_733((char *)s_entry->isorec.extent, s_entry->starting_block);
				add_hash(s_entry);
			}
			if (use_RockRidge)
				update_inode(s_entry, s_entry->starting_block);
		}
		if (dpnt->subdir) {
			assign_inodes(dpnt->subdir);
		}

		dpnt = dpnt->next;
	}
}

/*
 * Compute the link count for non-directory type files.
 */
LOCAL void
compute_linkcount(dpnt)
	struct directory	*dpnt;
{
	struct directory_entry	*s_entry;
	struct file_hash	*s_hash;

	while (dpnt) {
		s_entry = dpnt->contents;
		for (s_entry = dpnt->contents; s_entry; s_entry = s_entry->next) {
			/*
			 * Skip directories.
			 */
			if (s_entry->isorec.flags[0] & ISO_DIRECTORY)
				continue;
			if (s_entry->de_flags & RELOCATED_DIRECTORY)
				continue;

			/*
			 * skip resource files or file stream files
			 * XXX should we assign a standard link count == 1 instead?
			 */
			if (s_entry->de_flags & RESOURCE_FORK)
				continue;

			/*
			 * Assign inodes to symbolic links.
			 * We never come here in case that we create correct inodes,
			 * except with UDF symlinks.
			 */
			if (s_entry->dev == UNCACHED_DEVICE && s_entry->inode == UNCACHED_INODE) {
				s_entry->dev = PREV_SESS_DEV;

				/*
				 * With UDF symlinks, the starting_block is a
				 * valid inode number.
				 */
#ifdef	UDF
				if ((s_entry->de_flags & IS_SYMLINK) != 0 &&
				    create_udfsymlinks) {
#else
				if ((s_entry->de_flags & IS_SYMLINK) != 0) {
#endif
					s_entry->inode = s_entry->starting_block;
				} else {
					s_entry->inode = null_inodes--;	/* Only used for caching */
					if (correct_inodes)
						comerrno(EX_BAD,
						_("Implementation botch: Unhashed file '%s'.\n"),
						s_entry->whole_name ?
						s_entry->whole_name : s_entry->name);
				}
			}
			s_hash = find_hash(s_entry);
			if (s_hash) {
				s_hash->nlink++;
			} else {
				add_hash(s_entry);
				s_hash = find_hash(s_entry);
				if (s_hash == NULL) {
					if (s_entry->dev == UNCACHED_DEVICE &&
					    s_entry->inode == TABLE_INODE) {
						continue;
					}
					comerrno(EX_BAD,
					_("Implementation botch: File '%s' not hashed (dev/ino %llX/%llX).\n"),
					s_entry->whole_name ?
					s_entry->whole_name : s_entry->name,
					(Llong)s_entry->dev,
					(Llong)s_entry->inode);
				}
				s_hash->nlink++;
			}
		}
		if (dpnt->subdir) {
			compute_linkcount(dpnt->subdir);
		}

		dpnt = dpnt->next;
	}
}

/*
 * Assig the link count for non-directory type files to the value
 * computed with compute_linkcount().
 */
LOCAL void
assign_linkcount(dpnt)
	struct directory	*dpnt;
{
	struct directory_entry	*s_entry;
	struct file_hash	*s_hash;

	while (dpnt) {
		s_entry = dpnt->contents;
		for (s_entry = dpnt->contents; s_entry; s_entry = s_entry->next) {
			if (s_entry->isorec.flags[0] & ISO_DIRECTORY)
				continue;
			if (s_entry->de_flags & RELOCATED_DIRECTORY)
				continue;
			/*
			 * skip resource files or file stream files
			 */
			if (s_entry->de_flags & RESOURCE_FORK)
				continue;

			s_hash = find_hash(s_entry);
			if (s_hash) {
				update_nlink(s_entry, s_hash->nlink);
			} else {
				if (s_entry->dev == UNCACHED_DEVICE &&
				    s_entry->inode == TABLE_INODE) {
					continue;
				}
				comerrno(EX_BAD,
				_("Implementation botch: File '%s' not hashed.\n"),
					s_entry->whole_name ?
					s_entry->whole_name : s_entry->name);
			}
		}
		if (dpnt->subdir) {
			assign_linkcount(dpnt->subdir);
		}

		dpnt = dpnt->next;
	}
}

/*
 * Rewrite the content of the RR inode field in the PX record.
 */
LOCAL void
update_inode(s_entry, value)
	struct directory_entry	*s_entry;
	int			value;
{
	unsigned char	*pnt;
	int		len;

	if (!rrip112)
		return;

	pnt = s_entry->rr_attributes;
	len = s_entry->total_rr_attr_size;
	pnt = parse_xa(pnt, &len, 0);
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
		if (pnt[0] == 'P' && pnt[1] == 'X') {
			if ((pnt[2] & 0xFF) < 44)	/* Paranoia */
				return;
			set_733((char *)pnt + 36, value);
			break;
		}
		len -= pnt[2];
		pnt += pnt[2];
	}
}

/*
 * Rewrite the content of the RR nlink field in the PX record.
 */
LOCAL void
update_nlink(s_entry, value)
	struct directory_entry	*s_entry;
	int			value;
{
	unsigned char	*pnt;
	int		len;

	pnt = s_entry->rr_attributes;
	len = s_entry->total_rr_attr_size;
	pnt = parse_xa(pnt, &len, 0);
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
		if (pnt[0] == 'P' && pnt[1] == 'X') {
			set_733((char *)pnt + 12, value);
			break;
		}
		len -= pnt[2];
		pnt += pnt[2];
	}
}

/*
 * Set the link count for directories to 2 + number of sub-directories.
 * This is done here for all diresctories except for "/..".
 */
LOCAL int
update_dir_nlink(dpnt)
	struct directory *dpnt;
{
	struct directory *xpnt;
	struct directory_entry *s_entry;
	int		i = 0;

	while (dpnt) {
		if (dpnt->dir_flags & INHIBIT_ISO9660_ENTRY) {
			dpnt = dpnt->next;
			continue;
		}
		/*
		 * First, count up the number of subdirectories this dir has.
		 */
		for (i = 0, xpnt = dpnt->subdir; xpnt; xpnt = xpnt->next)
			if ((xpnt->dir_flags & INHIBIT_ISO9660_ENTRY) == 0)
				i++;
		/*
		 * Next check to see if we have any relocated directories in
		 * this directory. The nlink field will include these as
		 * real directories when they are properly relocated.
		 * In the non-rockridge disk, the relocated entries appear as
		 * zero length files.
		 */
		for (s_entry = dpnt->contents; s_entry;
						s_entry = s_entry->next) {
			if ((s_entry->de_flags & RELOCATED_DIRECTORY) != 0 &&
				(s_entry->de_flags & INHIBIT_ISO9660_ENTRY) ==
									0) {
				i++;
			}
		}
		/*
		 * Now update the field in the Rock Ridge entry.
		 */
		update_nlink(dpnt->self, i + 2);

		/*
		 * Update the '.' entry for this directory.
		 */
		update_nlink(dpnt->contents, i + 2);

		/*
		 * Update all of the '..' entries that point to this guy.
		 */
		for (xpnt = dpnt->subdir; xpnt; xpnt = xpnt->next) {
			update_nlink(xpnt->contents->next, i + 2);
		}

		if (dpnt->subdir)
			update_dir_nlink(dpnt->subdir);
		dpnt = dpnt->next;
	}
	return (i+2);
}
