/* fsys_xfs.c - an implementation for the SGI XFS file system */
/*  
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001,2002,2004  Free Software Foundation, Inc.
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

#ifdef FSYS_XFS

#include "shared.h"
#include "filesys.h"
#include "xfs.h"

#define MAX_LINK_COUNT	8

typedef struct xad {
	xfs_fileoff_t offset;
	xfs_fsblock_t start;
	xfs_filblks_t len;
} xad_t;

struct xfs_info {
	int bsize;
	int dirbsize;
	int isize;
	unsigned int agblocks;
	int bdlog;
	int blklog;
	int inopblog;
	int agblklog;
	int agnolog;
	unsigned int nextents;
	xfs_daddr_t next;
	xfs_daddr_t daddr;
	xfs_dablk_t forw;
	xfs_dablk_t dablk;
	xfs_bmbt_rec_32_t *xt;
	xfs_bmbt_ptr_t ptr0;
	int btnode_ptr0_off;
	int i8param;
	int dirpos;
	int dirmax;
	int blkoff;
	int fpos;
	xfs_ino_t rootino;
};

static struct xfs_info xfs;

#define dirbuf		((char *)FSYS_BUF)
#define filebuf		((char *)FSYS_BUF + 4096)
#define inode		((xfs_dinode_t *)((char *)FSYS_BUF + 8192))
#define icore		(inode->di_core)

#define	mask32lo(n)	(((xfs_uint32_t)1 << (n)) - 1)

#define	XFS_INO_MASK(k)		((xfs_uint32_t)((1ULL << (k)) - 1))
#define	XFS_INO_OFFSET_BITS	xfs.inopblog
#define	XFS_INO_AGBNO_BITS	xfs.agblklog
#define	XFS_INO_AGINO_BITS	(xfs.agblklog + xfs.inopblog)
#define	XFS_INO_AGNO_BITS	xfs.agnolog

static inline xfs_agblock_t
agino2agbno (xfs_agino_t agino)
{
	return agino >> XFS_INO_OFFSET_BITS;
}

static inline xfs_agnumber_t
ino2agno (xfs_ino_t ino)
{
	return ino >> XFS_INO_AGINO_BITS;
}

static inline xfs_agino_t
ino2agino (xfs_ino_t ino)
{
	return ino & XFS_INO_MASK(XFS_INO_AGINO_BITS);
}

static inline int
ino2offset (xfs_ino_t ino)
{
	return ino & XFS_INO_MASK(XFS_INO_OFFSET_BITS);
}

static inline __const__ xfs_uint16_t
le16 (xfs_uint16_t x)
{
	__asm__("xchgb %b0,%h0"	\
		: "=q" (x) \
		:  "0" (x)); \
		return x;
}

static inline __const__ xfs_uint32_t
le32 (xfs_uint32_t x)
{
#if 0
        /* 386 doesn't have bswap.  */
	__asm__("bswap %0" : "=r" (x) : "0" (x));
#else
	/* This is slower but this works on all x86 architectures.  */
	__asm__("xchgb %b0, %h0" \
		"\n\troll $16, %0" \
		"\n\txchgb %b0, %h0" \
		: "=q" (x) : "0" (x));
#endif
	return x;
}

static inline __const__ xfs_uint64_t
le64 (xfs_uint64_t x)
{
	xfs_uint32_t h = x >> 32;
        xfs_uint32_t l = x & ((1ULL<<32)-1);
        return (((xfs_uint64_t)le32(l)) << 32) | ((xfs_uint64_t)(le32(h)));
}


static xfs_fsblock_t
xt_start (xfs_bmbt_rec_32_t *r)
{
	return (((xfs_fsblock_t)(le32 (r->l1) & mask32lo(9))) << 43) | 
	       (((xfs_fsblock_t)le32 (r->l2)) << 11) |
	       (((xfs_fsblock_t)le32 (r->l3)) >> 21);
}

static xfs_fileoff_t
xt_offset (xfs_bmbt_rec_32_t *r)
{
	return (((xfs_fileoff_t)le32 (r->l0) &
		mask32lo(31)) << 23) |
		(((xfs_fileoff_t)le32 (r->l1)) >> 9);
}

static xfs_filblks_t
xt_len (xfs_bmbt_rec_32_t *r)
{
	return le32(r->l3) & mask32lo(21);
}

static inline int
xfs_highbit32(xfs_uint32_t v)
{
	int i;

	if (--v) {
		for (i = 0; i < 31; i++, v >>= 1) {
			if (v == 0)
				return i;
		}
	}
	return 0;
}

static int
isinxt (xfs_fileoff_t key, xfs_fileoff_t offset, xfs_filblks_t len)
{
	return (key >= offset) ? (key < offset + len ? 1 : 0) : 0;
}

static xfs_daddr_t
agb2daddr (xfs_agnumber_t agno, xfs_agblock_t agbno)
{
	return ((xfs_fsblock_t)agno*xfs.agblocks + agbno) << xfs.bdlog;
}

static xfs_daddr_t
fsb2daddr (xfs_fsblock_t fsbno)
{
	return agb2daddr ((xfs_agnumber_t)(fsbno >> xfs.agblklog),
			 (xfs_agblock_t)(fsbno & mask32lo(xfs.agblklog)));
}

#undef offsetof
#define offsetof(t,m)	((int)&(((t *)0)->m))

static inline int
btroot_maxrecs (void)
{
	int tmp = icore.di_forkoff ? (icore.di_forkoff << 3) : xfs.isize;

	return (tmp - sizeof(xfs_bmdr_block_t) - offsetof(xfs_dinode_t, di_u)) /
		(sizeof (xfs_bmbt_key_t) + sizeof (xfs_bmbt_ptr_t));
}

static int
di_read (xfs_ino_t ino)
{
	xfs_agino_t agino;
	xfs_agnumber_t agno;
	xfs_agblock_t agbno;
	xfs_daddr_t daddr;
	int offset;

	agno = ino2agno (ino);
	agino = ino2agino (ino);
	agbno = agino2agbno (agino);
	offset = ino2offset (ino);
	daddr = agb2daddr (agno, agbno);

	devread (daddr, offset*xfs.isize, xfs.isize, (char *)inode);

	xfs.ptr0 = *(xfs_bmbt_ptr_t *)
		    (inode->di_u.di_c + sizeof(xfs_bmdr_block_t)
		    + btroot_maxrecs ()*sizeof(xfs_bmbt_key_t));

	return 1;
}

static void
init_extents (void)
{
	xfs_bmbt_ptr_t ptr0;
	xfs_btree_lblock_t h;

	switch (icore.di_format) {
	case XFS_DINODE_FMT_EXTENTS:
		xfs.xt = inode->di_u.di_bmx;
		xfs.nextents = le32 (icore.di_nextents);
		break;
	case XFS_DINODE_FMT_BTREE:
		ptr0 = xfs.ptr0;
		for (;;) {
			xfs.daddr = fsb2daddr (le64(ptr0));
			devread (xfs.daddr, 0,
				 sizeof(xfs_btree_lblock_t), (char *)&h);
			if (!h.bb_level) {
				xfs.nextents = le16(h.bb_numrecs);
				xfs.next = fsb2daddr (le64(h.bb_rightsib));
				xfs.fpos = sizeof(xfs_btree_block_t);
				return;
			}
			devread (xfs.daddr, xfs.btnode_ptr0_off,
				 sizeof(xfs_bmbt_ptr_t), (char *)&ptr0);
		}
	}
}

static xad_t *
next_extent (void)
{
	static xad_t xad;

	switch (icore.di_format) {
	case XFS_DINODE_FMT_EXTENTS:
		if (xfs.nextents == 0)
			return NULL;
		break;
	case XFS_DINODE_FMT_BTREE:
		if (xfs.nextents == 0) {
			xfs_btree_lblock_t h;
			if (xfs.next == 0)
				return NULL;
			xfs.daddr = xfs.next;
			devread (xfs.daddr, 0, sizeof(xfs_btree_lblock_t), (char *)&h);
			xfs.nextents = le16(h.bb_numrecs);
			xfs.next = fsb2daddr (le64(h.bb_rightsib));
			xfs.fpos = sizeof(xfs_btree_block_t);
		}
		/* Yeah, I know that's slow, but I really don't care */
		devread (xfs.daddr, xfs.fpos, sizeof(xfs_bmbt_rec_t), filebuf);
		xfs.xt = (xfs_bmbt_rec_32_t *)filebuf;
		xfs.fpos += sizeof(xfs_bmbt_rec_32_t);
	}
	xad.offset = xt_offset (xfs.xt);
	xad.start = xt_start (xfs.xt);
	xad.len = xt_len (xfs.xt);
	++xfs.xt;
	--xfs.nextents;

	return &xad;
}

/*
 * Name lies - the function reads only first 100 bytes
 */
static void
xfs_dabread (void)
{
	xad_t *xad;
	xfs_fileoff_t offset;;

	init_extents ();
	while ((xad = next_extent ())) {
		offset = xad->offset;
		if (isinxt (xfs.dablk, offset, xad->len)) {
			devread (fsb2daddr (xad->start + xfs.dablk - offset),
				 0, 100, dirbuf);
			break;
		}
	}
}

static inline xfs_ino_t
sf_ino (char *sfe, int namelen)
{
	void *p = sfe + namelen + 3;

	return (xfs.i8param == 0)
		? le64(*(xfs_ino_t *)p) : le32(*(xfs_uint32_t *)p);
}

static inline xfs_ino_t
sf_parent_ino (void)
{
	return (xfs.i8param == 0)
		? le64(*(xfs_ino_t *)(&inode->di_u.di_dir2sf.hdr.parent))
		: le32(*(xfs_uint32_t *)(&inode->di_u.di_dir2sf.hdr.parent));
}

static inline int
roundup8 (int n)
{
	return ((n+7)&~7);
}

static char *
next_dentry (xfs_ino_t *ino)
{
	int namelen = 1;
	int toread;
	static char *usual[2] = {".", ".."};
	static xfs_dir2_sf_entry_t *sfe;
	char *name = usual[0];

	if (xfs.dirpos >= xfs.dirmax) {
		if (xfs.forw == 0)
			return NULL;
		xfs.dablk = xfs.forw;
		xfs_dabread ();
#define h	((xfs_dir2_leaf_hdr_t *)dirbuf)
		xfs.dirmax = le16 (h->count) - le16 (h->stale);
		xfs.forw = le32 (h->info.forw);
#undef h
		xfs.dirpos = 0;
	}

	switch (icore.di_format) {
	case XFS_DINODE_FMT_LOCAL:
		switch (xfs.dirpos) {
		case -2:
			*ino = 0;
			break;
		case -1:
			*ino = sf_parent_ino ();
			++name;
			++namelen;
			sfe = (xfs_dir2_sf_entry_t *)
				(inode->di_u.di_c 
				 + sizeof(xfs_dir2_sf_hdr_t)
				 - xfs.i8param);
			break;
		default:
			namelen = sfe->namelen;
			*ino = sf_ino ((char *)sfe, namelen);
			name = sfe->name;
			sfe = (xfs_dir2_sf_entry_t *)
				  ((char *)sfe + namelen + 11 - xfs.i8param);
		}
		break;
	case XFS_DINODE_FMT_BTREE:
	case XFS_DINODE_FMT_EXTENTS:
#define dau	((xfs_dir2_data_union_t *)dirbuf)
		for (;;) {
			if (xfs.blkoff >= xfs.dirbsize) {
				xfs.blkoff = sizeof(xfs_dir2_data_hdr_t);
				filepos &= ~(xfs.dirbsize - 1);
				filepos |= xfs.blkoff;
			}
			xfs_read (dirbuf, 4);
			xfs.blkoff += 4;
			if (dau->unused.freetag == XFS_DIR2_DATA_FREE_TAG) {
				toread = roundup8 (le16(dau->unused.length)) - 4;
				xfs.blkoff += toread;
				filepos += toread;
				continue;
			}
			break;
		}
		xfs_read ((char *)dirbuf + 4, 5);
		*ino = le64 (dau->entry.inumber);
		namelen = dau->entry.namelen;
#undef dau
		toread = roundup8 (namelen + 11) - 9;
		xfs_read (dirbuf, toread);
		name = (char *)dirbuf;
		xfs.blkoff += toread + 5;
	}
	++xfs.dirpos;
	name[namelen] = 0;

	return name;
}

static char *
first_dentry (xfs_ino_t *ino)
{
	xfs.forw = 0;
	switch (icore.di_format) {
	case XFS_DINODE_FMT_LOCAL:
		xfs.dirmax = inode->di_u.di_dir2sf.hdr.count;
		xfs.i8param = inode->di_u.di_dir2sf.hdr.i8count ? 0 : 4;
		xfs.dirpos = -2;
		break;
	case XFS_DINODE_FMT_EXTENTS:
	case XFS_DINODE_FMT_BTREE:
		filepos = 0;
		xfs_read (dirbuf, sizeof(xfs_dir2_data_hdr_t));
		if (((xfs_dir2_data_hdr_t *)dirbuf)->magic == le32(XFS_DIR2_BLOCK_MAGIC)) {
#define tail		((xfs_dir2_block_tail_t *)dirbuf)
			filepos = xfs.dirbsize - sizeof(*tail);
			xfs_read (dirbuf, sizeof(*tail));
			xfs.dirmax = le32 (tail->count) - le32 (tail->stale);
#undef tail
		} else {
			xfs.dablk = (1ULL << 35) >> xfs.blklog;
#define h		((xfs_dir2_leaf_hdr_t *)dirbuf)
#define n		((xfs_da_intnode_t *)dirbuf)
			for (;;) {
				xfs_dabread ();
				if ((n->hdr.info.magic == le16(XFS_DIR2_LEAFN_MAGIC))
				    || (n->hdr.info.magic == le16(XFS_DIR2_LEAF1_MAGIC))) {
					xfs.dirmax = le16 (h->count) - le16 (h->stale);
					xfs.forw = le32 (h->info.forw);
					break;
				}
				xfs.dablk = le32 (n->btree[0].before);
			}
#undef n
#undef h
		}
		xfs.blkoff = sizeof(xfs_dir2_data_hdr_t);
		filepos = xfs.blkoff;
		xfs.dirpos = 0;
	}
	return next_dentry (ino);
}

int
xfs_mount (void)
{
	xfs_sb_t super;

	if (!devread (0, 0, sizeof(super), (char *)&super)
	    || (le32(super.sb_magicnum) != XFS_SB_MAGIC)
	    || ((le16(super.sb_versionnum) 
		& XFS_SB_VERSION_NUMBITS) != XFS_SB_VERSION_4) ) {
		return 0;
	}

	xfs.bsize = le32 (super.sb_blocksize);
	xfs.blklog = super.sb_blocklog;
	xfs.bdlog = xfs.blklog - SECTOR_BITS;
	xfs.rootino = le64 (super.sb_rootino);
	xfs.isize = le16 (super.sb_inodesize);
	xfs.agblocks = le32 (super.sb_agblocks);
	xfs.dirbsize = xfs.bsize << super.sb_dirblklog;

	xfs.inopblog = super.sb_inopblog;
	xfs.agblklog = super.sb_agblklog;
	xfs.agnolog = xfs_highbit32 (le32(super.sb_agcount));

	xfs.btnode_ptr0_off =
		((xfs.bsize - sizeof(xfs_btree_block_t)) /
		(sizeof (xfs_bmbt_key_t) + sizeof (xfs_bmbt_ptr_t)))
		 * sizeof(xfs_bmbt_key_t) + sizeof(xfs_btree_block_t);

	return 1;
}

int
xfs_read (char *buf, int len)
{
	xad_t *xad;
	xfs_fileoff_t endofprev, endofcur, offset;
	xfs_filblks_t xadlen;
	int toread, startpos, endpos;

	if (icore.di_format == XFS_DINODE_FMT_LOCAL) {
		grub_memmove (buf, inode->di_u.di_c + filepos, len);
		filepos += len;
		return len;
	}

	startpos = filepos;
	endpos = filepos + len;
	endofprev = (xfs_fileoff_t)-1;
	init_extents ();
	while (len > 0 && (xad = next_extent ())) {
		offset = xad->offset;
		xadlen = xad->len;
		if (isinxt (filepos >> xfs.blklog, offset, xadlen)) {
			endofcur = (offset + xadlen) << xfs.blklog; 
			toread = (endofcur >= endpos)
				  ? len : (endofcur - filepos);

			disk_read_func = disk_read_hook;
			devread (fsb2daddr (xad->start),
				 filepos - (offset << xfs.blklog), toread, buf);
			disk_read_func = NULL;

			buf += toread;
			len -= toread;
			filepos += toread;
		} else if (offset > endofprev) {
			toread = ((offset << xfs.blklog) >= endpos)
				  ? len : ((offset - endofprev) << xfs.blklog);
			len -= toread;
			filepos += toread;
			for (; toread; toread--) {
				*buf++ = 0;
			}
			continue;
		}
		endofprev = offset + xadlen; 
	}

	return filepos - startpos;
}

int
xfs_dir (char *dirname)
{
	xfs_ino_t ino, parent_ino, new_ino;
	xfs_fsize_t di_size;
	int di_mode;
	int cmp, n, link_count;
	char linkbuf[xfs.bsize];
	char *rest, *name, ch;

	parent_ino = ino = xfs.rootino;
	link_count = 0;
	for (;;) {
		di_read (ino);
		di_size = le64 (icore.di_size);
		di_mode = le16 (icore.di_mode);

		if ((di_mode & IFMT) == IFLNK) {
			if (++link_count > MAX_LINK_COUNT) {
				errnum = ERR_SYMLINK_LOOP;
				return 0;
			}
			if (di_size < xfs.bsize - 1) {
				filepos = 0;
				filemax = di_size;
				n = xfs_read (linkbuf, filemax);
			} else {
				errnum = ERR_FILELENGTH;
				return 0;
			}

			ino = (linkbuf[0] == '/') ? xfs.rootino : parent_ino;
			while (n < (xfs.bsize - 1) && (linkbuf[n++] = *dirname++));
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

		name = first_dentry (&new_ino);
		for (;;) {
			cmp = (!*dirname) ? -1 : substring (dirname, name);
#ifndef STAGE1_5
			if (print_possibilities && ch != '/' && cmp <= 0) {
				if (print_possibilities > 0)
					print_possibilities = -print_possibilities;
				print_a_completion (name);
			} else
#endif
			if (cmp == 0) {
				parent_ino = ino;
				if (new_ino)
					ino = new_ino;
		        	*(dirname = rest) = ch;
				break;
			}
			name = next_dentry (&new_ino);
			if (name == NULL) {
				if (print_possibilities < 0)
					return 1;

				errnum = ERR_FILE_NOT_FOUND;
				*rest = ch;
				return 0;
			}
		}
	}
}

#endif /* FSYS_XFS */
