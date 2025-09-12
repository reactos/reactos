/* @(#)tree.c	1.137 16/12/13 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)tree.c	1.137 16/12/13 joerg";
#endif
/*
 * File tree.c - scan directory  tree and build memory structures for iso9660
 * filesystem
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999,2000-2016 J. Schilling
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
/* ADD_FILES changes made by Ross Biro biro@yggdrasil.com 2/23/95 */

/* APPLE_HYB James Pearson j.pearson@ge.ucl.ac.uk 23/2/2000 */

/* DUPLICATES_ONCE Alex Kopylov cdrtools@bootcd.ru 19.06.2004 */

#include "mkisofs.h"
#include "rock.h"
#include "match.h"
#include <schily/time.h>
#include <schily/errno.h>
#include <schily/fcntl.h>
#include <schily/device.h>
#include <schily/schily.h>

#ifdef UDF
#include "udf.h"
#endif

#ifdef VMS
#include <sys/file.h>
#include <vms/fabdef.h>
#include "vms.h"
#endif

LOCAL	Uchar	symlink_buff[PATH_MAX+1];

LOCAL	char	*filetype		__PR((int t));
LOCAL	char	*rstr			__PR((char *s1, char *s2));
LOCAL	void	stat_fix		__PR((struct stat *st));
EXPORT	int	stat_filter		__PR((char *path, struct stat *st));
EXPORT	int	lstat_filter		__PR((char *path, struct stat *st));
LOCAL	int	sort_n_finish		__PR((struct directory *this_dir));
LOCAL	void	generate_reloc_directory __PR((void));
EXPORT	void	attach_dot_entries	__PR((struct directory *dirnode,
						struct stat *this_stat,
						struct stat *parent_stat));
EXPORT	char	*find_rr_attribute	__PR((unsigned char *pnt, int len,
						char *attr_type));
EXPORT	void	finish_cl_pl_entries	__PR((void));
LOCAL	void	dir_nesting_warn	__PR((struct directory *this_dir,
						char *path, int contflag));
EXPORT	int	scan_directory_tree	__PR((struct directory *this_dir,
						char *path,
						struct directory_entry *de));
LOCAL	struct directory_entry *
		dup_relocated_dir	__PR((struct directory *this_dir,
					    struct directory_entry *s_entry,
					    char *whole_path,
					    char *short_name,
					    struct stat *statp));
EXPORT	int	insert_file_entry	__PR((struct directory *this_dir,
						char *whole_path,
						char *short_name,
						struct stat *statp, int have_rsrc));
EXPORT	struct directory_entry *
		dup_directory_entry	__PR((struct directory_entry *s_entry));
EXPORT	void	generate_iso9660_directories __PR((struct directory *node,
						FILE *outfile));

LOCAL	void	set_de_path		__PR((struct directory *parent,
						struct directory *this));

EXPORT	struct directory *
		find_or_create_directory __PR((struct directory *parent,
						char *path,
						struct directory_entry *de,
						int flag));
LOCAL	void	delete_directory	__PR((struct directory *parent,
						struct directory *child));
EXPORT	int	sort_tree		__PR((struct directory *node));
EXPORT	void	dump_tree		__PR((struct directory *node));
EXPORT	struct directory_entry *
		search_tree_file	__PR((struct directory *node,
						char *filename));
EXPORT	void	init_fstatbuf		__PR((void));

extern int	verbose;
struct stat	fstatbuf;		/* We use this for the artificial */
					/* entries we create		  */
struct stat	root_statbuf;		/* Stat buffer for root directory */
struct directory *reloc_dir;

LOCAL char *
filetype(t)
	int	t;
{
	static	char	unkn[32];

	if (S_ISFIFO(t))		/* 1 */
		return ("fifo");
	if (S_ISCHR(t))			/* 2 */
		return ("chr");
	if (S_ISMPC(t))			/* 3 */
		return ("multiplexed chr");
	if (S_ISDIR(t))			/* 4 */
		return ("dir");
	if (S_ISNAM(t))			/* 5 */
		return ("named file");
	if (S_ISBLK(t))			/* 6 */
		return ("blk");
	if (S_ISMPB(t))			/* 7 */
		return ("multiplexed blk");
	if (S_ISREG(t))			/* 8 */
		return ("regular file");
	if (S_ISCTG(t))			/* 9 */
		return ("contiguous file");
	if (S_ISLNK(t))			/* 10 */
		return ("symlink");
	if (S_ISSHAD(t))		/* 11 */
		return ("Solaris shadow inode");
	if (S_ISSOCK(t))		/* 12 */
		return ("socket");
	if (S_ISDOOR(t))		/* 13 */
		return ("door");
	if (S_ISWHT(t))			/* 14 */
		return ("whiteout");
	if (S_ISEVC(t))			/* 15 */
		return ("event count");

	/*
	 * Needs to be last in case somebody makes this
	 * a supported file type.
	 */
	if ((t & S_IFMT) == 0)		/* 0 (unallocated) */
		return ("unallocated");

	sprintf(unkn, "octal '%o'", t & S_IFMT);
	return (unkn);
}

/*
 * Check if s1 ends in strings s2
 */
LOCAL char *
rstr(s1, s2)
	char	*s1;
	char	*s2;
{
	int	l1;
	int	l2;

	l1 = strlen(s1);
	l2 = strlen(s2);
	if (l2 > l1)
		return ((char *)NULL);

	if (strcmp(&s1[l1 - l2], s2) == 0)
		return (&s1[l1 - l2]);
	return ((char *)NULL);
}

LOCAL void
stat_fix(st)
	struct stat	*st;
{
	int adjust_modes = 0;

	if (S_ISREG(st->st_mode))
		adjust_modes = rationalize_filemode;
	else if (S_ISDIR(st->st_mode))
		adjust_modes = rationalize_dirmode;
	else
		adjust_modes = (rationalize_filemode || rationalize_dirmode);

	/*
	 * If rationalizing, override the uid and gid, since the
	 * originals will only be useful on the author's system.
	 */
	if (rationalize_uid)
		st->st_uid = uid_to_use;
	if (rationalize_gid)
		st->st_gid = gid_to_use;

	if (adjust_modes) {

		if (S_ISREG(st->st_mode) && (filemode_to_use != 0)) {
			st->st_mode = filemode_to_use | S_IFREG;
		} else if (S_ISDIR(st->st_mode) && (dirmode_to_use != 0)) {
			st->st_mode = dirmode_to_use | S_IFDIR;
		} else {
			/*
			 * Make sure the file modes make sense.  Turn
			 * on all read bits.  Turn on all exec/search
			 * bits if any exec/search bit is set.  Turn
			 * off all write bits, and all special mode
			 * bits (on a r/o fs lock bits are useless,
			 * and with uid+gid 0 don't want set-id bits,
			 * either).
			 */

			st->st_mode |= 0444;
#if !defined(_WIN32) && !defined(__DJGPP__)	/* make all file "executable" */
			if (st->st_mode & 0111)
#endif
				st->st_mode |= 0111;
			st->st_mode &= ~07222;
		}
	}
}

EXPORT int
stat_filter(path, st)
	char		*path;
	struct stat	*st;
{
	int	result = stat(path, st);

	if (result >= 0 && rationalize)
		stat_fix(st);
	return (result);
}

EXPORT int
lstat_filter(path, st)
	char		*path;
	struct stat	*st;
{
	int	result = lstat(path, st);

	if (result >= 0 && rationalize)
		stat_fix(st);
	return (result);
}

LOCAL int
sort_n_finish(this_dir)
	struct directory	*this_dir;
{
	struct directory_entry *s_entry;
	struct directory_entry *s_entry1;
	struct directory_entry *table;
	int		count;
	int		d1;
	int		d2;
	int		d3;
	register int	new_reclen;
	char		*c;
	int		status = 0;
	int		tablesize = 0;
	char		newname[MAX_ISONAME+1];
	char		rootname[MAX_ISONAME+1];
	char		extname[MAX_ISONAME+1];

	/*
	 * Here we can take the opportunity to toss duplicate entries from the
	 * directory.
	 */
	/* ignore if it's hidden */
	if (this_dir->dir_flags & INHIBIT_ISO9660_ENTRY) {
		return (0);
	}
	table = NULL;

	init_fstatbuf();

	/*
	 * If we had artificially created this directory, then we might be
	 * missing the required '.' entries.  Create these now if we need
	 * them.
	 */
	if ((this_dir->dir_flags & (DIR_HAS_DOT | DIR_HAS_DOTDOT)) !=
		(DIR_HAS_DOT | DIR_HAS_DOTDOT)) {
		attach_dot_entries(this_dir, NULL, NULL);
	}
	flush_file_hash();
	s_entry = this_dir->contents;
	while (s_entry) {
#ifdef	USE_LARGEFILES
		/*
		 * Skip all but the last extent from a multi extent file,
		 * we like them all have the same name.
		 */
		if ((s_entry->de_flags & MULTI_EXTENT) &&
		    (s_entry->isorec.flags[0] & ISO_MULTIEXTENT)) {
			s_entry = s_entry->next;
			continue;
		}
#endif
		/* ignore if it's hidden */
		if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY) {
			s_entry = s_entry->next;
			continue;
		}
		/*
		 * First assume no conflict, and handle this case
		 */
		if (!(s_entry1 = find_file_hash(s_entry->isorec.name))) {
			add_file_hash(s_entry);
			s_entry = s_entry->next;
			continue;
		}
#ifdef APPLE_HYB
		/*
		 * if the pair are associated, then skip (as they have the
		 * same name!)
		 */
		if (apple_both && s_entry1->assoc &&
						s_entry1->assoc == s_entry) {
			s_entry = s_entry->next;
			continue;
		}
#endif	/* APPLE_HYB */

		if (s_entry1 == s_entry) {
			comerrno(EX_BAD,
			_("Fatal goof, file '%s' already in hash table.\n"),
			s_entry->isorec.name);
		}
		/*
		 * OK, handle the conflicts.  Try substitute names until we
		 * come up with a winner
		 */
		strlcpy(rootname, s_entry->isorec.name, sizeof (rootname));
		/*
		 * Strip off the non-significant part of the name so that we
		 * are left with a sensible root filename.  If we don't find
		 * a '.', then try a ';'.
		 */
		c = strchr(rootname, '.');
		/*
		 * In case we ever allow more than on dot, only modify the
		 * section past the last dot if the file name starts with a
		 * dot.
		 */
		if (c != NULL && c == rootname && c != strrchr(rootname, '.')) {
			c = strrchr(rootname, '.');
		}
		extname[0] = '\0';		/* In case we have no ext.  */
		if (c) {
			strlcpy(extname, c, sizeof (extname));
			*c = 0;			/* Cut off complete ext.    */
		} else {
			/*
			 * Could not find any '.'.
			 */
			c = strchr(rootname, ';');
			if (c) {
				*c = 0;		/* Cut off version number    */
			}
		}
		c = strchr(extname, ';');
		if (c) {
			*c = 0;			/* Cut off version number    */
		}
		d1 = strlen(rootname);
		if (full_iso9660_filenames || iso9660_level > 1) {
			d2 = strlen(extname);
			/*
			 * 31/37 chars minus the 3 characters we are
			 * appending below to create unique filenames.
			 */
			if ((d1 + d2) > (iso9660_namelen - 3))
				rootname[iso9660_namelen - 3 - d2] = 0;
		} else {
			if (d1 > 5)
				rootname[5] = 0;
		}
		new_reclen = strlen(rootname);
		sprintf(newname, "%s000%s%s",
				rootname,
				extname,
				((s_entry->isorec.flags[0] & ISO_DIRECTORY) ||
				omit_version_number ? "" : ";1"));

		for (d1 = 0; d1 < 36; d1++) {
			for (d2 = 0; d2 < 36; d2++) {
				for (d3 = 0; d3 < 36; d3++) {
					newname[new_reclen + 0] =
					    (d1 <= 9 ? '0' + d1 : 'A' + d1 - 10);
					newname[new_reclen + 1] =
					    (d2 <= 9 ? '0' + d2 : 'A' + d2 - 10);
					newname[new_reclen + 2] =
					    (d3 <= 9 ? '0' + d3 : 'A' + d3 - 10);
					if (debug)
						error(_("NEW name '%s'\n"), newname);

#ifdef VMS
					/* Sigh.  VAXCRTL seems to be broken here */
					{
						int	ijk = 0;

						while (newname[ijk]) {
							if (newname[ijk] == ' ')
								newname[ijk] = '0';
							ijk++;
						}
					}
#endif

					if (!find_file_hash(newname))
						goto got_valid_name;
				}
			}
		}

		/*
		 * If we fell off the bottom here, we were in real trouble.
		 */
		comerrno(EX_BAD,
			_("Unable to generate unique name for file %s\n"),
			s_entry->name);

got_valid_name:
		/*
		 * OK, now we have a good replacement name.  Now decide which
		 * one of these two beasts should get the name changed
		 */
		if (s_entry->priority < s_entry1->priority) {
			if (verbose > 0) {
				fprintf(stderr, _("Using %s for  %s%s%s (%s)\n"),
					newname,
					this_dir->whole_name, SPATH_SEPARATOR,
					s_entry->name, s_entry1->name);
			}

			s_entry->isorec.name_len[0] = strlen(newname);
			new_reclen = offsetof(struct iso_directory_record,
				name[0]) +
				strlen(newname);
			if (use_XA || use_RockRidge) {
				if (new_reclen & 1)
					new_reclen++; /* Pad to an even byte */
				new_reclen += s_entry->rr_attr_size;
			}
			if (new_reclen & 1)
				new_reclen++;	/* Pad to an even byte */
			s_entry->isorec.length[0] = new_reclen;
			strcpy(s_entry->isorec.name, newname);
#ifdef	USE_LARGEFILES
			if (s_entry->de_flags & MULTI_EXTENT) {
				struct directory_entry	*s_e;

				/*
				 * Copy over the new name to all other entries
				 */
				for (s_e = s_entry->mxroot;
				    s_e && s_e->mxroot == s_entry->mxroot;
							s_e = s_e->next) {
					s_e->isorec.length[0] = new_reclen;
					s_e->isorec.name_len[0] = s_entry->isorec.name_len[0];
					strcpy(s_e->isorec.name, newname);
				}
			}
#endif
#ifdef APPLE_HYB
			/*
			 * Has resource fork - needs new name
			 */
			if (apple_both && s_entry->assoc) {
				struct directory_entry *s_entry2 =
								s_entry->assoc;

				/*
				 * resource fork name *should* be the same as
				 * the data fork
				 */
				s_entry2->isorec.name_len[0] =
						s_entry->isorec.name_len[0];
				strcpy(s_entry2->isorec.name,
						s_entry->isorec.name);
				s_entry2->isorec.length[0] = new_reclen;
			}
#endif	/* APPLE_HYB */
		} else {
			delete_file_hash(s_entry1);
			if (verbose > 0) {
				fprintf(stderr, _("Using %s for  %s%s%s (%s)\n"),
					newname,
					this_dir->whole_name, SPATH_SEPARATOR,
					s_entry1->name, s_entry->name);
			}
			s_entry1->isorec.name_len[0] = strlen(newname);
			new_reclen = offsetof(struct iso_directory_record,
					name[0]) +
					strlen(newname);
			if (use_XA || use_RockRidge) {
				if (new_reclen & 1)
					new_reclen++; /* Pad to an even byte */
				new_reclen += s_entry1->rr_attr_size;
			}
			if (new_reclen & 1)
				new_reclen++;	/* Pad to an even byte */
			s_entry1->isorec.length[0] = new_reclen;
			strcpy(s_entry1->isorec.name, newname);
#ifdef	USE_LARGEFILES
			if (s_entry1->de_flags & MULTI_EXTENT) {
				struct directory_entry	*s_e;

				/*
				 * Copy over the new name to all other entries
				 */
				for (s_e = s_entry1->mxroot;
				    s_e && s_e->mxroot == s_entry1->mxroot;
							s_e = s_e->next) {
					s_e->isorec.length[0] = new_reclen;
					s_e->isorec.name_len[0] = s_entry1->isorec.name_len[0];
					strcpy(s_e->isorec.name, newname);
				}
			}
#endif
			add_file_hash(s_entry1);
#ifdef APPLE_HYB
			/*
			 * Has resource fork - needs new name
			 */
			if (apple_both && s_entry1->assoc) {
				struct directory_entry *s_entry2 =
							s_entry1->assoc;

				/*
				 * resource fork name *should* be the same as
				 * the data fork
				 */
				s_entry2->isorec.name_len[0] =
						s_entry1->isorec.name_len[0];
				strcpy(s_entry2->isorec.name,
							s_entry1->isorec.name);
				s_entry2->isorec.length[0] = new_reclen;
			}
#endif	/* APPLE_HYB */
		}
		add_file_hash(s_entry);
		s_entry = s_entry->next;
	}

	if (generate_tables &&
	    !find_file_hash(trans_tbl) &&
	    (reloc_dir != this_dir) &&
	    (this_dir->extent == 0)) {
		/*
		 * First we need to figure out how big this table is
		 */
		for (s_entry = this_dir->contents; s_entry;
						s_entry = s_entry->next) {
			if (strcmp(s_entry->name, ".") == 0 ||
				strcmp(s_entry->name, "..") == 0)
				continue;
#ifdef APPLE_HYB
			/*
			 * Skip table entry for the resource fork
			 */
			if (apple_both &&
			    (s_entry->isorec.flags[0] & ISO_ASSOCIATED))
				continue;
#endif	/* APPLE_HYB */
			if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY)
				continue;
			if (s_entry->table) {
				/*
				 * Max namelen, a space before and a space
				 * after the iso filename.
				 */
				tablesize += MAX_ISONAME + 2 +
						strlen(s_entry->table);
			}
		}
	}
	if (tablesize > 0) {
		table = (struct directory_entry *)
			e_malloc(sizeof (struct directory_entry));
		memset(table, 0, sizeof (struct directory_entry));
		table->table = NULL;
		table->next = this_dir->contents;
		this_dir->contents = table;

		table->filedir = root;
		table->isorec.flags[0] = ISO_FILE;
		table->priority = 32768;
		iso9660_date(table->isorec.date, fstatbuf.st_mtime);
		table->inode = TABLE_INODE;
		table->dev = UNCACHED_DEVICE;
		set_723(table->isorec.volume_sequence_number,
						volume_sequence_number);
		set_733((char *)table->isorec.size, tablesize);
		table->size = tablesize;
		table->filedir = this_dir;
		if (jhide_trans_tbl)
			table->de_flags |= INHIBIT_JOLIET_ENTRY;
		/*
		 * Always hide transtable from UDF tree.
		 */
		table->de_flags |= INHIBIT_UDF_ENTRY;
/*		table->name = e_strdup("<translation table>");*/
		table->name = e_strdup(trans_tbl);
		/*
		 * We use sprintf() to create the strings, for this reason
		 * we need to add one byte for the null character at the
		 * end of the string even though we don't use it.
		 */
		table->table = (char *)e_malloc(ISO_ROUND_UP(tablesize)+1);
		memset(table->table, 0, ISO_ROUND_UP(tablesize)+1);
		iso9660_file_length(trans_tbl, table, 0);

		if (use_XA || use_RockRidge) {
			fstatbuf.st_mode = 0444 | S_IFREG;
			fstatbuf.st_nlink = 1;
			generate_xa_rr_attributes("",
				trans_tbl, table,
				&fstatbuf, &fstatbuf, 0);
		}
	}
	/*
	 * We have now chosen the 8.3 names and we should now know the length
	 * of every entry in the directory.
	 */
	for (s_entry = this_dir->contents; s_entry; s_entry = s_entry->next) {
		/* skip if it's hidden */
		if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY) {
			continue;
		}
		new_reclen = strlen(s_entry->isorec.name);

		/*
		 * First update the path table sizes for directories.
		 */
		if (s_entry->isorec.flags[0] & ISO_DIRECTORY) {
			if (strcmp(s_entry->name, ".") != 0 &&
					strcmp(s_entry->name, "..") != 0) {
				path_table_size += new_reclen +
						offsetof(struct iso_path_table,
						name[0]);
				if (new_reclen & 1)
					path_table_size++;
			} else {
				new_reclen = 1;
				if (this_dir == root && strlen(s_entry->name)
									== 1) {
					path_table_size += new_reclen +
						offsetof(struct iso_path_table,
						name[0]);
				}
			}
		}
		if (path_table_size & 1)
			path_table_size++;	/* For odd lengths we pad */
		s_entry->isorec.name_len[0] = new_reclen;

		new_reclen += offsetof(struct iso_directory_record, name[0]);

		if (new_reclen & 1)
			new_reclen++;

		new_reclen += s_entry->rr_attr_size;

		if (new_reclen & 1)
			new_reclen++;

		if (new_reclen > 0xff) {
			comerrno(EX_BAD,
				_("Fatal error - RR overflow (reclen %d) for file %s\n"),
				new_reclen,
				s_entry->name);
		}
		s_entry->isorec.length[0] = new_reclen;
	}

	status = sort_directory(&this_dir->contents, (reloc_dir == this_dir));
	if (status > 0) {
		errmsgno(EX_BAD, _("Unable to sort directory %s\n"),
			this_dir->whole_name);
		errmsgno(EX_BAD,
		_("If there was more than one directory type argument to mkisofs\n"));
		comerrno(EX_BAD,
		_("use -graft-points to create different target directory names.\n"));
	}
	/*
	 * If we are filling out a TRANS.TBL, generate the entries that will
	 * go in the thing.
	 */
	if (table) {
		count = 0;
		for (s_entry = this_dir->contents; s_entry;
						s_entry = s_entry->next) {
			if (s_entry == table)
				continue;
			if (!s_entry->table)
				continue;
			if (strcmp(s_entry->name, ".") == 0 ||
				strcmp(s_entry->name, "..") == 0)
				continue;
#ifdef APPLE_HYB
			/*
			 * Skip table entry for the resource fork
			 */
			if (apple_both &&
			    (s_entry->isorec.flags[0] & ISO_ASSOCIATED))
				continue;
#endif	/* APPLE_HYB */
			if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY)
				continue;
			/*
			 * Warning: we cannot use the return value of sprintf
			 * because old BSD based sprintf() implementations
			 * will return a pointer to the result instead of a
			 * count.
			 * Old mkiofs introduced a space after the iso
			 * filename to make parsing TRANS.TBL easier.
			 */
			sprintf(table->table + count, "%c %-*s%s",
				s_entry->table[0],
				MAX_ISONAME + 1,
				s_entry->isorec.name, s_entry->table + 1);
			count += strlen(table->table + count);
			free(s_entry->table);
			/*
			 * for a memory file, set s_entry->table to the
			 * correct data - which is stored in
			 * s_entry->whole_name
			 */
			if (s_entry->de_flags & MEMORY_FILE) {
				s_entry->table = s_entry->whole_name;
				s_entry->whole_name = NULL;
			} else {
				s_entry->table = NULL;
			}
		}

		if (count != tablesize) {
			comerrno(EX_BAD,
				_("Translation table size mismatch %d %d\n"),
				count, tablesize);
		}
	}
	/*
	 * Now go through the directory and figure out how large this one will
	 * be. Do not split a directory entry across a sector boundary
	 */
	s_entry = this_dir->contents;
	this_dir->ce_bytes = 0;
	while (s_entry) {
		/* skip if it's hidden */
		if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY) {
			s_entry = s_entry->next;
			continue;
		}
		new_reclen = s_entry->isorec.length[0];
		if ((this_dir->size & (SECTOR_SIZE - 1)) + new_reclen
								>= SECTOR_SIZE)

			this_dir->size = (this_dir->size + (SECTOR_SIZE - 1)) &
				~(SECTOR_SIZE - 1);
		this_dir->size += new_reclen;

		/*
		 * See if continuation entries were used on disc
		 */
		if (use_RockRidge &&
			s_entry->rr_attr_size != s_entry->total_rr_attr_size) {
			unsigned char	*pnt;
			int		len;
			int		nbytes;

			pnt = s_entry->rr_attributes;
			len = s_entry->total_rr_attr_size;
			pnt = parse_xa(pnt, &len, 0);
/*			pnt = parse_xa(pnt, &len, s_entry);*/

			/*
			 * We make sure that each continuation entry record is
			 * not split across sectors, but each file could in
			 * theory have more than one CE, so we scan through
			 * and figure out what we need.
			 */
			while (len > 3) {
				if (pnt[0] == 'C' && pnt[1] == 'E') {
					nbytes = get_733((char *)pnt + 20);

					if ((this_dir->ce_bytes & (SECTOR_SIZE - 1)) + nbytes >=
						SECTOR_SIZE)
						this_dir->ce_bytes =
							ISO_ROUND_UP(this_dir->ce_bytes);
					/*
					 * Now store the block in the
					 * ce buffer
					 */
					this_dir->ce_bytes += nbytes;
					if (this_dir->ce_bytes & 1)
						this_dir->ce_bytes++;
				}
				len -= pnt[2];
				pnt += pnt[2];
			}
		}
		s_entry = s_entry->next;
	}
	return (status);
}

LOCAL void
generate_reloc_directory()
{
	time_t		current_time;
	struct directory_entry *s_entry;

	/*
	 * Create an  entry for our internal tree
	 */
	time(&current_time);
	reloc_dir = (struct directory *)
		e_malloc(sizeof (struct directory));
	memset(reloc_dir, 0, sizeof (struct directory));
	reloc_dir->parent = root;
	reloc_dir->next = root->subdir;
	root->subdir = reloc_dir;
	reloc_dir->depth = 1;
	if (hide_rr_moved) {
		reloc_dir->whole_name = e_strdup("./.rr_moved");
		reloc_dir->de_name = e_strdup(".rr_moved");
	} else {
		reloc_dir->whole_name = e_strdup("./rr_moved");
		reloc_dir->de_name = e_strdup("rr_moved");
	}
	reloc_dir->de_path = reloc_dir->de_name;
	reloc_dir->extent = 0;


	/*
	 * Now create an actual directory entry
	 */
	s_entry = (struct directory_entry *)
		e_malloc(sizeof (struct directory_entry));
	memset(s_entry, 0, sizeof (struct directory_entry));
	s_entry->next = root->contents;
	reloc_dir->self = s_entry;

	/*
	 * The rr_moved entry will not appear in the Joliet nor the UDF tree.
	 */
	reloc_dir->dir_flags |= INHIBIT_JOLIET_ENTRY;
	s_entry->de_flags |= INHIBIT_JOLIET_ENTRY;

	reloc_dir->dir_flags |= INHIBIT_UDF_ENTRY;
	s_entry->de_flags |= INHIBIT_UDF_ENTRY;

	root->contents = s_entry;
	root->contents->name = e_strdup(reloc_dir->de_name);
	root->contents->filedir = root;
	root->contents->isorec.flags[0] = ISO_DIRECTORY;
	root->contents->priority = 32768;
	iso9660_date(root->contents->isorec.date, current_time);
	root->contents->inode = UNCACHED_INODE;
	root->contents->dev = UNCACHED_DEVICE;
	set_723(root->contents->isorec.volume_sequence_number,
						volume_sequence_number);
	iso9660_file_length(reloc_dir->de_name, root->contents, 1);

	init_fstatbuf();

	if (use_XA || use_RockRidge) {
		fstatbuf.st_mode = 0555 | S_IFDIR;
		fstatbuf.st_nlink = 2;
		generate_xa_rr_attributes("",
			hide_rr_moved ? ".rr_moved" : "rr_moved",
			s_entry, &fstatbuf, &fstatbuf, NEED_RE);
	};

	/*
	 * Now create the . and .. entries in rr_moved
	 * Now create an actual directory entry
	 */
	if (root_statbuf.st_nlink == 0)
		root_statbuf = fstatbuf;
	attach_dot_entries(reloc_dir, NULL, &root_statbuf);
}

/*
 * Function:		attach_dot_entries
 *
 * Purpose:		Create . and .. entries for a new directory.
 *
 * Notes:		Only used for artificial directories that
 *			we are creating.
 */
EXPORT void
attach_dot_entries(dirnode, this_stat, parent_stat)
	struct directory	*dirnode;
	struct stat		*this_stat;
	struct stat		*parent_stat;
{
	struct directory_entry *s_entry;
	struct directory_entry *orig_contents;
	int		deep_flag = 0;

	init_fstatbuf();
	fstatbuf.st_mode = new_dir_mode | S_IFDIR;
	fstatbuf.st_nlink = 2;
	if (parent_stat == NULL)
		parent_stat = &fstatbuf;
	if (this_stat == NULL)
		this_stat = &fstatbuf;

	orig_contents = dirnode->contents;

	if ((dirnode->dir_flags & DIR_HAS_DOTDOT) == 0) {
		if (dirnode->parent &&
		    dirnode->parent == reloc_dir) {
			deep_flag = NEED_PL;
		}
		s_entry = (struct directory_entry *)
			e_malloc(sizeof (struct directory_entry));
		memcpy(s_entry, dirnode->self,
			sizeof (struct directory_entry));
#ifdef	APPLE_HYB
		if (dirnode->self->hfs_ent) {
			s_entry->hfs_ent = (hfsdirent *)
				e_malloc(sizeof (hfsdirent));
			memcpy(s_entry->hfs_ent, dirnode->self->hfs_ent,
				sizeof (hfsdirent));
		}
#endif
		s_entry->name = e_strdup("..");
		s_entry->whole_name = NULL;
		s_entry->isorec.name_len[0] = 1;
		s_entry->isorec.flags[0] = ISO_DIRECTORY;
		iso9660_file_length("..", s_entry, 1);
		iso9660_date(s_entry->isorec.date, parent_stat->st_mtime);
		set_723(s_entry->isorec.volume_sequence_number,
						volume_sequence_number);
		set_733(s_entry->isorec.size, SECTOR_SIZE);
		memset(s_entry->isorec.extent, 0, 8);
		s_entry->filedir = dirnode->parent;

		dirnode->contents = s_entry;
		dirnode->contents->next = orig_contents;
		orig_contents = s_entry;

		if (use_XA || use_RockRidge) {
			generate_xa_rr_attributes("",
				"..", s_entry,
				parent_stat,
				parent_stat, deep_flag);
		}
		dirnode->dir_flags |= DIR_HAS_DOTDOT;
	}
	deep_flag = 0;
	if ((dirnode->dir_flags & DIR_HAS_DOT) == 0) {
		s_entry = (struct directory_entry *)
			e_malloc(sizeof (struct directory_entry));
		memcpy(s_entry, dirnode->self,
			sizeof (struct directory_entry));
#ifdef	APPLE_HYB
		if (dirnode->self->hfs_ent) {
			s_entry->hfs_ent = (hfsdirent *)
				e_malloc(sizeof (hfsdirent));
			memcpy(s_entry->hfs_ent, dirnode->self->hfs_ent,
				sizeof (hfsdirent));
		}
#endif
		s_entry->name = e_strdup(".");
		s_entry->whole_name = NULL;
		s_entry->isorec.name_len[0] = 1;
		s_entry->isorec.flags[0] = ISO_DIRECTORY;
		iso9660_file_length(".", s_entry, 1);
		iso9660_date(s_entry->isorec.date, this_stat->st_mtime);
		set_723(s_entry->isorec.volume_sequence_number,
						volume_sequence_number);
		set_733(s_entry->isorec.size, SECTOR_SIZE);
		memset(s_entry->isorec.extent, 0, 8);
		s_entry->filedir = dirnode;

		dirnode->contents = s_entry;
		dirnode->contents->next = orig_contents;

		if (use_XA || use_RockRidge) {
			if (dirnode == root) {
				deep_flag |= NEED_CE | NEED_SP;	/* For extension record */
			}
			generate_xa_rr_attributes("",
				".", s_entry,
				this_stat, this_stat, deep_flag);
		}
		dirnode->dir_flags |= DIR_HAS_DOT;
	}
}

EXPORT char *
find_rr_attribute(pnt, len, attr_type)
	unsigned char	*pnt;
	int		len;
	char		*attr_type;
{
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
			return (NULL);
		}
		if (strncmp((char *)pnt, attr_type, 2) == 0)
			return ((char *)pnt);
		else if (strncmp((char *)pnt, "ST", 2) == 0)
			return (NULL);
		len -= pnt[2];
		pnt += pnt[2];
	}
	return (NULL);
}

EXPORT void
finish_cl_pl_entries()
{
	struct directory_entry	*s_entry;
	struct directory_entry	*s_entry1;
	struct directory	*d_entry;

	/*
	 * If the reloc_dir is hidden (empty), then return
	 */
	if (reloc_dir->dir_flags & INHIBIT_ISO9660_ENTRY)
		return;

	s_entry = reloc_dir->contents;
	s_entry = s_entry->next->next;	/* Skip past . and .. */
	for (; s_entry; s_entry = s_entry->next) {
		/* skip if it's hidden */
		if (s_entry->de_flags & INHIBIT_ISO9660_ENTRY) {
			continue;
		}
		d_entry = reloc_dir->subdir;
		while (d_entry) {
			if (d_entry->self == s_entry)
				break;
			d_entry = d_entry->next;
		};
		if (!d_entry) {
			comerrno(EX_BAD,
					_("Unable to locate directory parent\n"));
		};

		if (s_entry->filedir != NULL && s_entry->parent_rec != NULL) {
			char	*rr_attr;

			/*
			 * First fix the PL pointer in the directory in the
			 * rr_reloc dir
			 */
			s_entry1 = d_entry->contents->next;

/*			set_733((char *) s_entry1->rr_attributes +*/
/*				s_entry1->total_rr_attr_size - 8,*/
/*				s_entry->filedir->extent); */
			/*
			 * The line above won't work when entry was read from
			 * the previous session, because if total_rr_attr_size
			 * was odd when recording previous session, now we have
			 * total_rr_attr_size off by 1 due to padding.
			 *
			 * So, just search for the attributes by name
			 */
			rr_attr = find_rr_attribute(s_entry1->rr_attributes,
				s_entry1->total_rr_attr_size, "PL");
			if (rr_attr != NULL)
				set_733(rr_attr + 4, s_entry->filedir->extent);


			/*
			 * Now fix the CL pointer
			 */
			s_entry1 = s_entry->parent_rec;

/*			set_733((char *) s_entry1->rr_attributes +*/
/*			s_entry1->total_rr_attr_size - 8, d_entry->extent); */
			rr_attr = find_rr_attribute(s_entry1->rr_attributes,
				s_entry1->total_rr_attr_size, "CL");
			if (rr_attr != NULL)
				set_733(rr_attr + 4, d_entry->extent);
		}
		s_entry->filedir = reloc_dir;	/* Now we can fix this */
	}
	/*
	 * We would need to modify the NLINK terms in the assorted root
	 * directory records to account for the presence of the RR_MOVED
	 * directory. We do not do this here because we do it later when
	 * we correct the link count for all direstories in the tree.
	 */


	finish_cl_pl_for_prev_session();
}

LOCAL void
dir_nesting_warn(this_dir, path, contflag)
	struct directory	*this_dir;
	char			*path;
	int			contflag;
{
	static	BOOL	did_hint = FALSE;

	errmsgno(EX_BAD,
		_("Directories too deep for '%s' (%d) max is %d%s.\n"),
		path, this_dir->depth, RR_relocation_depth,
		contflag?_("; ignored - continuing"):"");
	if (!did_hint) {
		did_hint = TRUE;
		errmsgno(EX_BAD, _("To include the complete directory tree,\n"));
		errmsgno(EX_BAD, _("use Rock Ridge extensions via -R or -r,\n"));
		errmsgno(EX_BAD, _("or allow deep ISO9660 directory nesting via -D.\n"));
	}
}

/*
 * Function:		scan_directory_tree
 *
 * Purpose:		Walk through a directory on the local machine
 *			filter those things we don't want to include
 *			and build our representation of a dir.
 *
 * Notes:
 */
EXPORT int
scan_directory_tree(this_dir, path, de)
	struct directory	*this_dir;
	char			*path;
	struct directory_entry	*de;
{
	DIR		*current_dir;
	char		whole_path[2*PATH_MAX];	/* Avoid stat buffer overflow */
	struct dirent	*d_entry;
	int		dflag;
extern	BOOL		nodesc;

	if (nodesc)
		return (1);

	if (verbose > 1) {
		fprintf(stderr, _("Scanning %s\n"), path);
	}
/*#define	check_needed*/
#ifdef	check_needed
	/*
	 * Trying to use this to avoid directory loops from hard links
	 * or followed symlinks does not work. It would prevent us from
	 * implementing merge directories.
	 */
	if (this_dir->dir_flags & DIR_WAS_SCANNED) {
		fprintf(stderr, _("Already scanned directory %s\n"), path);
		return (1);	/* It's a directory */
	}
#endif
	this_dir->dir_flags |= DIR_WAS_SCANNED;

	errno = 0;	/* Paranoia */
	current_dir = opendir(path);
	d_entry = NULL;

	/*
	 * Apparently NFS sometimes allows you to open the directory, but then
	 * refuses to allow you to read the contents.  Allow for this
	 */
	if (current_dir) {
		errno = 0;
		d_entry = readdir(current_dir);
	}

	if (!current_dir || (!d_entry && errno != 0)) {
		int	ret = 1;

		errmsg(_("Unable to open directory %s\n"), path);

		if (errno == ENOTDIR) {
			/*
			 * Mark as not a directory
			 */
			de->isorec.flags[0] &= ~ISO_DIRECTORY;
			ret = 0;
		}
		if (current_dir)
			closedir(current_dir);
		return (ret);
	}

	/*
	 * Set up the struct for the current directory, and insert it into
	 * the tree
	 */
#ifdef VMS
	vms_path_fixup(path);
#endif

	/*
	 * if entry for this sub-directory is hidden, then hide this directory
	 */
	if (de->de_flags & INHIBIT_ISO9660_ENTRY)
		this_dir->dir_flags |= INHIBIT_ISO9660_ENTRY;

	if (de->de_flags & INHIBIT_JOLIET_ENTRY)
		this_dir->dir_flags |= INHIBIT_JOLIET_ENTRY;

#ifdef SORTING
	/*
	 * set any sort weighting from it's own directory entry - if a
	 * directory is given a weighting, then all the contents will use
	 * this as the default weighting
	 */
	this_dir->sort = de->sort;
#endif /* SORTING */

	/*
	 * Now we scan the directory itself, and look at what is inside of it.
	 */
	whole_path[0] = '\0';
	dflag = -2;
	while (1 == 1) {
		char	*d_name;

		if (dflag < 0) {
			/*
			 * Some filesystems do not deliver "." and ".." at all,
			 * others (on Linux) deliver them in the wrong order.
			 * Make sure we add "." and ".." before all other
			 * entries.
			 */
			if (dflag < -1)
				d_name = ".";
			else
				d_name = "..";
			dflag++;
		} else {
			/*
			 * The first time through, skip this, since we already
			 * asked for the first entry when we opened the
			 * directory.
			 */
			if (dflag > 0) {
				d_entry = readdir(current_dir);
			} else {
				dflag++;
			}
			if (!d_entry)
				break;
			d_name = d_entry->d_name;
		}

		/*
		 * OK, got a valid entry
		 *
		 * If we do not want all files, then pitch the backups.
		 */
		if (!all_files) {
			if (strchr(d_name, '~') ||
			    strchr(d_name, '#') ||
			    rstr(d_name, ".bak")) {
				if (verbose > 0) {
					fprintf(stderr,
						_("Ignoring file %s\n"),
						d_name);
				}
				continue;
			}
		}
#ifdef APPLE_HYB
		if (apple_both) {
			/*
			 * exclude certain HFS type files/directories for the
			 * time being
			 */
			if (hfs_exclude(d_name))
				continue;
		}
#endif	/* APPLE_HYB */

		if (strlen(path) + strlen(d_name) + 2 > sizeof (whole_path)) {
			errmsgno(EX_BAD, _("Path name %s/%s too long.\n"),
					path, d_name);
			comerrno(EX_BAD, _("Overflow of stat buffer\n"));
		};

		/*
		 * Generate the complete ASCII path for this file
		 */
		strlcpy(whole_path, path, sizeof (whole_path));
#ifndef VMS
		if (whole_path[strlen(whole_path) - 1] != '/')
			strcat(whole_path, "/");
#endif
		strcat(whole_path, d_name);

		/*
		 * Should we exclude this file ?
		 * Do no check "." and ".."
		 */
		if (!(d_name[0] == '.' && (d_name[1] == '\0' ||
		    (d_name[1] == '.' && d_name[2] == '\0'))) &&
		    (matches(d_name) || matches(whole_path))) {
			if (verbose > 1) {
				fprintf(stderr,
					_("Excluded by match: %s\n"), whole_path);
			}
			continue;
		}
		if (generate_tables &&
		    strcmp(d_name, trans_tbl) == 0) {
			/*
			 * Ignore this entry.  We are going to be generating
			 * new versions of these files, and we need to ignore
			 * any originals that we might have found.
			 */
			if (verbose > 1) {
				fprintf(stderr, _("Excluded: %s\n"), whole_path);
			}
			continue;
		}
		/*
		 * If we already have a '.' or a '..' entry, then don't insert
		 * new ones.
		 */
		if (strcmp(d_name, ".") == 0 &&
		    this_dir->dir_flags & DIR_HAS_DOT) {
			continue;
		}
		if (strcmp(d_name, "..") == 0 &&
		    this_dir->dir_flags & DIR_HAS_DOTDOT) {
			continue;
		}
#if 0
		if (verbose > 1)
			fprintf(stderr, "%s\n", whole_path);
#endif
		/*
		 * This actually adds the entry to the directory in question.
		 */
		insert_file_entry(this_dir, whole_path, d_name, NULL, 0);
	}
	closedir(current_dir);

#ifdef APPLE_HYB
	/*
	 * if we cached the HFS info stuff for this directory, then delete it
	 */
	if (this_dir->hfs_info) {
		del_hfs_info(this_dir->hfs_info);
		this_dir->hfs_info = 0;
	}
#endif	/* APPLE_HYB */

	return (1);
}

LOCAL struct directory_entry *
dup_relocated_dir(this_dir, s_entry, whole_path, short_name, statp)
	struct directory	*this_dir;
	struct directory_entry	*s_entry;
	char			*whole_path;
	char			*short_name;
	struct stat		*statp;
{
	struct directory_entry *s_entry1;

	if (!reloc_dir)
		generate_reloc_directory();
	init_fstatbuf();

	/*
	 * Replicate the entry for this directory.  The old one will
	 * stay where it is, and it will be neutered so that it no
	 * longer looks like a directory. The new one will look like
	 * a directory, and it will be put in the reloc_dir.
	 */
	s_entry1 = (struct directory_entry *)
		e_malloc(sizeof (struct directory_entry));
	memcpy(s_entry1, s_entry, sizeof (struct directory_entry));
	s_entry1->table = NULL;
	s_entry1->name = e_strdup(short_name);
	s_entry1->whole_name = e_strdup(whole_path);
	s_entry1->next = reloc_dir->contents;
	reloc_dir->contents = s_entry1;
	s_entry1->priority = 32768;
	s_entry1->parent_rec = this_dir->contents;
	set_723(s_entry1->isorec.volume_sequence_number,
						volume_sequence_number);
	s_entry1->filedir = this_dir;
	iso9660_date(s_entry1->isorec.date, fstatbuf.st_mtime);

	if (use_XA || use_RockRidge) {
		generate_xa_rr_attributes(whole_path,
			short_name, s_entry1,
			statp, statp, NEED_RE);
	}

	statp->st_size = (off_t)0;
	statp->st_mode &= 0777;
	set_733((char *)s_entry->isorec.size, 0);
	s_entry->size = 0;
	s_entry->isorec.flags[0] = ISO_FILE;
	s_entry->inode = UNCACHED_INODE;
	s_entry->dev = UNCACHED_DEVICE;
	s_entry->de_flags |= RELOCATED_DIRECTORY;

	return (s_entry1);
}


/*
 * Function:		insert_file_entry
 *
 * Purpose:		Insert one entry into our directory node.
 *
 * Note:
 * This function inserts a single entry into the directory.  It
 * is assumed that all filtering and decision making regarding what
 * we want to include has already been made, so the purpose of this
 * is to insert one entry (file, link, dir, etc), into this directory.
 * Note that if the entry is a dir (or if we are following links,
 * and the thing it points to is a dir), then we will scan those
 * trees before we return.
 */
EXPORT int
insert_file_entry(this_dir, whole_path, short_name, statp, have_rsrc)
	struct directory	*this_dir;
	char			*whole_path;
	char			*short_name;
	struct stat		*statp;
	int			have_rsrc;
{
	struct stat	statbuf,
			lstatbuf;
	struct directory_entry *s_entry,
			*s_entry1;
	int		lstatus;
	int		status;
	int		deep_flag;
#ifdef	USE_NO_SCANDIR
	int		no_scandir = 0;
#endif

#ifdef APPLE_HYB
	int		x_hfs = 0;
	int		htype = TYPE_NONE;

#endif	/* APPLE_HYB */

	if (statp) {
		status = lstatus = 0;
		lstatbuf = *statp;
		if (S_ISLNK(lstatbuf.st_mode)) {
			status = stat_filter(short_name, &statbuf);
		} else {
			statbuf = *statp;
		}
	} else {
		status = stat_filter(whole_path, &statbuf);

		lstatus = lstat_filter(whole_path, &lstatbuf);
	}

	if ((status == -1) && (lstatus == -1)) {
		/*
		 * This means that the file doesn't exist, or isn't accessible.
		 * Sometimes this is because of NFS permissions problems.
		 */
		errmsg(_("Non-existent or inaccessible: %s\n"), whole_path);
		return (0);
	}
	if (S_ISDIR(statbuf.st_mode) &&
	    (this_dir->depth > RR_relocation_depth) && !use_RockRidge &&
	    strcmp(short_name, ".") != 0 && strcmp(short_name, "..") != 0) {
		dir_nesting_warn(this_dir, whole_path, TRUE);
		return (0);
	}
	if (this_dir == root && strcmp(short_name, ".") == 0)
		root_statbuf = statbuf;	/* Save this for later on */

	/*
	 * We do this to make sure that the root entries are consistent
	 */
	if (this_dir == root && strcmp(short_name, "..") == 0) {
		statbuf = root_statbuf;
		lstatbuf = root_statbuf;
	}
	if (S_ISLNK(lstatbuf.st_mode)) {
		/*
		 * Here we decide how to handle the symbolic links.  Here we
		 * handle the general case - if we are not following links or
		 * there is an error, then we must change something.  If RR
		 * is in use, it is easy, we let RR describe the file.  If
		 * not, then we punt the file.
		 */
		if ((status || !follow_links)) {
#ifdef UDF
			if (use_RockRidge || use_udf) {
#else
			if (use_RockRidge) {
#endif
				status = 0;
				STAT_INODE(statbuf) = UNCACHED_INODE;
				statbuf.st_dev = UNCACHED_DEVICE;
#ifdef UDF
				if (create_udfsymlinks) {
					char symlinkcontents[2048];
					off_t size = sizeof (symlinkcontents);

					if (udf_get_symlinkcontents(whole_path,
					    symlinkcontents, &size) == -1) {
						statbuf.st_size = (off_t)0;
						statbuf.st_mode =
							(statbuf.st_mode & ~S_IFMT) | S_IFREG;
					} else {
						statbuf.st_size = size;
						statbuf.st_mode = lstatbuf.st_mode;
					}
				} else {
#endif
					statbuf.st_size = (off_t)0;
					statbuf.st_mode =
						(statbuf.st_mode & ~S_IFMT) | S_IFREG;
#ifdef UDF
				}
#endif
			} else {
				if (follow_links) {
					/* XXX errno may be wrong! */
					errmsg(_("Unable to stat file %s - ignoring and continuing.\n"),
						whole_path);
				} else {
					errmsgno(EX_BAD,
						_("Symlink %s ignored - continuing.\n"),
						whole_path);
					return (0); /* Non Rock Ridge discs */
						    /* - ignore all symlinks */
				}
			}
		}
		/*
		 * Here we handle a different kind of case.  Here we have a
		 * symlink, but we want to follow symlinks.  If we run across
		 * a directory loop, then we need to pretend that we are not
		 * following symlinks for this file.  If this is the first
		 * time we have seen this, then make this seem as if there was
		 * no symlink there in the first place
		 */
		if (follow_links &&
		    S_ISDIR(statbuf.st_mode)) {
			if (strcmp(short_name, ".") != 0 &&
			    strcmp(short_name, "..") != 0) {
				if (find_directory_hash(statbuf.st_dev,
							STAT_INODE(statbuf))) {
					if (!use_RockRidge) {
						fprintf(stderr,
						_("Already cached directory seen (%s)\n"),
							whole_path);
						return (0);
					}
					lstatbuf = statbuf;
					/*
					 * XXX when this line was active,
					 * XXX mkisofs did not include all
					 * XXX files if it was called with '-f'
					 * XXX (follow symlinks).
					 * XXX Now scan_directory_tree()
					 * XXX checks if the directory has
					 * XXX already been scanned via the
					 * XXX DIR_WAS_SCANNED flag.
					 */
/*					no_scandir = 1;*/
				} else {
					lstatbuf = statbuf;
					add_directory_hash(statbuf.st_dev,
							STAT_INODE(statbuf));
				}
			}
		}
		/*
		 * For non-directories, we just copy the stat information over
		 * so we correctly include this file.
		 */
		if (follow_links &&
		    !S_ISDIR(statbuf.st_mode)) {
			lstatbuf = statbuf;
		}
	}
	/*
	 * Add directories to the cache so that we don't waste space even if
	 * we are supposed to be following symlinks.
	 */
	if (follow_links &&
	    strcmp(short_name, ".") != 0 &&
	    strcmp(short_name, "..") != 0 &&
	    S_ISDIR(statbuf.st_mode)) {
		add_directory_hash(statbuf.st_dev, STAT_INODE(statbuf));
	}
#ifdef VMS
	if (!S_ISDIR(lstatbuf.st_mode) && (statbuf.st_fab_rfm != FAB$C_FIX &&
			statbuf.st_fab_rfm != FAB$C_STMLF)) {
		fprintf(stderr,
		_("Warning - file %s has an unsupported VMS record format (%d)\n"),
			whole_path, statbuf.st_fab_rfm);
	}
#endif

	if (S_ISREG(lstatbuf.st_mode) &&
	    ((statp != NULL && (status = access(short_name, R_OK))) ||
	    (statp == NULL && (status = access(whole_path, R_OK))))) {
		errmsg(_("File %s is not readable - ignoring\n"),
			whole_path);
		return (0);
	}
#ifdef	USE_LARGEFILES
	if (S_ISREG(lstatbuf.st_mode) && (lstatbuf.st_size >= maxnonlarge) &&
	    !do_largefiles) {
#else
	/*
	 * >= is required by the large file summit standard.
	 */
	if (S_ISREG(lstatbuf.st_mode) && (lstatbuf.st_size >= (off_t)0x7FFFFFFF)) {
#endif
#ifdef	EOVERFLOW
		errno = EOVERFLOW;
#else
		errno = EFBIG;
#endif
		errmsg(_("File %s is too large for current mkisofs settings (-iso-level 3 or more required) - ignoring\n"),
			whole_path);
		return (0);
	}
	/*
	 * Add this so that we can detect directory loops with hard links.
	 * If we are set up to follow symlinks, then we skip this checking.
	 */
	if (!follow_links &&
	    S_ISDIR(lstatbuf.st_mode) &&
	    strcmp(short_name, ".") != 0 &&
	    strcmp(short_name, "..") != 0) {
		if (find_directory_hash(statbuf.st_dev, STAT_INODE(statbuf))) {
/*			comerrno(EX_BAD,*/
/*			_("Directory loop - fatal goof (%s %lx %lu).\n"),*/
			errmsgno(EX_BAD,
			_("Warning: Directory loop (%s dev: %lx ino: %lu).\n"),
				whole_path, (unsigned long) statbuf.st_dev,
				(unsigned long) STAT_INODE(statbuf));
		}
		add_directory_hash(statbuf.st_dev, STAT_INODE(statbuf));
	}
	if (!S_ISCHR(lstatbuf.st_mode) && !S_ISBLK(lstatbuf.st_mode) &&
		!S_ISFIFO(lstatbuf.st_mode) && !S_ISSOCK(lstatbuf.st_mode) &&
		!S_ISLNK(lstatbuf.st_mode) && !S_ISREG(lstatbuf.st_mode) &&
		!S_ISDIR(lstatbuf.st_mode)) {
		fprintf(stderr,
		_("Unknown file type (%s) %s - ignoring and continuing.\n"),
			filetype((int)lstatbuf.st_mode), whole_path);
		return (0);
	}
	/*
	 * Who knows what trash this is - ignore and continue
	 */
	if (status) {
		errmsg(_("Unable to stat file %s - ignoring and continuing.\n"),
			whole_path);
		return (0);
	}

#if !defined(__MINGW32__) && !defined(_MSC_VER)
#define	is_archive(st)	((st).st_dev == archive_dev && (st).st_ino == archive_ino)
	if (archive_isreg && is_archive(statbuf)) {
		errmsgno(EX_BAD,
			_("'%s' is the archive. Not dumped.\n"), whole_path);
		return (0);
	}
#endif

	/*
	 * Check to see if we have already seen this directory node. If so,
	 * then we don't create a new entry for it, but we do want to recurse
	 * beneath it and add any new files we do find.
	 */
	if (S_ISDIR(statbuf.st_mode)) {
		int	dflag;

		for (s_entry = this_dir->contents; s_entry;
						s_entry = s_entry->next) {
			if (strcmp(s_entry->name, short_name) == 0) {
				break;
			}
		}
		if (s_entry != NULL &&
		    strcmp(short_name, ".") != 0 &&
		    strcmp(short_name, "..") != 0) {
			struct directory *child;

			/*
			 * This should not create a new directory
			 */
			if ((s_entry->de_flags & RELOCATED_DIRECTORY) != 0) {
				for (s_entry = reloc_dir->contents; s_entry;
						s_entry = s_entry->next) {
					if (strcmp(s_entry->name, short_name)
									== 0) {
						break;
					}
				}
				child = find_or_create_directory(reloc_dir,
					whole_path,
					s_entry, 1);
			} else {
				child = find_or_create_directory(this_dir,
					whole_path,
					s_entry, 1);
				/*
				 * If unable to scan directory, mark this as a
				 * non-directory
				 */
			}
/*			if (no_scandir)*/
			if (0)
				dflag = 1;
			else
				dflag = scan_directory_tree(child,
							whole_path, s_entry);
			if (!dflag) {
				lstatbuf.st_mode =
					(lstatbuf.st_mode & ~S_IFMT) | S_IFREG;
			}
			return (0);
		}
	}
#ifdef APPLE_HYB
	/*
	 * Should we exclude this HFS file ? - only works with -hfs
	 */
	if (!have_rsrc && apple_hyb && strcmp(short_name, ".") != 0 &&
					strcmp(short_name, "..") != 0) {
		if ((x_hfs = (hfs_matches(short_name) ||
					hfs_matches(whole_path))) == 1) {
			if (verbose > 1) {
				fprintf(stderr, _("Hidden from HFS tree: %s\n"),
							whole_path);
			}
		}
	}
	/*
	 * check we are a file, using Apple extensions and have a .resource
	 * part and not excluded
	 */
	if (S_ISREG(lstatbuf.st_mode) && !have_rsrc && apple_both && !x_hfs) {
		char	rsrc_path[PATH_MAX];	/* rsrc fork filename */

		/*
		 * Construct the resource full path
		 */
		htype = get_hfs_rname(whole_path, short_name, rsrc_path);
		/*
		 * Check we can read the resouce fork
		 */
		if (htype) {
			struct stat	rstatbuf,
					rlstatbuf;

			/*
			 * Some further checks on the file
			 */
			status = stat_filter(rsrc_path, &rstatbuf);

			lstatus = lstat_filter(rsrc_path, &rlstatbuf);

/*			if (!status && !lstatus && S_ISREG(rlstatbuf.st_mode)*/
/*					&& rlstatbuf.st_size > (off_t)0) { */
			if (!status && !lstatus && S_ISREG(rstatbuf.st_mode) &&
					rstatbuf.st_size > (off_t)0) {

				/*
				 * have a resource file - insert it into the
				 * current directory but flag that we have a
				 * resource fork
				 */
				insert_file_entry(this_dir, rsrc_path,
							short_name, NULL, htype);
			}
		}
	}
#endif	/* APPLE_HYB */

	s_entry = (struct directory_entry *)
		e_malloc(sizeof (struct directory_entry));
	/* memset the whole struct, not just the isorec.extent part JCP */
	memset(s_entry, 0, sizeof (struct directory_entry));
	s_entry->next = this_dir->contents;
/*	memset(s_entry->isorec.extent, 0, 8); */
	this_dir->contents = s_entry;
	deep_flag = 0;
	s_entry->table = NULL;

	s_entry->name = e_strdup(short_name);
	s_entry->whole_name = e_strdup(whole_path);

	s_entry->de_flags = 0;
	if (S_ISLNK(lstatbuf.st_mode))
		s_entry->de_flags |= IS_SYMLINK;
#ifdef	DUPLICATES_ONCE
	s_entry->digest_fast = NULL;
	s_entry->digest_full = NULL;
#endif

	/*
	 * If the current directory is hidden, then hide all it's members
	 * otherwise check if this entry needs to be hidden as well
	 */
	if (this_dir->dir_flags & INHIBIT_ISO9660_ENTRY) {
		s_entry->de_flags |= INHIBIT_ISO9660_ENTRY;
	} else if (strcmp(short_name, ".") != 0 &&
		    strcmp(short_name, "..") != 0) {
		if (i_matches(short_name) || i_matches(whole_path)) {
			if (verbose > 1) {
				fprintf(stderr,
					_("Hidden from ISO9660 tree: %s\n"),
					whole_path);
			}
			s_entry->de_flags |= INHIBIT_ISO9660_ENTRY;
		}
		if (h_matches(short_name) || h_matches(whole_path)) {
			if (verbose > 1) {
				fprintf(stderr,
					_("Hidden ISO9660 attribute: %s\n"),
					whole_path);
			}
			s_entry->de_flags |= HIDDEN_FILE;
		}
	}
	if (this_dir != reloc_dir &&
				this_dir->dir_flags & INHIBIT_JOLIET_ENTRY) {
		s_entry->de_flags |= INHIBIT_JOLIET_ENTRY;
	} else if (strcmp(short_name, ".") != 0 &&
		    strcmp(short_name, "..") != 0) {
		if (j_matches(short_name) || j_matches(whole_path)) {
			if (verbose > 1) {
				fprintf(stderr,
					_("Hidden from Joliet tree: %s\n"),
					whole_path);
			}
			s_entry->de_flags |= INHIBIT_JOLIET_ENTRY;
		}
	}
	if (this_dir != reloc_dir &&
				this_dir->dir_flags & INHIBIT_UDF_ENTRY) {
		s_entry->de_flags |= INHIBIT_UDF_ENTRY;
	} else if (strcmp(short_name, ".") != 0 &&
		    strcmp(short_name, "..") != 0) {
		if (u_matches(short_name) || u_matches(whole_path)) {
			if (verbose > 1) {
				fprintf(stderr,
					_("Hidden from UDF tree: %s\n"),
					whole_path);
			}
			s_entry->de_flags |= INHIBIT_UDF_ENTRY;
		}
	}

#ifdef SORTING
	/*
	 * Inherit any sort weight from parent directory
	 */
	s_entry->sort = this_dir->sort;

#ifdef  DVD_AUD_VID
	/*
	 * No use at all to do a sort if we don't make a dvd video/audio
	 */
	/*
	 * Assign special weights to VIDEO_TS and AUDIO_TS files.
	 * This can't be done with sort_matches for two reasons:
	 * first, we need to match against the destination (DVD)
	 * path rather than the source path, and second, there are
	 * about 2400 different file names to check, each needing
	 * a different priority, and adding that many patterns to
	 * sort_matches would slow things to a crawl.
	 */

	if (dvd_aud_vid_flag) {
		s_entry->sort = assign_dvd_weights(s_entry->name, this_dir, s_entry->sort);
		/*
		 * Turn on sorting if necessary, regardless of cmd-line options
		 */
		if ((s_entry->sort != this_dir->sort) && do_sort == 0)
			do_sort++;
	}
#endif

	/*
	 * See if this entry should have a new weighting
	 */
	if (do_sort && strcmp(short_name, ".") != 0 &&
			strcmp(short_name, "..") != 0) {
		s_entry->sort = sort_matches(whole_path, s_entry->sort);
	}
#endif /* SORTING */

	s_entry->filedir = this_dir;
	s_entry->isorec.flags[0] = ISO_FILE;
	if (s_entry->de_flags & HIDDEN_FILE)
		s_entry->isorec.flags[0] |= ISO_EXISTENCE;
	s_entry->isorec.ext_attr_length[0] = 0;
	iso9660_date(s_entry->isorec.date, statbuf.st_mtime);
	s_entry->isorec.file_unit_size[0] = 0;
	s_entry->isorec.interleave[0] = 0;

#ifdef APPLE_HYB
	if (apple_both && !x_hfs) {
		s_entry->hfs_ent = NULL;
		s_entry->assoc = NULL;
		s_entry->hfs_off = (off_t)0;
		s_entry->hfs_type = htype;
		if (have_rsrc) {
			/* associated (rsrc) file */
			s_entry->isorec.flags[0] |= ISO_ASSOCIATED;
			/* set the type of HFS file */
			s_entry->hfs_type = have_rsrc;
			/*
			 * don't want the rsrc file to be included in any
			 * Joliet/UDF tree
			 */
			s_entry->de_flags |= INHIBIT_JOLIET_ENTRY;
			s_entry->de_flags |= INHIBIT_UDF_ENTRY;
		} else if (s_entry->next) {
			/*
			 * if previous entry is an associated file,
			 * then "link" it to this file i.e. we have a
			 * data/resource pair
			 */
			if (s_entry->next->isorec.flags[0] & ISO_ASSOCIATED) {
				s_entry->assoc = s_entry->next;
				/*
				 * share the same HFS parameters
				 */
				s_entry->hfs_ent = s_entry->next->hfs_ent;
				s_entry->hfs_type = s_entry->next->hfs_type;
			}
		}
		/*
		 * Allocate HFS entry if required
		 */
		if (apple_both && strcmp(short_name, ".") != 0 &&
					strcmp(short_name, "..") != 0) {
			if (!s_entry->hfs_ent) {
				hfsdirent	*hfs_ent;

				hfs_ent =
				(hfsdirent *) e_malloc(sizeof (hfsdirent));

				/*
				 * Sill in the defaults
				 */
				memset(hfs_ent, 0, sizeof (hfsdirent));

				s_entry->hfs_ent = hfs_ent;
			}
			/*
			 * the resource fork is processed first, but the
			 * data fork's time info is used in preference
			 * i.e. time info is set from the resource fork
			 * initially, then it is set from the data fork
			 */
			if (have_rsrc) {
				/*
				 * Set rsrc size
				 */
				s_entry->hfs_ent->u.file.rsize = lstatbuf.st_size;
				/*
				 * This will be overwritten - but might as
				 * well set it here ...
				 */
				s_entry->hfs_ent->crdate = lstatbuf.st_ctime;
				s_entry->hfs_ent->mddate = lstatbuf.st_mtime;
			} else {
				/*
				 * Set data size
				 */
				s_entry->hfs_ent->u.file.dsize = lstatbuf.st_size;
				s_entry->hfs_ent->crdate = lstatbuf.st_ctime;
				s_entry->hfs_ent->mddate = lstatbuf.st_mtime;
			}
		}
	}
#endif	/* APPLE_HYB */

	if (strcmp(short_name, ".") == 0) {
		this_dir->dir_flags |= DIR_HAS_DOT;
	}
	if (strcmp(short_name, "..") == 0) {
		this_dir->dir_flags |= DIR_HAS_DOTDOT;
	}
	if (this_dir->parent &&
	    this_dir->parent == reloc_dir &&
	    strcmp(short_name, "..") == 0) {
		s_entry->inode = UNCACHED_INODE;
		s_entry->dev = UNCACHED_DEVICE;
		deep_flag = NEED_PL;
	} else
#ifdef APPLE_HYB
	if (have_rsrc) {
		/*
		 * don't want rsrc files to be cached
		 */
		s_entry->de_flags |= RESOURCE_FORK;
		s_entry->inode = UNCACHED_INODE;
		s_entry->dev = UNCACHED_DEVICE;
	} else
#endif	/* APPLE_HYB */
	{
		s_entry->inode = STAT_INODE(statbuf);
		s_entry->dev = statbuf.st_dev;
	}
	set_723(s_entry->isorec.volume_sequence_number,
						volume_sequence_number);
	iso9660_file_length(short_name, s_entry, S_ISDIR(statbuf.st_mode));
	s_entry->rr_attr_size = 0;
	s_entry->total_rr_attr_size = 0;
	s_entry->rr_attributes = NULL;

	/*
	 * Directories are assigned sizes later on
	 */
	if (!S_ISDIR(statbuf.st_mode)) {
		if (S_ISCHR(lstatbuf.st_mode) || S_ISBLK(lstatbuf.st_mode) ||
			S_ISFIFO(lstatbuf.st_mode) ||
				S_ISSOCK(lstatbuf.st_mode) ||
#ifdef	UDF
				(S_ISLNK(lstatbuf.st_mode) && !create_udfsymlinks)) {
#else
				FALSE) {
#endif
			s_entry->size = (off_t)0;
			statbuf.st_size = (off_t)0;
		} else {
			s_entry->size = statbuf.st_size;
		}

		set_733((char *)s_entry->isorec.size, statbuf.st_size);
	} else {
		s_entry->isorec.flags[0] |= ISO_DIRECTORY;
	}
#ifdef APPLE_HYB
	/*
	 * If the directory is HFS excluded, then we don't have an hfs_ent
	 */
	if (apple_both && s_entry->hfs_ent &&
				(s_entry->isorec.flags[0] & ISO_DIRECTORY)) {
		/*
		 * Get the Mac directory name
		 */
		get_hfs_dir(whole_path, short_name, s_entry);

		/*
		 * If required, set ISO directory name from HFS name
		 */
		if (use_mac_name)
			iso9660_file_length(s_entry->hfs_ent->name, s_entry, 1);
	}
#endif	/* APPLE_HYB */

	if (strcmp(short_name, ".") != 0 && strcmp(short_name, "..") != 0 &&
		S_ISDIR(statbuf.st_mode) &&
				this_dir->depth > RR_relocation_depth) {
		struct directory *child;

		/*
		 * Replicate the entry for this directory.  The old one will
		 * stay where it is, and it will be neutered so that it no
		 * longer looks like a directory. The new one will look like
		 * a directory, and it will be put in the reloc_dir.
		 */
		s_entry1 = dup_relocated_dir(this_dir, s_entry,
					whole_path, short_name, &statbuf);

		/*
		 * We need to set this temporarily so that the parent to this
		 * is correctly determined.
		 */
		s_entry1->filedir = reloc_dir;
		child = find_or_create_directory(reloc_dir, whole_path,
			s_entry1, 0);
		free(child->de_path);		/* allocated in this case */
		set_de_path(this_dir, child);


/*		if (!no_scandir)*/
		if (!0)
			scan_directory_tree(child, whole_path, s_entry1);
		s_entry1->filedir = this_dir;
		deep_flag = NEED_CL;
	}
	if (generate_tables &&
	    strcmp(s_entry->name, ".") != 0 &&
	    strcmp(s_entry->name, "..") != 0) {

		char	buffer[SECTOR_SIZE];
		int	nchar;

		switch (lstatbuf.st_mode & S_IFMT) {
		case S_IFDIR:
			sprintf(buffer, "D\t%s\n",
				s_entry->name);
			break;

/*
 * extra for WIN32 - if it doesn't have the major/minor defined, then
 * S_IFBLK and S_IFCHR type files are unlikely to exist anyway ...
 * code similar to that in rock.c
 */
#if 0
/*
 * Use the device handling code from <schily/device.h>
 */
#ifndef major
#define	major(dev) (sizeof (dev_t) <= 2 ? ((dev) >> 8) : \
	(sizeof (dev_t) <= 4 ? (((dev) >> 8) >> 8) : \
	(((dev) >> 16) >> 16)))
#define	minor(dev) (sizeof (dev_t) <= 2 ? (dev) & 0xff : \
	(sizeof (dev_t) <= 4 ? (dev) & 0xffff : \
	(dev) & 0xffffffff))
#endif
#endif

#ifdef S_IFBLK
		case S_IFBLK:
			sprintf(buffer, "B\t%s\t%lu %lu\n",
				s_entry->name,
				(unsigned long) major(statbuf.st_rdev),
				(unsigned long) minor(statbuf.st_rdev));
			break;
#endif
#ifdef S_IFIFO
		case S_IFIFO:
			sprintf(buffer, "P\t%s\n",
				s_entry->name);
			break;
#endif
#ifdef S_IFCHR
		case S_IFCHR:
			sprintf(buffer, "C\t%s\t%lu %lu\n",
				s_entry->name,
				(unsigned long) major(statbuf.st_rdev),
				(unsigned long) minor(statbuf.st_rdev));
			break;
#endif
#ifdef S_IFLNK
		case S_IFLNK:
#ifdef	HAVE_READLINK
			nchar = readlink(statp?short_name:whole_path,
				(char *)symlink_buff,
				sizeof (symlink_buff)-1);
			if (nchar < 0) {
				errmsg(_("Cannot read link '%s'.\n"),
					statp?short_name:whole_path);
			}
#else
			nchar = -1;
#endif
			symlink_buff[nchar < 0 ? 0 : nchar] = 0;
			sprintf(buffer, "L\t%s\t%s\n",
				s_entry->name, symlink_buff);
			break;
#endif
#ifdef S_IFSOCK
		case S_IFSOCK:
			sprintf(buffer, "S\t%s\n",
				s_entry->name);
			break;
#endif
		case S_IFREG:
		default:
			sprintf(buffer, "F\t%s\n",
				s_entry->name);
			break;
		};
		s_entry->table = e_strdup(buffer);
	}
	if (S_ISDIR(statbuf.st_mode)) {
		int	dflag;

		if (strcmp(short_name, ".") != 0 &&
		    strcmp(short_name, "..") != 0) {
			struct directory *child;

			child = find_or_create_directory(this_dir, whole_path,
				s_entry, 1);
#ifdef	USE_NO_SCANDIR
			if (no_scandir)
				dflag = 1;
			else
#endif
				dflag = scan_directory_tree(child, whole_path,
								s_entry);

			if (!dflag) {
				lstatbuf.st_mode =
					(lstatbuf.st_mode & ~S_IFMT) | S_IFREG;
				if (child->contents == NULL) {
					delete_directory(this_dir, child);
				}
			}
		}
		/* If unable to scan directory, mark this as a non-directory */
	}
	if (use_RockRidge && this_dir == root &&
	    strcmp(s_entry->name, ".") == 0) {
		deep_flag |= NEED_CE | NEED_SP;	/* For extension record */
	}
	/*
	 * Now figure out how much room this file will take in the directory
	 */

#ifdef APPLE_HYB
	/*
	 * Ff the file is HFS excluded, then we don't have an hfs_ent
	 */
	if (apple_both && !have_rsrc && s_entry->hfs_ent) {
		if (S_ISREG(lstatbuf.st_mode)) { /* it's a regular file */

			/*
			 * Fill in the rest of the HFS entry
			 */
			get_hfs_info(whole_path, short_name, s_entry);

			/*
			 * If required, set ISO directory name from HFS name
			 */
			if (use_mac_name)
				iso9660_file_length(s_entry->hfs_ent->name,
								s_entry, 0);

			/*
			 * Print details about the HFS file
			 */
			if (verbose > 2)
				print_hfs_info(s_entry);

			/*
			 * copy the new ISO9660 name to the rsrc fork
			 * - if it exists
			 */
			if (s_entry->assoc)
				strcpy(s_entry->assoc->isorec.name,
							s_entry->isorec.name);

			/*
			 * we can't handle hard links in the hybrid case, so we
			 * "uncache" the file. The downside to this is that
			 * hard linked files are added to the output image
			 * more than once (we've already done this for rsrc
			 * files)
			 */
			if (apple_hyb && have_rsrc) {
				s_entry->de_flags |= RESOURCE_FORK;
				s_entry->inode = UNCACHED_INODE;
				s_entry->dev = UNCACHED_DEVICE;
			}
		} else if (!(s_entry->isorec.flags[0] & ISO_DIRECTORY)) {
			/* not a directory .. */

			/*
			 * no mac equivalent, so ignore - have to be careful
			 * here, the hfs_ent may be also be for a relocated
			 * directory
			 */
			if (s_entry->hfs_ent &&
			    !(s_entry->de_flags & RELOCATED_DIRECTORY) &&
			    (s_entry->isorec.flags[0] & ISO_MULTIEXTENT) == 0) {
				free(s_entry->hfs_ent);
			}
			s_entry->hfs_ent = NULL;
		}
		/*
		 * if the rsrc size is zero, then we don't need the entry, so
		 * we might as well delete it - this will only happen if we
		 * didn't know the rsrc size from the rsrc file size
		 */
		if (s_entry->assoc && s_entry->assoc->size == 0)
			delete_rsrc_ent(s_entry);
	}
	if (apple_ext && s_entry->assoc) {
		/*
		 * Need Apple extensions for the resource fork as well
		 */
		generate_xa_rr_attributes(whole_path,
			short_name, s_entry->assoc,
			&statbuf, &lstatbuf, deep_flag | (statp?DID_CHDIR:0));
	}
	/* leave out resource fork for the time being */
	/*
	 * XXX This is most likely wrong and should just be:
	 * XXX if (use_XA || use_RockRidge) {
	 */
/*	if ((use_XA || use_RockRidge) && !have_rsrc) {*/
	if (use_XA || use_RockRidge) {
#else
	if (use_XA || use_RockRidge) {
#endif	/* APPLE_HYB */
		generate_xa_rr_attributes(whole_path,
			short_name, s_entry,
			&statbuf, &lstatbuf, deep_flag | (statp?DID_CHDIR:0));

	}
#ifdef UDF
	/* set some info used for udf */
	s_entry->mode = lstatbuf.st_mode;
	s_entry->rdev = lstatbuf.st_rdev;
	s_entry->uid = lstatbuf.st_uid;
	s_entry->gid = lstatbuf.st_gid;
	s_entry->atime.tv_sec = lstatbuf.st_atime;
	s_entry->atime.tv_nsec = stat_ansecs(&lstatbuf);
	s_entry->mtime.tv_sec = lstatbuf.st_mtime;
	s_entry->mtime.tv_nsec = stat_mnsecs(&lstatbuf);
	s_entry->ctime.tv_sec = lstatbuf.st_ctime;
	s_entry->ctime.tv_nsec = stat_cnsecs(&lstatbuf);
#endif

#ifdef	USE_LARGEFILES
#ifndef	MAX_EXTENT
	/*
	 * Allow to #define MAX_EXTENT from outside for debug purposes.
	 */
#ifdef	PROTOTYPES
#define	LARGE_EXTENT	((off_t)0xFFFFF800UL)
#define	MAX_EXTENT	((off_t)0xFFFFFFFEUL)
#else
#define	LARGE_EXTENT	((off_t)0xFFFFF800L)
#define	MAX_EXTENT	((off_t)0xFFFFFFFEL)
#endif
#else	/* MAX_EXTENT */
#define	LARGE_EXTENT	MAX_EXTENT & ~(off_t)2047L
#endif	/* !MAX_EXTENT */
	/*
	 * Break up files greater than (4GB -2) into multiple extents.
	 * The original entry, with ->size untouched, remains for UDF.
	 * Each of the new file sections will get its own entry.
	 * The file sections are the only entries actually written out to the
	 * disk. The UDF entry will use "mxroot" to get the same start
	 * block as the first file section, and all the sections will end up
	 * in the ISO9660 directory in the correct order by "mxpart",
	 * which the directory sorting routine knows about.
	 *
	 * If we ever need to be able to find mxpart == 1 after sorting,
	 * we need to add another pointer to s_entry or to be very careful
	 * with the loops above where the ISO-9660 name is copied back to
	 * all multi-extent parts.
	 */
	if (s_entry->size > MAX_EXTENT) {
		off_t	size;

		s_entry->de_flags |= MULTI_EXTENT;
		s_entry->isorec.flags[0] |= ISO_MULTIEXTENT;
		s_entry->mxroot = s_entry;
		s_entry->mxpart = 0;
		set_733((char *)s_entry->isorec.size, LARGE_EXTENT);
		s_entry1 = dup_directory_entry(s_entry);
		s_entry->next = s_entry1;

		/*
		 * full size UDF version
		 */
		s_entry->de_flags |= INHIBIT_ISO9660_ENTRY|INHIBIT_JOLIET_ENTRY;
		if (s_entry->size > (((off_t)190)*0x3FFFF800)) {
#ifndef	EOVERFLOW
#define	EOVERFLOW	EFBIG
#endif
			errmsgno(EOVERFLOW,
			_("File %s is too large - hiding from UDF tree.\n"),
							whole_path);
			s_entry->de_flags |= INHIBIT_UDF_ENTRY;
		}

		/*
		 * Prepare the first file multi-extent section of the file.
		 */
		s_entry = s_entry1;
		s_entry->de_flags |= INHIBIT_UDF_ENTRY;
		size = s_entry->size;
		s_entry->size = LARGE_EXTENT;
		s_entry->mxpart++;

		/*
		 * Additional extents, as needed
		 */
		while (size > MAX_EXTENT) {
			s_entry1 = dup_directory_entry(s_entry);
			s_entry->next = s_entry1;

			s_entry = s_entry1;
			s_entry->mxpart++;
			size -= LARGE_EXTENT;
		}
		/*
		 * That was the last one.
		 */
		s_entry->isorec.flags[0] &= ~ISO_MULTIEXTENT;
		s_entry->size = size;
		set_733((char *)s_entry->isorec.size, (UInt32_t)s_entry->size);
	}
#endif /* USE_LARGEFILES */

	return (1);
}

EXPORT struct directory_entry *
dup_directory_entry(s_entry)
	struct directory_entry	*s_entry;
{
	struct directory_entry	*s_entry1;

	s_entry1 = (struct directory_entry *)
		e_malloc(sizeof (struct directory_entry));
	memcpy(s_entry1, s_entry, sizeof (struct directory_entry));

	if (s_entry->rr_attributes) {
		s_entry1->rr_attributes =
				e_malloc(s_entry->total_rr_attr_size);
		memcpy(s_entry1->rr_attributes, s_entry->rr_attributes,
					s_entry->total_rr_attr_size);
	}
	if (s_entry->name)
		s_entry1->name = e_strdup(s_entry->name);
	if (s_entry->whole_name)
		s_entry1->whole_name = e_strdup(s_entry->whole_name);
#ifdef	APPLE_HYB
	/*
	 * If we also duplicate s_entry->hfs_ent, we would need to change
	 * free_one_directory() and other calls to free(s_entry->hfs_ent) too.
	 */
#endif
	return (s_entry1);
}

EXPORT void
generate_iso9660_directories(node, outfile)
	struct directory	*node;
	FILE			*outfile;
{
	struct directory *dpnt;

	dpnt = node;

	while (dpnt) {
		if (dpnt->extent > session_start) {
			generate_one_directory(dpnt, outfile);
		}
		if (dpnt->subdir)
			generate_iso9660_directories(dpnt->subdir, outfile);
		dpnt = dpnt->next;
	}
}

/*
 * XXX This may need some work for the MVS port.
 */
LOCAL void
set_de_path(parent, this)
	struct directory	*parent;
	struct directory	*this;
{
	char	*p;
	size_t	len;

	if (parent == NULL) {			/* We are just creating root */
		this->de_path = this->whole_name;
		return;
	} else if (parent == root) {		/* full path == component    */
		this->de_path = this->de_name;
		return;
	}
	len = strlen(parent->de_path)+1+strlen(this->de_name)+1;
	p = e_malloc(len);
	js_snprintf(p, len, "%s/%s", parent->de_path, this->de_name);
	this->de_path = p;
}

/*
 * Function:	find_or_create_directory
 *
 * Purpose:	Locate a directory entry in the tree, create if needed.
 *
 * Arguments:	parent & de are never NULL at the same time.
 */
EXPORT struct directory *
find_or_create_directory(parent, path, de, flag)
	struct directory	*parent;
	char			*path;
	struct directory_entry	*de;
	int			flag;
{
	struct directory *child = 0;
	struct directory *dpnt;
	struct directory_entry *orig_de;
	struct directory *next_brother;
	const char	*cpnt;
	const char	*pnt;
	int		deep_flag = 0;

	orig_de = de;

	/*
	 * XXX It seems that the tree that has been read from the
	 * XXX previous session does not carry whole_name entries.
	 * XXX We provide a hack in multi.c:find_or_create_directory()
	 * XXX that should be removed when a reasonable method could
	 * XXX be found.
	 */
	if (path == NULL) {
		error(_("Warning: missing whole name for: '%s'\n"), de->name);
		path = de->name;
		if (path == NULL)
			comerrno(EX_BAD, _("Panic no node name.\n"));
	}
	pnt = strrchr(path, PATH_SEPARATOR);
	if (pnt == NULL) {
		pnt = path;
	} else {
		pnt++;
	}

	if (parent != NULL) {

		dpnt = parent->subdir;
		if (dpnt == NULL) {
			struct directory_entry *s_entry;

			for (s_entry = parent->contents; s_entry;
						s_entry = s_entry->next) {
				if ((strcmp(s_entry->name, pnt) == 0) &&
				    (s_entry->de_flags & RELOCATED_DIRECTORY)) {
					return (find_or_create_directory(
							reloc_dir, path, de,
									flag));
				}
			}
		}

		while (dpnt) {
			/*
			 * Weird hack time - if there are two directories by
			 * the same name in the reloc_dir, they are not
			 * treated as the same thing unless the entire path
			 * matches completely.
			 */
			if (flag && strcmp(dpnt->de_name, pnt) == 0) {

				/*
				 * XXX Remove test?
				 * XXX dpnt->de_path should always be != NULL
				 */
				if (dpnt->de_path != NULL &&
				    strcmp(dpnt->de_path, path) == 0)
					return (dpnt);

				if (parent != reloc_dir &&
				    strcmp(dpnt->de_name, pnt) == 0)
					return (dpnt);
			}
			dpnt = dpnt->next;
		}
	}
	/*
	 * We don't know if we have a valid directory entry for this one yet.
	 * If not, we need to create one.
	 */
	if (de == NULL) {
		de = (struct directory_entry *)
			e_malloc(sizeof (struct directory_entry));
		memset(de, 0, sizeof (struct directory_entry));
		de->next = parent->contents;
		parent->contents = de;
		de->name = e_strdup(pnt);
		de->whole_name = e_strdup(path);
		de->filedir = parent;
		de->isorec.flags[0] = ISO_DIRECTORY;
		de->priority = 32768;
		de->inode = UNCACHED_INODE;
		de->dev = UNCACHED_DEVICE;
		set_723(de->isorec.volume_sequence_number,
						volume_sequence_number);
		iso9660_file_length(pnt, de, 1);

		init_fstatbuf();
#ifdef APPLE_HYB
		if (apple_both) {
			/*
			 * Give the directory an HFS entry
			 */
			hfsdirent	*hfs_ent;

			hfs_ent = (hfsdirent *) e_malloc(sizeof (hfsdirent));

			/*
			 * Fill in the defaults
			 */
			memset(hfs_ent, 0, sizeof (hfsdirent));
			hfs_ent->crdate = fstatbuf.st_ctime;
			hfs_ent->mddate = fstatbuf.st_mtime;

			de->hfs_ent = hfs_ent;

			/*
			 * Get the Mac directory name
			 */
			get_hfs_dir((char *)path, (char *)pnt, de);
		}
#endif	/* APPLE_HYB */
	}
	/*
	 * If we don't have a directory for this one yet, then allocate it now,
	 * and patch it into the tree in the appropriate place.
	 */
	dpnt = (struct directory *)e_malloc(sizeof (struct directory));
	memset(dpnt, 0, sizeof (struct directory));
	dpnt->next = NULL;
	dpnt->subdir = NULL;
	dpnt->self = de;
	dpnt->contents = NULL;
	dpnt->whole_name = e_strdup(path);
	cpnt = strrchr(path, PATH_SEPARATOR);
	if (cpnt)
		cpnt++;
	else
		cpnt = path;
	dpnt->de_name = e_strdup(cpnt);
	dpnt->de_path = NULL;
	set_de_path(parent, dpnt);
	dpnt->size = 0;
	dpnt->extent = 0;
	dpnt->jextent = 0;
	dpnt->jsize = 0;
#ifdef APPLE_HYB
	dpnt->hfs_ent = de->hfs_ent;
#endif	/* APPLE_HYB */

	if (orig_de == NULL) {
		struct stat	xstatbuf;
		int		sts;

		/*
		 * Now add a . and .. entry in the directory itself. This is a
		 * little tricky - if the real directory exists, we need to
		 * stat it first. Otherwise, we use the fictitious fstatbuf
		 * which points to the time at which mkisofs was started.
		 */
		if (parent == NULL || parent->whole_name[0] == '\0')
			sts = -1;
		else
			sts = stat_filter(parent->whole_name, &xstatbuf);
		if (debug && parent) {
			error(_("stat parent->whole_name: '%s' -> %d.\n"),
				parent->whole_name, sts);
		}
		if (sts == 0) {
			attach_dot_entries(dpnt, NULL, &xstatbuf);
		} else {
			attach_dot_entries(dpnt, NULL, NULL);
		}
	}
	if (!parent || parent == root) {
		if (!root) {
			root = dpnt;	/* First time through for root	*/
					/* directory only		*/
			root->depth = 0;
			root->parent = root;
		} else {
			dpnt->depth = 1;
			if (!root->subdir) {
				root->subdir = dpnt;
			} else {
				next_brother = root->subdir;
				while (next_brother->next)
					next_brother = next_brother->next;
				next_brother->next = dpnt;
			}
			dpnt->parent = parent;
		}
	} else {
		struct directory_entry *s_entry1;

		/*
		 * Come through here for  normal traversal of  tree
		 */
#ifdef DEBUG
		fprintf(stderr, "%s(%d) ", path, dpnt->depth);
#endif

		if (parent->depth > RR_relocation_depth && use_RockRidge) {
			/*
			 * We come here in case that a graft-point needs to
			 * create a new relocated (deep) directory.
			 *
			 * Replicate the entry for this directory.  The old one
			 * will stay where it is, and it will be neutered so
			 * that it no longer looks like a directory. The new
			 * one will look like a directory, and it will be put
			 * in the reloc_dir.
			 */
			s_entry1 = dup_relocated_dir(parent, de, path,
						dpnt->de_name, &fstatbuf);
			child = find_or_create_directory(reloc_dir, path,
								s_entry1, 0);
			free(child->de_path);	/* allocated in this case */
			set_de_path(parent, child);

			deep_flag |= NEED_CL;
		}
		if (parent->depth > RR_relocation_depth && !use_RockRidge) {
			dir_nesting_warn(parent, path, FALSE);
			exit(EX_BAD);
		}
		dpnt->parent = parent;
		dpnt->depth = parent->depth + 1;

		if ((deep_flag & NEED_CL) == 0) {
			/*
			 * Do not add this directory to the list of subdirs if
			 * this is a relocated directory.
			 */
			if (!parent->subdir) {
				parent->subdir = dpnt;
			} else {
				next_brother = parent->subdir;
				while (next_brother->next)
					next_brother = next_brother->next;
				next_brother->next = dpnt;
			}
		}
	}
	/*
	 * It doesn't exist for real, so we cannot add any
	 * XA or Rock Ridge attributes.
	 */
	if (orig_de == NULL || (parent == NULL && path[0] == '\0')) {
		init_fstatbuf();
		fstatbuf.st_mode = new_dir_mode | S_IFDIR;
		fstatbuf.st_nlink = 2;
		if ((use_XA || use_RockRidge) &&
		    !(parent == NULL && path[0] == '\0')) {
			/*
			 * We cannot set up RR attrubutes for the real
			 * ISO-9660 root directory. This is why we
			 * check for parent == NULL && path[0] == '\0'.
			 */
			generate_xa_rr_attributes("",
				(char *)pnt, de,
				&fstatbuf,
				&fstatbuf, deep_flag);
		}
#ifdef UDF
		/* set some info used for udf */
		de->mode = fstatbuf.st_mode;
		de->uid =  fstatbuf.st_uid;
		de->gid =  fstatbuf.st_gid;
		de->atime.tv_sec = fstatbuf.st_atime;
		de->atime.tv_nsec = stat_ansecs(&fstatbuf);
		de->mtime.tv_sec = fstatbuf.st_mtime;
		de->mtime.tv_nsec = stat_mnsecs(&fstatbuf);
		de->ctime.tv_sec = fstatbuf.st_ctime;
		de->ctime.tv_nsec = stat_cnsecs(&fstatbuf);
#endif
		iso9660_date(de->isorec.date, fstatbuf.st_mtime);
	}
	if (child)
		return (child);	/* Return reloaction target */

	return (dpnt);
}

/*
 * Function:	delete_directory
 *
 * Purpose:	Locate a directory entry in the tree, create if needed.
 *
 * Arguments:
 */
LOCAL void
delete_directory(parent, child)
	struct directory	*parent;
	struct directory	*child;
{
	struct directory *tdir;

	if (child == NULL)
		return;
	if (child->contents != NULL) {
		comerrno(EX_BAD, _("Unable to delete non-empty directory\n"));
	}
	free(child->whole_name);
	child->whole_name = NULL;

	free(child->de_name);
	child->de_name = NULL;

#ifdef APPLE_HYB
	if (apple_both && child->hfs_ent)
		free(child->hfs_ent);
#endif	/* APPLE_HYB */

	if (parent->subdir == child) {
		parent->subdir = child->next;
	} else {
		for (tdir = parent->subdir; tdir != NULL && tdir->next != NULL;
							tdir = tdir->next) {
			if (tdir->next == child) {
				tdir->next = child->next;
				break;
			}
		}
		if (tdir == NULL || tdir->next != child->next) {
			comerrno(EX_BAD,
			_("Unable to locate child directory in parent list\n"));
		}
	}
	free(child);
}

EXPORT int
sort_tree(node)
	struct directory	*node;
{
	struct directory *dpnt;
	int		ret = 0;

	dpnt = node;

	while (dpnt) {
		ret = sort_n_finish(dpnt);
		if (ret) {
			break;
		}
		if (dpnt->subdir)
			sort_tree(dpnt->subdir);
		dpnt = dpnt->next;
	}
	return (ret);
}

EXPORT void
dump_tree(node)
	struct directory *node;
{
	struct directory *dpnt;

	dpnt = node;

	while (dpnt) {
		fprintf(stderr, "%4d %5d %s\n",
				dpnt->extent, dpnt->size, dpnt->de_name);
		if (dpnt->subdir)
			dump_tree(dpnt->subdir);
		dpnt = dpnt->next;
	}
}

/*
 * something quick and dirty to locate a file given a path
 * recursively walks down path in filename until it finds the
 * directory entry for the desired file
 */
EXPORT struct directory_entry *
search_tree_file(node, filename)
	struct directory *node;
	char		*filename;
{
	struct directory_entry *depnt;
	struct directory *dpnt;
	char		*p1;
	char		*rest;
	char		*subdir;

	/*
	 * Strip off next directory name from filename:
	 */
	subdir = e_strdup(filename);

	if ((p1 = strchr(subdir, '/')) == subdir) {
		fprintf(stderr,
		_("call to search_tree_file with an absolute path, stripping\n"));
		fprintf(stderr,
		_("initial path separator. Hope this was intended...\n"));
		memmove(subdir, subdir + 1, strlen(subdir) - 1);
		p1 = strchr(subdir, '/');
	}
	/*
	 * Do we need to find a subdirectory?
	 */
	if (p1) {
		*p1 = '\0';

#ifdef DEBUG_TORITO
		fprintf(stderr, _("Looking for subdir called %s\n"), p1);
#endif

		rest = p1 + 1;

#ifdef DEBUG_TORITO
		fprintf(stderr, _("Remainder of path name is now %s\n"), rest);
#endif

		dpnt = node->subdir;
		while (dpnt) {
#ifdef DEBUG_TORITO
			fprintf(stderr,
				"%4d %5d %s\n", dpnt->extent, dpnt->size,
				dpnt->de_name);
#endif
			if (strcmp(subdir, dpnt->de_name) == 0) {
#ifdef DEBUG_TORITO
				fprintf(stderr,
				_("Calling next level with filename = %s\n"), rest);
#endif
				return (search_tree_file(dpnt, rest));
			}
			dpnt = dpnt->next;
		}

		/*
		 * If we got here means we couldn't find the subdir.
		 */
		return (NULL);
	} else {
		/*
		 * Look for a normal file now
		 */
		depnt = node->contents;
		while (depnt) {
#ifdef DEBUG_TORITO
			fprintf(stderr, "%4d %5d %s\n", depnt->isorec.extent,
				depnt->size, depnt->name);
#endif
			if (strcmp(filename, depnt->name) == 0) {
#ifdef DEBUG_TORITO
				fprintf(stderr, _("Found our file %s"), filename);
#endif
				return (depnt);
			}
			depnt = depnt->next;
		}
		/*
		 * If we got here means we couldn't find the subdir.
		 */
		return (NULL);
	}
#ifdef	ERIC_FUN
	fprintf(stderr, _("We cant get here in search_tree_file :-/ \n"));
#endif
}

EXPORT void
init_fstatbuf()
{
	struct timeval	current_time;

	if (fstatbuf.st_ctime == 0) {
		gettimeofday(&current_time, NULL);
		if (rationalize_uid)
			fstatbuf.st_uid = uid_to_use;
		else
			fstatbuf.st_uid = getuid();
		if (rationalize_gid)
			fstatbuf.st_gid = gid_to_use;
		else
			fstatbuf.st_gid = getgid();

		current_time.tv_usec *= 1000;
		fstatbuf.st_ctime = current_time.tv_sec;
		stat_set_ansecs(&fstatbuf, current_time.tv_usec);
		fstatbuf.st_mtime = current_time.tv_sec;
		stat_set_mnsecs(&fstatbuf, current_time.tv_usec);
		fstatbuf.st_atime = current_time.tv_sec;
		stat_set_cnsecs(&fstatbuf, current_time.tv_usec);
	}
}
