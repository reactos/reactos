/* @(#)hash.c	1.30 16/12/13 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)hash.c	1.30 16/12/13 joerg";

#endif
/*
 * File hash.c - generate hash tables for iso9660 filesystem.
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

/* APPLE_HYB James Pearson j.pearson@ge.ucl.ac.uk 23/2/2000 */

/*
 * From jb@danware.dk:
 *
 * Cygwin fakes inodes by hashing file info, actual collisions observed!
 * This is documented in the cygwin source, look at winsup/cygwin/path.cc
 * and search for the word 'Hash'.  On NT, cygwin ORs together the
 * high and low 32 bits of the 64 bit genuine inode, look at fhandler.cc.
 *
 * Note:	Other operating systems which support the FAT filesystem may
 *		have the same problem because FAT does not use the inode
 *		concept.  For NTFS, genuine inode numbers exist, but they are
 *		64 bits and available only through an open file handle.
 *
 * The solution is the new options -no-cache-inodes/-cache-inodes that
 * allow to disable the mkisofs inode cache.
 */

/* DUPLICATES_ONCE Alex Kopylov cdrtools@bootcd.ru 19.06.2004 */

#include "mkisofs.h"
#include <schily/schily.h>
#include <schily/sha3.h>

#define	NR_HASH	(16*1024)

#define	HASH_FN(DEV, INO)	((DEV + INO  + (INO >> 8) + (INO << 16)) % NR_HASH)

static struct file_hash *hash_table[NR_HASH];

#ifdef	DUPLICATES_ONCE

#define	DIGEST_FAST_SIZE		65536
#define	UNIQUE_FILES_HASH_FN(SIZE)	((SIZE) % NR_HASH)

#ifndef	DIGEST_Init
#define	DIGEST_Init			SHA3_512_Init
#define	DIGEST_Update			SHA3_Update
#define	DIGEST_Final			SHA3_Final
#define	DIGEST_CTX			SHA3_CTX
#define	DIGEST_LENGTH			SHA3_512_DIGEST_LENGTH

#ifdef	SHORT_DIGEST
#undef	DIGEST_Init
#undef	DIGEST_LENGTH
#define	DIGEST_Init			SHA3_256_Init
#define	DIGEST_LENGTH			SHA3_256_DIGEST_LENGTH
#endif	/* SHORT_DIGEST */
#endif	/* DIGEST_Init */
#endif

#ifdef	HASH_DEBUG
EXPORT	void		debug_hash	__PR((void));
#endif
EXPORT	void		add_hash	__PR((struct directory_entry *spnt));
EXPORT	struct file_hash *find_hash	__PR((struct directory_entry *spnt));
EXPORT	void		flush_hash	__PR((void));
EXPORT	void		add_directory_hash __PR((dev_t dev, ino_t inode));
EXPORT	struct file_hash *find_directory_hash __PR((dev_t dev, ino_t inode));
LOCAL	unsigned int	name_hash	__PR((const char *name));
EXPORT	void		add_file_hash	__PR((struct directory_entry *de));
EXPORT	struct directory_entry *find_file_hash __PR((char *name));
LOCAL	BOOL		isoname_endsok	__PR((char *name));
EXPORT	int		delete_file_hash __PR((struct directory_entry *de));
EXPORT	void		flush_file_hash	__PR((void));
#ifdef	DUPLICATES_ONCE
LOCAL	struct directory_entry *compare_files __PR((
					struct directory_entry *spnt1,
					struct directory_entry *spnt2));
LOCAL	unsigned char	*DIGEST_File	__PR((char *name, size_t size));
#endif

#ifdef	HASH_DEBUG
EXPORT void
debug_hash()
{
	struct file_hash **p = hash_table;
	int	i;
	int	j = 0;
	struct file_hash *p2;
	int	k;
	int	maxlen = 0;
	int	minlen = 100000000;
	int	tot = 0;

	for (i = 0; i < NR_HASH; i++, p++) {
		if (*p == 0)
			j++;
		else {
			p2 = *p;
			k = 0;
			while (p2) {
				tot++;
				k++;
				p2 = p2->next;
			}
			if (k > maxlen)
				maxlen = k;
			if (k < minlen)
				minlen = k;
		}
	}
	error("Unbenutzt: %d von %d Einträgen maxlen %d minlen %d total %d optmittel %d\n",
		j, NR_HASH, maxlen, minlen, tot, tot/NR_HASH);
}
#endif

EXPORT void
add_hash(spnt)
	struct directory_entry	*spnt;
{
	struct file_hash *s_hash;
	unsigned int    hash_number = 0;

	if (spnt->size == 0 || spnt->starting_block == 0)
		if (spnt->size != 0 && spnt->starting_block == 0) {
			comerrno(EX_BAD,
			_("Non zero-length file '%s' assigned zero extent.\n"),
							spnt->name);
		};

#ifdef	DUPLICATES_ONCE
	if (!cache_inodes && !duplicates_once)
#else
	if (!cache_inodes)
#endif
		return;
	if (spnt->dev == UNCACHED_DEVICE &&
	    (spnt->inode == TABLE_INODE || spnt->inode == UNCACHED_INODE)) {
		return;
	}
#ifdef	DUPLICATES_ONCE
	if (cache_inodes)
#endif
		hash_number = HASH_FN((unsigned int) spnt->dev,
						(unsigned int) spnt->inode);
#ifdef	DUPLICATES_ONCE
	else if (duplicates_once &&
		    spnt->size && !(spnt->isorec.flags[0] & ISO_DIRECTORY))
		hash_number = UNIQUE_FILES_HASH_FN((unsigned int) spnt->size);
#endif

#if 0
	if (verbose > 1)
		fprintf(stderr, "%s ", spnt->name);
#endif
	s_hash = (struct file_hash *)e_malloc(sizeof (struct file_hash));
	s_hash->next = hash_table[hash_number];
	s_hash->inode = spnt->inode;
	s_hash->dev = spnt->dev;
	s_hash->nlink = 0;
	s_hash->starting_block = spnt->starting_block;
	s_hash->size = spnt->size;
#if	defined(SORTING) || defined(DUPLICATES_ONCE)
	s_hash->de = spnt;
#endif /* defined(SORTING) || defined(DUPLICATES_ONCE) */
	hash_table[hash_number] = s_hash;
}

EXPORT struct file_hash *
find_hash(spnt)
	struct directory_entry	*spnt;
{
	unsigned int    hash_number;
	struct file_hash *s_hash;

#ifdef	DUPLICATES_ONCE
	if (!cache_inodes && !duplicates_once)
#else
	if (!cache_inodes)
#endif
		return (NULL);
	if (spnt->dev == UNCACHED_DEVICE &&
	    (spnt->inode == TABLE_INODE || spnt->inode == UNCACHED_INODE))
		return (NULL);

#ifdef	DUPLICATES_ONCE
	if (cache_inodes) {
#endif
		hash_number = HASH_FN((unsigned int) spnt->dev,
				    (unsigned int) spnt->inode);
		s_hash = hash_table[hash_number];
		while (s_hash) {
			if (s_hash->inode == spnt->inode &&
			    s_hash->dev == spnt->dev)
				return (s_hash);
			s_hash = s_hash->next;
		}
#ifdef	DUPLICATES_ONCE
	} else if (duplicates_once &&
		    spnt->size && !(spnt->isorec.flags[0] & ISO_DIRECTORY)) {
		hash_number = UNIQUE_FILES_HASH_FN((unsigned int) spnt->size);
		s_hash = hash_table[hash_number];
		while (s_hash) {
			if (compare_files(spnt, s_hash->de))
				return (s_hash);
			s_hash = s_hash->next;
		}
	}
#endif
	return (NULL);
}

/*
 * based on flush_file_hash() below - needed as we want to re-use the
 * file hash table.
 */
EXPORT void
flush_hash()
{
	struct file_hash	*fh;
	struct file_hash	*fh1;
	int			i;

	for (i = 0; i < NR_HASH; i++) {
		fh = hash_table[i];
		while (fh) {
			fh1 = fh->next;
			free(fh);
			fh = fh1;
		}
		hash_table[i] = NULL;
	}
}

#ifdef	DUPLICATES_ONCE
LOCAL struct directory_entry *
compare_files(spnt1, spnt2)
	struct directory_entry	*spnt1;
	struct directory_entry	*spnt2;
{
	if (spnt1->size != spnt2->size)
		return (NULL);

	if (!spnt1->digest_fast)
		if (!(spnt1->digest_fast = DIGEST_File(spnt1->whole_name,
				(spnt1->size > DIGEST_FAST_SIZE) ?
				DIGEST_FAST_SIZE : spnt1->size)))
			return (NULL);

	if (spnt1->size <= DIGEST_FAST_SIZE)
		spnt1->digest_full = spnt1->digest_fast;

	if (!spnt2->digest_fast)
		if (!(spnt2->digest_fast = DIGEST_File(spnt2->whole_name,
				(spnt2->size > DIGEST_FAST_SIZE) ?
				DIGEST_FAST_SIZE : spnt2->size)))
			return (NULL);

	if (spnt2->size <= DIGEST_FAST_SIZE)
		spnt2->digest_full = spnt2->digest_fast;

	if (memcmp(spnt1->digest_fast, spnt2->digest_fast,
			DIGEST_LENGTH * sizeof (unsigned char)))
		return (NULL);

	if (!spnt1->digest_full)
		if (!(spnt1->digest_full = DIGEST_File(spnt1->whole_name,
						spnt1->size)))
			return (NULL);

	if (!spnt2->digest_full)
		if (!(spnt2->digest_full = DIGEST_File(spnt2->whole_name,
						spnt2->size)))
			return (NULL);

	if (memcmp(spnt1->digest_full, spnt2->digest_full,
			DIGEST_LENGTH * sizeof (unsigned char)))
		return (NULL);

	return (spnt2);
}

LOCAL unsigned char *
DIGEST_File(name, size)
	char		*name;
	size_t		size;
{
	DIGEST_CTX	digest_ctx;
	FILE		*infile;
	unsigned char	buf[32768];
	unsigned char	*digest_hash;
	size_t		cnt;

	DIGEST_Init(&digest_ctx);

	if ((infile = fopen(name, "rb")) == NULL)
		return (NULL);

	while (size) {
		cnt = (size > sizeof (buf)) ? sizeof (buf) : size;
		if ((cnt = fread(buf, 1, cnt, infile)) <= 0) {
			fclose(infile);
			return (NULL);
		}
		DIGEST_Update(&digest_ctx, buf, cnt);
		size -= cnt;
	}

	fclose(infile);

	digest_hash = e_malloc(sizeof (unsigned char) * DIGEST_LENGTH);
	DIGEST_Final(digest_hash, &digest_ctx);
	return (digest_hash);
}
#endif

static struct file_hash *directory_hash_table[NR_HASH];

#ifdef	PROTOTYPES
EXPORT void
add_directory_hash(dev_t dev, ino_t inode)
#else
EXPORT void
add_directory_hash(dev, inode)
	dev_t	dev;
	ino_t	inode;
#endif
{
	struct file_hash *s_hash;
	unsigned int    hash_number;

	if (!cache_inodes)
		return;
	if (dev == UNCACHED_DEVICE &&
	    (inode == TABLE_INODE || inode == UNCACHED_INODE))
		return;

	hash_number = HASH_FN((unsigned int) dev, (unsigned int) inode);

	s_hash = (struct file_hash *)e_malloc(sizeof (struct file_hash));
	s_hash->next = directory_hash_table[hash_number];
	s_hash->inode = inode;
	s_hash->dev = dev;
	s_hash->nlink = 0;
	directory_hash_table[hash_number] = s_hash;
}

#ifdef	PROTOTYPES
EXPORT struct file_hash *
find_directory_hash(dev_t dev, ino_t inode)
#else
EXPORT struct file_hash *
find_directory_hash(dev, inode)
	dev_t	dev;
	ino_t	inode;
#endif
{
	unsigned int    hash_number;
	struct file_hash *spnt;

	if (!cache_inodes)
		return (NULL);
	if (dev == UNCACHED_DEVICE &&
	    (inode == TABLE_INODE || inode == UNCACHED_INODE))
		return (NULL);

	hash_number = HASH_FN((unsigned int) dev, (unsigned int) inode);
	spnt = directory_hash_table[hash_number];
	while (spnt) {
		if (spnt->inode == inode && spnt->dev == dev)
			return (spnt);
		spnt = spnt->next;
	};
	return (NULL);
}

struct name_hash {
	struct name_hash *next;
	struct directory_entry *de;
	int	sum;
};

#define	NR_NAME_HASH	128

static struct name_hash *name_hash_table[NR_NAME_HASH] = {0, };

/*
 * Find the hash bucket for this name.
 */
LOCAL unsigned int
name_hash(name)
	const char	*name;
{
	unsigned int	hash = 0;
	const char	*p;

	p = name;

	while (*p) {
		/*
		 * Don't hash the  iso9660 version number.
		 * This way we can detect duplicates in cases where we have
		 * directories (i.e. foo) and non-directories (i.e. foo;1).
		 */
		if (*p == ';') {
			break;
		}
		hash = (hash << 15) + (hash << 3) + (hash >> 3) + (*p++ & 0xFF);
	}
	return (hash % NR_NAME_HASH);
}

EXPORT void
add_file_hash(de)
	struct directory_entry	*de;
{
	struct name_hash	*new;
	int			hash;
	Uchar			*p;
	int			sum = 0;

	new = (struct name_hash *)e_malloc(sizeof (struct name_hash));
	new->de = de;
	new->next = NULL;
	for (p = (Uchar *)de->isorec.name; *p; p++) {
		if (*p == ';')
			break;
		sum += *p & 0xFF;
	}
	new->sum = sum;
	hash = name_hash(de->isorec.name);

	/* Now insert into the hash table */
	new->next = name_hash_table[hash];
	name_hash_table[hash] = new;
}

EXPORT struct directory_entry *
find_file_hash(name)
	register char			*name;
{
	register char			*p1;
	register char			*p2;
	register struct name_hash	*nh;
	register int			sum = 0;

	if (debug > 1)
		error("find_hash('%s')\n", name);

	for (p1 = name; *p1; p1++) {
		if (*p1 == ';')
			break;
		sum += *p1 & 0xFF;
	}

	for (nh = name_hash_table[name_hash(name)]; nh; nh = nh->next) {
		if (nh->sum != sum)
			continue;

		p1 = name;
		p2 = nh->de->isorec.name;
		if (debug > 1)
			error(_("Checking name '%s' isorec.name '%s'\n"), p1, p2);

		/* Look for end of string, or a mismatch. */
		while (1 == 1) {
			if ((*p1 == '\0' || *p1 == ';') ||
			    (*p2 == '\0' || *p2 == ';') ||
			    (*p1 != *p2)) {
				break;
			}
			p1++;
			p2++;
		}
		if (!isoname_endsok(p1) || !isoname_endsok(p2)) {
			if (debug > 1) {
				if (!isoname_endsok(p1))
					error(_("'%s' does NOT END OK\n"), p1);
				if (!isoname_endsok(p2))
					error(_("'%s' does NOT END OK\n"), p2);
			}
			/*
			 * If one file does not end with a valid version number
			 * and the other name ends here, we found a miss match.
			 */
			if (*p1 == '\0' || *p2 == '\0')
				continue;

			if (*p1 == ';' && *p2 == ';') {
				p1++;
				p2++;
				continue;
			}
		}

		/*
		 * If we are at the end of both strings, then we have a match.
		 */
		if ((*p1 == '\0' || *p1 == ';') &&
		    (*p2 == '\0' || *p2 == ';')) {
			return (nh->de);
		}
	}
	return (NULL);
}

/*
 * The macro 'eo' is just an idea on how one might speed up isoname_endsok()
 */
#define	eo(p)	(((p)[0] == '\0') || \
		((p)[0] == ';' && (p)[1] == '1' && (p)[2] == '\0') || \
		isoname_endsok(p))

LOCAL BOOL
isoname_endsok(name)
	char	*name;
{
	int	i;
	char	*p;

	if (*name == '\0')
		return (TRUE);
	if (*name != ';')
		return (FALSE);

	for (p = ++name, i = 0; *p && i < 5; p++, i++) {
		if (*p < '0' || *p > '9')
			return (FALSE);
	}
	i = atoi(name);
	if (i < 1 || i > 32767)
		return (FALSE);
	return (TRUE);
}

EXPORT int
delete_file_hash(de)
	struct directory_entry	*de;
{
	struct name_hash	*nh;
	struct name_hash	*prev;
	int			hash;

	prev = NULL;
	hash = name_hash(de->isorec.name);
	for (nh = name_hash_table[hash]; nh; nh = nh->next) {
		if (nh->de == de)
			break;
		prev = nh;
	}
	if (!nh)
		return (1);
	if (!prev)
		name_hash_table[hash] = nh->next;
	else
		prev->next = nh->next;
	free(nh);
	return (0);
}

EXPORT void
flush_file_hash()
{
	struct name_hash	*nh;
	struct name_hash	*nh1;
	int			i;

	for (i = 0; i < NR_NAME_HASH; i++) {
		nh = name_hash_table[i];
		while (nh) {
			nh1 = nh->next;
			free(nh);
			nh = nh1;
		}
		name_hash_table[i] = NULL;

	}
}
