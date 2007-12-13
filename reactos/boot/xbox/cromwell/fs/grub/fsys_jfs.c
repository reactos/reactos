/* fsys_jfs.c - an implementation for the IBM JFS file system */
/*  
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001,2002  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef FSYS_JFS

#include "shared.h"
#include "filesys.h"
#include "jfs.h"

#define MAX_LINK_COUNT	8

#define DTTYPE_INLINE	0
#define DTTYPE_PAGE	1

struct jfs_info
{
	int bsize;
	int l2bsize;
	int bdlog;
	int xindex;
	int xlastindex;
	int sindex;
	int slastindex;
	int de_index;
	int dttype;
	xad_t *xad;
	ldtentry_t *de;
};

static struct jfs_info jfs;

#define xtpage		((xtpage_t *)FSYS_BUF)
#define dtpage		((dtpage_t *)((char *)FSYS_BUF + 4096))
#define fileset		((dinode_t *)((char *)FSYS_BUF + 8192))
#define inode		((dinode_t *)((char *)FSYS_BUF + 8192 + sizeof(dinode_t)))
#define dtroot		((dtroot_t *)(&inode->di_btroot))

static ldtentry_t de_always[2] = {
	{1, -1, 2, {'.', '.'}},
	{1, -1, 1, {'.'}}
};

static int
isinxt (s64 key, s64 offset, s64 len)
{
	return (key >= offset) ? (key < offset + len ? 1 : 0) : 0;
}

static xad_t *
first_extent (dinode_t *di)
{
	xtpage_t *xtp;

	jfs.xindex = 2;
	xtp = (xtpage_t *)&di->di_btroot;
	jfs.xad = &xtp->xad[2];
	if (xtp->header.flag & BT_LEAF) {
	    	jfs.xlastindex = xtp->header.nextindex;
	} else {
		do {
			devread (addressXAD (jfs.xad) << jfs.bdlog, 0,
				 sizeof(xtpage_t), (char *)xtpage);
			jfs.xad = &xtpage->xad[2];
		} while (!(xtpage->header.flag & BT_LEAF));
		jfs.xlastindex = xtpage->header.nextindex;
	}

	return jfs.xad;
}

static xad_t *
next_extent (void)
{
	if (++jfs.xindex < jfs.xlastindex) {
	} else if (xtpage->header.next) {
		devread (xtpage->header.next << jfs.bdlog, 0,
			 sizeof(xtpage_t), (char *)xtpage);
		jfs.xlastindex = xtpage->header.nextindex;
		jfs.xindex = XTENTRYSTART;
		jfs.xad = &xtpage->xad[XTENTRYSTART];
	} else {
		return NULL;
	}
	return ++jfs.xad;
}


static void
di_read (u32 inum, dinode_t *di)
{
	s64 key;
	u32 xd, ioffset;
	s64 offset;
	xad_t *xad;
	pxd_t pxd;

	key = (((inum >> L2INOSPERIAG) << L2INOSPERIAG) + 4096) >> jfs.l2bsize;
	xd = (inum & (INOSPERIAG - 1)) >> L2INOSPEREXT;
	ioffset = ((inum & (INOSPERIAG - 1)) & (INOSPEREXT - 1)) << L2DISIZE;
	xad = first_extent (fileset);
	do {
		offset = offsetXAD (xad);
		if (isinxt (key, offset, lengthXAD (xad))) {
			devread ((addressXAD (xad) + key - offset) << jfs.bdlog,
				 3072 + xd*sizeof(pxd_t), sizeof(pxd_t), (char *)&pxd);
			devread (addressPXD (&pxd) << jfs.bdlog,
				 ioffset, DISIZE, (char *)di);
			break;
		}
	} while ((xad = next_extent ()));
}

static ldtentry_t *
next_dentry (void)
{
	ldtentry_t *de;
	s8 *stbl;

	if (jfs.dttype == DTTYPE_INLINE) {
		if (jfs.sindex < jfs.slastindex) {
			return (ldtentry_t *)&dtroot->slot[(int)dtroot->header.stbl[jfs.sindex++]];
		}
	} else {
		de = (ldtentry_t *)dtpage->slot;
		stbl = (s8 *)&de[(int)dtpage->header.stblindex];
		if (jfs.sindex < jfs.slastindex) {
			return &de[(int)stbl[jfs.sindex++]];
		} else if (dtpage->header.next) {
			devread (dtpage->header.next << jfs.bdlog, 0,
				 sizeof(dtpage_t), (char *)dtpage);
			jfs.slastindex = dtpage->header.nextindex;
			jfs.sindex = 1;
			return &de[(int)((s8 *)&de[(int)dtpage->header.stblindex])[0]];
		}
	}

	return (jfs.de_index < 2) ? &de_always[jfs.de_index++] : NULL;
}

static ldtentry_t *
first_dentry (void)
{
	dtroot_t *dtr;
	pxd_t *xd;
	idtentry_t *de;

	dtr = (dtroot_t *)&inode->di_btroot;
	jfs.sindex = 0;
	jfs.de_index = 0;

	de_always[0].inumber = inode->di_parent;
	de_always[1].inumber = inode->di_number;
	if (dtr->header.flag & BT_LEAF) {
		jfs.dttype = DTTYPE_INLINE;
		jfs.slastindex = dtr->header.nextindex;
	} else {
		de = (idtentry_t *)dtpage->slot;
		jfs.dttype = DTTYPE_PAGE;
		xd = &((idtentry_t *)dtr->slot)[(int)dtr->header.stbl[0]].xd;
		for (;;) {
			devread (addressPXD (xd) << jfs.bdlog, 0,
				 sizeof(dtpage_t), (char *)dtpage);
			if (dtpage->header.flag & BT_LEAF)
				break;
			xd = &de[(int)((s8 *)&de[(int)dtpage->header.stblindex])[0]].xd;
		}
		jfs.slastindex = dtpage->header.nextindex;
	}

	return next_dentry ();
}


static dtslot_t *
next_dslot (int next)
{
	return (jfs.dttype == DTTYPE_INLINE)
		? (dtslot_t *)&dtroot->slot[next]
		: &((dtslot_t *)dtpage->slot)[next];
}

static void
uni2ansi (UniChar *uni, char *ansi, int len)
{
	for (; len; len--, uni++)
		*ansi++ = (*uni & 0xff80) ? '?' : *(char *)uni;
}

int
jfs_mount (void)
{
	struct jfs_superblock super;

	if (part_length < MINJFS >> SECTOR_BITS
	    || !devread (SUPER1_OFF >> SECTOR_BITS, 0,
			 sizeof(struct jfs_superblock), (char *)&super)
	    || (super.s_magic != JFS_MAGIC)
	    || !devread ((AITBL_OFF >> SECTOR_BITS) + FILESYSTEM_I,
			 0, DISIZE, (char*)fileset)) {
		return 0;
	}

	jfs.bsize = super.s_bsize;
	jfs.l2bsize = super.s_l2bsize;
	jfs.bdlog = jfs.l2bsize - SECTOR_BITS;

	return 1;
}

int
jfs_read (char *buf, int len)
{
	xad_t *xad;
	s64 endofprev, endofcur;
	s64 offset, xadlen;
	int toread, startpos, endpos;

	startpos = filepos;
	endpos = filepos + len;
	endofprev = (1ULL << 62) - 1;
	xad = first_extent (inode);
	do {
		offset = offsetXAD (xad);
		xadlen = lengthXAD (xad);
		if (isinxt (filepos >> jfs.l2bsize, offset, xadlen)) {
			endofcur = (offset + xadlen) << jfs.l2bsize; 
			toread = (endofcur >= endpos)
				  ? len : (endofcur - filepos);

			disk_read_func = disk_read_hook;
			devread (addressXAD (xad) << jfs.bdlog,
				 filepos - (offset << jfs.l2bsize), toread, buf);
			disk_read_func = NULL;

			buf += toread;
			len -= toread;
			filepos += toread;
		} else if (offset > endofprev) {
			toread = ((offset << jfs.l2bsize) >= endpos)
				  ? len : ((offset - endofprev) << jfs.l2bsize);
			len -= toread;
			filepos += toread;
			for (; toread; toread--) {
				*buf++ = 0;
			}
			continue;
		}
		endofprev = offset + xadlen; 
		xad = next_extent ();
	} while (len > 0 && xad);

	return filepos - startpos;
}

int
jfs_dir (char *dirname)
{
	char *ptr, *rest, ch;
	ldtentry_t *de;
	dtslot_t *ds;
	u32 inum, parent_inum;
	s64 di_size;
	u32 di_mode;
	int namlen, cmp, n, link_count;
	char namebuf[JFS_NAME_MAX + 1], linkbuf[JFS_PATH_MAX];

	parent_inum = inum = ROOT_I;
	link_count = 0;
	for (;;) {
		di_read (inum, inode);
		di_size = inode->di_size;
		di_mode = inode->di_mode;

		if ((di_mode & IFMT) == IFLNK) {
			if (++link_count > MAX_LINK_COUNT) {
				errnum = ERR_SYMLINK_LOOP;
				return 0;
			}
			if (di_size < (di_mode & INLINEEA ? 256 : 128)) {
				grub_memmove (linkbuf, inode->di_fastsymlink, di_size);
				n = di_size;
			} else if (di_size < JFS_PATH_MAX - 1) {
				filepos = 0;
				filemax = di_size;
				n = jfs_read (linkbuf, filemax);
			} else {
				errnum = ERR_FILELENGTH;
				return 0;
			}

			inum = (linkbuf[0] == '/') ? ROOT_I : parent_inum;
			while (n < (JFS_PATH_MAX - 1) && (linkbuf[n++] = *dirname++));
			linkbuf[n] = 0;
			dirname = linkbuf;
			continue;
		}

		if (!*dirname || isspace (*dirname)) {
			if ((di_mode & IFMT) != IFREG) {
				errnum = ERR_BAD_FILETYPE;
				return 0;
			}
			filepos = 0;
			filemax = di_size;
			return 1;
		}

		if ((di_mode & IFMT) != IFDIR) {
			errnum = ERR_BAD_FILETYPE;
			return 0;
		}

		for (; *dirname == '/'; dirname++);

		for (rest = dirname; (ch = *rest) && !isspace (ch) && ch != '/'; rest++);
		*rest = 0;

		de = first_dentry ();
		for (;;) {
			namlen = de->namlen;
			if (de->next == -1) {
				uni2ansi (de->name, namebuf, namlen);
				namebuf[namlen] = 0;
			} else {
				uni2ansi (de->name, namebuf, DTLHDRDATALEN);
				ptr = namebuf;
				ptr += DTLHDRDATALEN;
				namlen -= DTLHDRDATALEN;
				ds = next_dslot (de->next);
				while (ds->next != -1) {
					uni2ansi (ds->name, ptr, DTSLOTDATALEN);
					ptr += DTSLOTDATALEN;
					namlen -= DTSLOTDATALEN;
					ds = next_dslot (ds->next);
				}
				uni2ansi (ds->name, ptr, namlen);
				ptr += namlen;
				*ptr = 0;
			}

			cmp = (!*dirname) ? -1 : substring (dirname, namebuf);
#ifndef STAGE1_5
			if (print_possibilities && ch != '/'
			    && cmp <= 0) {
				if (print_possibilities > 0)
					print_possibilities = -print_possibilities;
				print_a_completion (namebuf);
			} else
#endif
			if (cmp == 0) {
				parent_inum = inum;
				inum = de->inumber;
		        	*(dirname = rest) = ch;
				break;
			}
			de = next_dentry ();
			if (de == NULL) {
				if (print_possibilities < 0)
					return 1;

				errnum = ERR_FILE_NOT_FOUND;
				*rest = ch;
				return 0;
			}
		}
	}
}

int
jfs_embed (int *start_sector, int needed_sectors)
{
	struct jfs_superblock super;

	if (needed_sectors > 63
	    || !devread (SUPER1_OFF >> SECTOR_BITS, 0,
			 sizeof (struct jfs_superblock),
			 (char *)&super)
	    || (super.s_magic != JFS_MAGIC)) {
		return 0;
	}

	*start_sector = 1;
	return 1;
}

#endif /* FSYS_JFS */
