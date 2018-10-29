/* check.c - Check and repair a PC/MS-DOS filesystem

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
   Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
   Copyright (C) 2008-2014 Daniel Baumann <mail@daniel-baumann.ch>
   Copyright (C) 2015 Andreas Bombe <aeb@debian.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.

   The complete text of the GNU General Public License
   can be found in /usr/share/common-licenses/GPL-3 file.
*/

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */

#include "vfatlib.h"

#define NDEBUG
#include <debug.h>


/* the longest path on the filesystem that can be handled by path_name() */
#define PATH_NAME_MAX 1023

static DOS_FILE *root;

/* get start field of a dir entry */
#define FSTART(p,fs) \
  ((uint32_t)le16toh(p->dir_ent.start) | \
   (fs->fat_bits == 32 ? le16toh(p->dir_ent.starthi) << 16 : 0))

#define MODIFY(p,i,v)					\
  do {							\
    if (p->offset) {					\
	p->dir_ent.i = v;				\
	fs_write(p->offset+offsetof(DIR_ENT,i),		\
		 sizeof(p->dir_ent.i),&p->dir_ent.i);	\
    }							\
  } while(0)

#define MODIFY_START(p,v,fs)						\
  do {									\
    uint32_t __v = (v);							\
    if (!p->offset) {							\
	/* writing to fake entry for FAT32 root dir */			\
	if (!__v) die("Oops, deleting FAT32 root dir!");		\
	fs->root_cluster = __v;						\
	p->dir_ent.start = htole16(__v&0xffff);				\
	p->dir_ent.starthi = htole16(__v>>16);				\
	__v = htole32(__v);						\
	fs_write(offsetof(struct boot_sector,root_cluster),		\
	         sizeof(((struct boot_sector *)0)->root_cluster),	\
		 &__v);							\
    }									\
    else {								\
	MODIFY(p,start,htole16((__v)&0xffff));				\
	if (fs->fat_bits == 32)						\
	    MODIFY(p,starthi,htole16((__v)>>16));			\
    }									\
  } while(0)

off_t alloc_rootdir_entry(DOS_FS * fs, DIR_ENT * de, const char *pattern, int gen_name)
{
    static int curr_num = 0;
    off_t offset;

    if (fs->root_cluster) {
	DIR_ENT d2;
	int i = 0, got = 0;
	uint32_t clu_num, prev = 0;
	off_t offset2;

	clu_num = fs->root_cluster;
	offset = cluster_start(fs, clu_num);
	while (clu_num > 0 && clu_num != -1) {
	    fs_read(offset, sizeof(DIR_ENT), &d2);
	    if (IS_FREE(d2.name) && d2.attr != VFAT_LN_ATTR) {
		got = 1;
		break;
	    }
	    i += sizeof(DIR_ENT);
	    offset += sizeof(DIR_ENT);
	    if ((i % fs->cluster_size) == 0) {
		prev = clu_num;
		if ((clu_num = next_cluster(fs, clu_num)) == 0 || clu_num == -1)
		    break;
		offset = cluster_start(fs, clu_num);
	    }
	}
	if (!got) {
	    /* no free slot, need to extend root dir: alloc next free cluster
	     * after previous one */
	    if (!prev)
		die("Root directory has no cluster allocated!");
	    for (clu_num = prev + 1; clu_num != prev; clu_num++) {
		FAT_ENTRY entry;

		if (clu_num >= fs->data_clusters + 2)
		    clu_num = 2;
		get_fat(&entry, fs->fat, clu_num, fs);
		if (!entry.value)
		    break;
	    }
	    if (clu_num == prev)
		die("Root directory full and no free cluster");
	    set_fat(fs, prev, clu_num);
	    set_fat(fs, clu_num, -1);
	    set_owner(fs, clu_num, get_owner(fs, fs->root_cluster));
	    /* clear new cluster */
	    memset(&d2, 0, sizeof(d2));
	    offset = cluster_start(fs, clu_num);
	    for (i = 0; i < fs->cluster_size; i += sizeof(DIR_ENT))
		fs_write(offset + i, sizeof(d2), &d2);
	}
	memset(de, 0, sizeof(DIR_ENT));
	if (gen_name) {
	    while (1) {
		char expanded[12];
		sprintf(expanded, pattern, curr_num);
		memcpy(de->name, expanded, MSDOS_NAME);
		clu_num = fs->root_cluster;
		i = 0;
		offset2 = cluster_start(fs, clu_num);
		while (clu_num > 0 && clu_num != -1) {
		    fs_read(offset2, sizeof(DIR_ENT), &d2);
		    if (offset2 != offset &&
			!strncmp((const char *)d2.name, (const char *)de->name,
				 MSDOS_NAME))
			break;
		    i += sizeof(DIR_ENT);
		    offset2 += sizeof(DIR_ENT);
		    if ((i % fs->cluster_size) == 0) {
			if ((clu_num = next_cluster(fs, clu_num)) == 0 ||
			    clu_num == -1)
			    break;
			offset2 = cluster_start(fs, clu_num);
		    }
		}
		if (clu_num == 0 || clu_num == -1)
		    break;
		if (++curr_num >= 10000)
		    die("Unable to create unique name");
	    }
	} else {
	    memcpy(de->name, pattern, MSDOS_NAME);
	}
    } else {
	DIR_ENT *root;
	int next_free = 0, scan;

	root = alloc(fs->root_entries * sizeof(DIR_ENT));
	fs_read(fs->root_start, fs->root_entries * sizeof(DIR_ENT), root);

	while (next_free < fs->root_entries)
	    if (IS_FREE(root[next_free].name) &&
		root[next_free].attr != VFAT_LN_ATTR)
		break;
	    else
		next_free++;
	if (next_free == fs->root_entries)
	    die("Root directory is full.");
	offset = fs->root_start + next_free * sizeof(DIR_ENT);
	memset(de, 0, sizeof(DIR_ENT));
	if (gen_name) {
	    while (1) {
		char expanded[12];
		sprintf(expanded, pattern, curr_num);
		memcpy(de->name, expanded, MSDOS_NAME);
		for (scan = 0; scan < fs->root_entries; scan++)
		    if (scan != next_free &&
			!strncmp((const char *)root[scan].name,
				 (const char *)de->name, MSDOS_NAME))
			break;
		if (scan == fs->root_entries)
		    break;
		if (++curr_num >= 10000)
		    die("Unable to create unique name");
	    }
	} else {
	    memcpy(de->name, pattern, MSDOS_NAME);
	}
	free(root);
    }
    ++n_files;
    return offset;
}

/**
 * Construct a full path (starting with '/') for the specified dentry,
 * relative to the partition. All components are "long" names where possible.
 *
 * @param[in]   file    Information about dentry (file or directory) of interest
 *
 * return       Pointer to static string containing file's full path
 */
static char *path_name(DOS_FILE * file)
{
    static char path[PATH_NAME_MAX * 2];

    if (!file)
	*path = 0;		/* Reached the root directory */
    else {
	if (strlen(path_name(file->parent)) > PATH_NAME_MAX)
	    die("Path name too long.");
	if (strcmp(path, "/") != 0)
	    strcat(path, "/");

	/* Append the long name to the path,
	 * or the short name if there isn't a long one
	 */
	strcpy(strrchr(path, 0),
	       file->lfn ? file->lfn : file_name(file->dir_ent.name));
    }
    return path;
}

static const int day_n[] =
    {   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 0, 0, 0, 0 };
/*    Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec              */

/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */

static time_t date_dos2unix(unsigned short time, unsigned short date)
{
    int month, year;
    time_t secs;

    month = ((date >> 5) & 15) - 1;
    if (month < 0) {
	/* make sure that nothing bad happens if the month bits were zero */
	month = 0;
    }
    year = date >> 9;
    secs =
	(time & 31) * 2 + 60 * ((time >> 5) & 63) + (time >> 11) * 3600 +
	86400 * ((date & 31) - 1 + day_n[month] + (year / 4) + year * 365 -
		 ((year & 3) == 0 && month < 2 ? 1 : 0) + 3653);
    /* days since 1.1.70 plus 80's leap day */
    return secs;
}

#ifdef __REACTOS__ // Old version!

static char *file_stat(DOS_FILE * file)
{
    static char temp[100];
    char tmp[100];
    time_t date;
    LARGE_INTEGER time;
    TIME_FIELDS time_fields;

    date = date_dos2unix(le16toh(file->dir_ent.time), le16toh(file->dir_ent.date));

    RtlSecondsSince1970ToTime(date, &time);
    RtlTimeToTimeFields(&time, &time_fields);

    _snprintf(tmp, sizeof(tmp), "%d:%d:%d %d.%d.%d",
        time_fields.Hour, time_fields.Minute, time_fields.Second,
        time_fields.Day, time_fields.Month, time_fields.Year);

    _snprintf(temp, sizeof(temp), "  Size %u bytes, date %s", le32toh(file->dir_ent.size), tmp);
    return temp;
}

#else

static char *file_stat(DOS_FILE * file)
{
    static char temp[100];
    struct tm *tm;
    char tmp[100];
    time_t date;

    date =
	date_dos2unix(le16toh(file->dir_ent.time), le16toh(file->dir_ent.date));
    tm = localtime(&date);
    strftime(tmp, 99, "%H:%M:%S %b %d %Y", tm);
    sprintf(temp, "  Size %u bytes, date %s", le32toh(file->dir_ent.size), tmp);
    return temp;
}

#endif

static int bad_name(DOS_FILE * file)
{
    int i, spc, suspicious = 0;
    const char *bad_chars = atari_format ? "*?\\/:" : "*?<>|\"\\/:";
    const unsigned char *name = file->dir_ent.name;
    const unsigned char *ext = name + 8;

    /* Do not complain about (and auto-correct) the extended attribute files
     * of OS/2. */
    if (strncmp((const char *)name, "EA DATA  SF", 11) == 0 ||
	strncmp((const char *)name, "WP ROOT  SF", 11) == 0)
	return 0;

    /* check if we have neither a long filename nor a short name */
    if ((file->lfn == NULL) && (file->dir_ent.lcase & FAT_NO_83NAME)) {
	return 1;
    }

    /* don't complain about the dummy 11 bytes used by patched Linux
       kernels */
    if (file->dir_ent.lcase & FAT_NO_83NAME)
	return 0;

    for (i = 0; i < MSDOS_NAME; i++) {
	if (name[i] < ' ' || name[i] == 0x7f)
	    return 1;
	if (name[i] > 0x7f)
	    ++suspicious;
	if (strchr(bad_chars, name[i]))
	    return 1;
    }

    spc = 0;
    for (i = 0; i < 8; i++) {
	if (name[i] == ' ')
	    spc = 1;
	else if (spc)
	    /* non-space after a space not allowed, space terminates the name
	     * part */
	    return 1;
    }

    spc = 0;
    for (i = 0; i < 3; i++) {
	if (ext[i] == ' ')
	    spc = 1;
	else if (spc)
	    /* non-space after a space not allowed, space terminates the ext
	     * part */
	    return 1;
    }

    /* Under GEMDOS, chars >= 128 are never allowed. */
    if (atari_format && suspicious)
	return 1;

#ifdef __REACTOS__ // Old !!!!!!!!!!!!!!!

    /* Only complain about too much suspicious chars in interactive mode,
     * never correct them automatically. The chars are all basically ok, so we
     * shouldn't auto-correct such names. */
    if (interactive && suspicious > 6)
	return 1;
    return 0;

#else

    /* Under MS-DOS and Windows, chars >= 128 in short names are valid
     * (but these characters can be visualised differently depending on
     * local codepage: CP437, CP866, etc). The chars are all basically ok,
     * so we shouldn't auto-correct such names. */
    return 0;

#endif
}

static void lfn_remove(off_t from, off_t to)
{
    DIR_ENT empty;

    /* New dir entry is zeroed except first byte, which is set to 0xe5.
     * This is to avoid that some FAT-reading OSes (not Linux! ;) stop reading
     * a directory at the first zero entry...
     */
    memset(&empty, 0, sizeof(empty));
    empty.name[0] = DELETED_FLAG;

    for (; from < to; from += sizeof(empty)) {
	fs_write(from, sizeof(DIR_ENT), &empty);
    }
}

static void drop_file(DOS_FS * fs, DOS_FILE * file)
{
    uint32_t cluster;

    MODIFY(file, name[0], DELETED_FLAG);
    if (file->lfn)
	lfn_remove(file->lfn_offset, file->offset);
    for (cluster = FSTART(file, fs); cluster > 0 && cluster <
	 fs->data_clusters + 2; cluster = next_cluster(fs, cluster))
	set_owner(fs, cluster, NULL);
    --n_files;
}

static void truncate_file(DOS_FS * fs, DOS_FILE * file, uint32_t clusters)
{
    int deleting;
    uint32_t walk, next;

    walk = FSTART(file, fs);
    if ((deleting = !clusters))
	MODIFY_START(file, 0, fs);
    while (walk > 0 && walk != -1) {
	next = next_cluster(fs, walk);
	if (deleting)
	    set_fat(fs, walk, 0);
	else if ((deleting = !--clusters))
	    set_fat(fs, walk, -1);
	walk = next;
    }
}

static void auto_rename(DOS_FILE * file)
{
    DOS_FILE *first, *walk;
    uint32_t number;

    if (!file->offset)
	return;			/* cannot rename FAT32 root dir */
    first = file->parent ? file->parent->first : root;
    number = 0;
    while (1) {
	char num[8];
	sprintf(num, "%07lu", (unsigned long)number);
	memcpy(file->dir_ent.name, "FSCK", 4);
	memcpy(file->dir_ent.name + 4, num, 7);
	for (walk = first; walk; walk = walk->next)
	    if (walk != file
		&& !strncmp((const char *)walk->dir_ent.name,
			    (const char *)file->dir_ent.name, MSDOS_NAME))
		break;
	if (!walk) {
	    if (file->dir_ent.lcase & FAT_NO_83NAME) {
		/* as we only assign a new 8.3 filename, reset flag that 8.3 name is not
		   present */
		file->dir_ent.lcase &= ~FAT_NO_83NAME;
		/* reset the attributes, only keep DIR and VOLUME */
		file->dir_ent.attr &= ~(ATTR_DIR | ATTR_VOLUME);
		fs_write(file->offset, MSDOS_NAME + 2, &file->dir_ent);
	    } else {
		fs_write(file->offset, MSDOS_NAME, file->dir_ent.name);
	    }
	    if (file->lfn)
		lfn_fix_checksum(file->lfn_offset, file->offset,
				 (const char *)file->dir_ent.name);
	    return;
	}
	number++;
	if (number > 9999999) {
	    die("Too many files need repair.");
	}
    }
    die("Can't generate a unique name.");
}

static void rename_file(DOS_FILE * file)
{
#ifndef __REACTOS__
    unsigned char name[46];
    unsigned char *walk, *here;
#endif

    if (!file->offset) {
#ifndef __REACTOS__
	printf("Cannot rename FAT32 root dir\n");
#else
	VfatPrint( "Cannot rename FAT32 root dir\n" );
#endif
	return;			/* cannot rename FAT32 root dir */
    }
    while (1) {
#ifndef __REACTOS__
	printf("New name: ");
	fflush(stdout);
	if (fgets((char *)name, 45, stdin)) {
	    if ((here = (unsigned char *)strchr((const char *)name, '\n')))
		*here = 0;
	    for (walk = (unsigned char *)strrchr((const char *)name, 0);
		 walk >= name && (*walk == ' ' || *walk == '\t'); walk--) ;
	    walk[1] = 0;
	    for (walk = name; *walk == ' ' || *walk == '\t'; walk++) ;
	    if (file_cvt(walk, file->dir_ent.name)) {
		if (file->dir_ent.lcase & FAT_NO_83NAME) {
		    /* as we only assign a new 8.3 filename, reset flag that 8.3 name is not
		       present */
		    file->dir_ent.lcase &= ~FAT_NO_83NAME;
		    /* reset the attributes, only keep DIR and VOLUME */
		    file->dir_ent.attr &= ~(ATTR_DIR | ATTR_VOLUME);
		    fs_write(file->offset, MSDOS_NAME + 2, &file->dir_ent);
		} else {
		    fs_write(file->offset, MSDOS_NAME, file->dir_ent.name);
		}
		if (file->lfn)
		    lfn_fix_checksum(file->lfn_offset, file->offset,
				     (const char *)file->dir_ent.name);
		return;
	    }
	}
#else
    return;
#endif
    }
}

static int handle_dot(DOS_FS * fs, DOS_FILE * file, int dots)
{
    const char *name;

    name =
	strncmp((const char *)file->dir_ent.name, MSDOS_DOT,
		MSDOS_NAME) ? ".." : ".";
    if (!(file->dir_ent.attr & ATTR_DIR)) {
	printf("%s\n  Is a non-directory.\n", path_name(file));
	if (interactive)
	    printf("1) Drop it\n2) Auto-rename\n3) Rename\n"
		   "4) Convert to directory\n");
#ifndef __REACTOS__
	else
#else
	else if (rw)
#endif
	    printf("  Auto-renaming it.\n");
#ifdef __REACTOS__
	if (rw || interactive) {
#endif
	switch (interactive ? get_key("1234", "?") : '2') {
	case '1':
	    drop_file(fs, file);
	    return 1;
	case '2':
	    auto_rename(file);
	    printf("  Renamed to %s\n", file_name(file->dir_ent.name));
	    return 0;
	case '3':
	    rename_file(file);
	    return 0;
	case '4':
	    MODIFY(file, size, htole32(0));
	    MODIFY(file, attr, file->dir_ent.attr | ATTR_DIR);
	    break;
#ifdef __REACTOS__
		}
#endif
	}
    }
    if (!dots) {
	printf("Root contains directory \"%s\". Dropping it.\n", name);
	drop_file(fs, file);
	return 1;
    }
    return 0;
}

static int check_file(DOS_FS * fs, DOS_FILE * file)
{
    DOS_FILE *owner;
    int restart;
    uint32_t expect, curr, this, clusters, prev, walk, clusters2;

    if (file->dir_ent.attr & ATTR_DIR) {
	if (le32toh(file->dir_ent.size)) {
#ifndef __REACTOS__
	    printf("%s\n  Directory has non-zero size. Fixing it.\n",
		   path_name(file));
#else
	    printf("%s\n  Directory has non-zero size.%s\n",
		   path_name(file), (rw) ? " Fixing it." : "");
	    if (rw)
#endif
	    MODIFY(file, size, htole32(0));
	}
	if (file->parent
	    && !strncmp((const char *)file->dir_ent.name, MSDOS_DOT,
			MSDOS_NAME)) {
	    expect = FSTART(file->parent, fs);
	    if (FSTART(file, fs) != expect) {
		printf("%s\n  Start (%lu) does not point to parent (%lu)\n",
		       path_name(file), (unsigned long)FSTART(file, fs), (long)expect);
#ifdef __REACTOS__
		if (rw)
#endif
		MODIFY_START(file, expect, fs);
	    }
	    return 0;
	}
	if (file->parent
	    && !strncmp((const char *)file->dir_ent.name, MSDOS_DOTDOT,
			MSDOS_NAME)) {
	    expect =
		file->parent->parent ? FSTART(file->parent->parent, fs) : 0;
	    if (fs->root_cluster && expect == fs->root_cluster)
		expect = 0;
	    if (FSTART(file, fs) != expect) {
		printf("%s\n  Start (%lu) does not point to .. (%lu)\n",
		       path_name(file), (unsigned long)FSTART(file, fs), (unsigned long)expect);
#ifdef __REACTOS__
		if (rw)
#endif
		MODIFY_START(file, expect, fs);
	    }
	    return 0;
	}
	if (FSTART(file, fs) == 0) {
#ifndef __REACTOS__
	    printf("%s\n Start does point to root directory. Deleting dir. \n",
		   path_name(file));
#else
	    printf("%s\n Start does point to root directory.%s\n",
		   path_name(file), (rw) ? " Deleting dir. " : "");
	    if (rw)
#endif
	    MODIFY(file, name[0], DELETED_FLAG);
	    return 0;
	}
    }
    if (FSTART(file, fs) == 1) {
	printf("%s\n  Bad start cluster 1. Truncating file.\n",
	       path_name(file));
	if (!file->offset)
	    die("Bad FAT32 root directory! (bad start cluster 1)\n");
#ifdef __REACTOS__
	if (rw)
#endif
	MODIFY_START(file, 0, fs);
    }
    if (FSTART(file, fs) >= fs->data_clusters + 2) {
	printf
#ifndef __REACTOS__
	    ("%s\n  Start cluster beyond limit (%lu > %lu). Truncating file.\n",
	     path_name(file), (unsigned long)FSTART(file, fs),
	     (unsigned long)(fs->data_clusters + 1));
#else
	    ("%s\n  Start cluster beyond limit (%lu > %lu).%s\n",
	     path_name(file), (unsigned long)FSTART(file, fs),
	     (unsigned long)(fs->data_clusters + 1),
	     (rw) ? " Truncating file." : "");
#endif
	if (!file->offset)
	    die("Bad FAT32 root directory! (start cluster beyond limit: %lu > %lu)\n",
		(unsigned long)FSTART(file, fs),
		(unsigned long)(fs->data_clusters + 1));
#ifdef __REACTOS__
	if (rw)
#endif
	MODIFY_START(file, 0, fs);
    }
    clusters = prev = 0;
    for (curr = FSTART(file, fs) ? FSTART(file, fs) :
	 -1; curr != -1; curr = next_cluster(fs, curr)) {
	FAT_ENTRY curEntry;
	get_fat(&curEntry, fs->fat, curr, fs);

	if (!curEntry.value || bad_cluster(fs, curr)) {
#ifndef __REACTOS__
	    printf("%s\n  Contains a %s cluster (%lu). Assuming EOF.\n",
		   path_name(file), curEntry.value ? "bad" : "free", (unsigned long)curr);
#else
	    printf("%s\n  Contains a %s cluster (%lu).%s\n",
		   path_name(file), curEntry.value ? "bad" : "free", (unsigned long)curr,
		   (rw) ? " Assuming EOF." : "");
#endif
	    if (prev)
#ifdef __REACTOS__
	    {
	    if (rw)
#endif
		set_fat(fs, prev, -1);
#ifdef __REACTOS__
	    }
#endif
	    else if (!file->offset)
		die("FAT32 root dir starts with a bad cluster!");
	    else
#ifdef __REACTOS__
		if (rw)
#endif
		MODIFY_START(file, 0, fs);
	    break;
	}
	if (!(file->dir_ent.attr & ATTR_DIR) && le32toh(file->dir_ent.size) <=
	    (uint64_t)clusters * fs->cluster_size) {
#ifdef __REACTOS__
	    if (rw) {
#endif
	    printf
		("%s\n  File size is %u bytes, cluster chain length is > %llu "
		 "bytes.\n  Truncating file to %u bytes.\n", path_name(file),
		 le32toh(file->dir_ent.size),
		 (unsigned long long)clusters * fs->cluster_size,
		 le32toh(file->dir_ent.size));
	    truncate_file(fs, file, clusters);
#ifdef __REACTOS__
	    } else {
	    printf
		("%s\n  File size is %u bytes, cluster chain length is > %llu "
		 "bytes.\n", path_name(file),
		 le32toh(file->dir_ent.size),
		 (unsigned long long)clusters * fs->cluster_size);
	    }
#endif
	    break;
	}
	if ((owner = get_owner(fs, curr))) {
	    int do_trunc = 0;
	    printf("%s  and\n", path_name(owner));
	    printf("%s\n  share clusters.\n", path_name(file));
	    clusters2 = 0;
	    for (walk = FSTART(owner, fs); walk > 0 && walk != -1; walk =
		 next_cluster(fs, walk))
		if (walk == curr)
		    break;
		else
		    clusters2++;
	    restart = file->dir_ent.attr & ATTR_DIR;
#ifndef __REACTOS__
	    if (!owner->offset) {
#else
	    if (!owner->offset && rw) {
#endif
		printf("  Truncating second to %llu bytes because first "
		       "is FAT32 root dir.\n",
		       (unsigned long long)clusters * fs->cluster_size);
		do_trunc = 2;
#ifndef __REACTOS__
	    } else if (!file->offset) {
#else
	    } else if (!file->offset && rw) {
#endif
		printf("  Truncating first to %llu bytes because second "
		       "is FAT32 root dir.\n",
		       (unsigned long long)clusters2 * fs->cluster_size);
		do_trunc = 1;
	    } else if (interactive)
		printf("1) Truncate first to %llu bytes%s\n"
		       "2) Truncate second to %llu bytes\n",
		       (unsigned long long)clusters2 * fs->cluster_size,
		       restart ? " and restart" : "",
		       (unsigned long long)clusters * fs->cluster_size);
	    else
#ifdef __REACTOS__
	    if (rw)
#endif
		printf("  Truncating second to %llu bytes.\n",
		       (unsigned long long)clusters * fs->cluster_size);
#ifndef __REACTOS__
	    if (do_trunc != 2
		&& (do_trunc == 1
#else
	    if ((do_trunc != 2 && rw)
		&& ((do_trunc == 1 && rw)
#endif
		    || (interactive && get_key("12", "?") == '1'))) {
		prev = 0;
		clusters = 0;
		for (this = FSTART(owner, fs); this > 0 && this != -1; this =
		     next_cluster(fs, this)) {
		    if (this == curr) {
			if (prev)
			    set_fat(fs, prev, -1);
			else
			    MODIFY_START(owner, 0, fs);
			MODIFY(owner, size,
			       htole32((uint64_t)clusters *
				       fs->cluster_size));
			if (restart)
			    return 1;
			while (this > 0 && this != -1) {
			    set_owner(fs, this, NULL);
			    this = next_cluster(fs, this);
			}
			this = curr;
			break;
		    }
		    clusters++;
		    prev = this;
		}
		if (this != curr)
		    die("Internal error: didn't find cluster %d in chain"
			" starting at %d", curr, FSTART(owner, fs));
	    } else {
		if (prev)
		    set_fat(fs, prev, -1);
		else
		    MODIFY_START(file, 0, fs);
		break;
	    }
	}
	set_owner(fs, curr, file);
	clusters++;
	prev = curr;
    }
    if (!(file->dir_ent.attr & ATTR_DIR) && le32toh(file->dir_ent.size) >
#ifndef __REACTOS__
	(uint64_t)clusters * fs->cluster_size) {
#else
	(uint64_t)clusters * fs->cluster_size && rw) {
#endif
	printf
	    ("%s\n  File size is %u bytes, cluster chain length is %llu bytes."
	     "\n  Truncating file to %llu bytes.\n", path_name(file),
	     le32toh(file->dir_ent.size),
	     (unsigned long long)clusters * fs->cluster_size,
	     (unsigned long long)clusters * fs->cluster_size);
	MODIFY(file, size,
	       htole32((uint64_t)clusters * fs->cluster_size));
    }
    return 0;
}

static int check_files(DOS_FS * fs, DOS_FILE * start)
{
    while (start) {
	if (check_file(fs, start))
	    return 1;
	start = start->next;
    }
    return 0;
}

static int check_dir(DOS_FS * fs, DOS_FILE ** root, int dots)
{
    DOS_FILE *parent, **walk, **scan;
    int dot, dotdot, skip, redo;
    int good, bad;

    if (!*root)
	return 0;
    parent = (*root)->parent;
    good = bad = 0;
    for (walk = root; *walk; walk = &(*walk)->next)
	if (bad_name(*walk))
	    bad++;
	else
	    good++;
    if (*root && parent && good + bad > 4 && bad > good / 2) {
	printf("%s\n  Has a large number of bad entries. (%d/%d)\n",
	       path_name(parent), bad, good + bad);
	if (!dots)
	    printf("  Not dropping root directory.\n");
#ifndef __REACTOS__
	else if (!interactive)
#else
	else if (!interactive || !rw)
#endif
	    printf("  Not dropping it in auto-mode.\n");
	else if (get_key("yn", "Drop directory ? (y/n)") == 'y') {
	    truncate_file(fs, parent, 0);
	    MODIFY(parent, name[0], DELETED_FLAG);
	    /* buglet: deleted directory stays in the list. */
	    return 1;
	}
    }
    dot = dotdot = redo = 0;
    walk = root;
    while (*walk) {
	if (!strncmp
	    ((const char *)((*walk)->dir_ent.name), MSDOS_DOT, MSDOS_NAME)
	    || !strncmp((const char *)((*walk)->dir_ent.name), MSDOS_DOTDOT,
			MSDOS_NAME)) {
	    if (handle_dot(fs, *walk, dots)) {
		*walk = (*walk)->next;
		continue;
	    }
	    if (!strncmp
		((const char *)((*walk)->dir_ent.name), MSDOS_DOT, MSDOS_NAME))
		dot++;
	    else
		dotdot++;
	}
	if (!((*walk)->dir_ent.attr & ATTR_VOLUME) && bad_name(*walk)) {
#ifndef __REACTOS__
	    puts(path_name(*walk));
	    printf("  Bad short file name (%s).\n",
		   file_name((*walk)->dir_ent.name));
#else
	    printf("%s\n  Bad short file name (%s).\n",
		   path_name(*walk), file_name((*walk)->dir_ent.name));
#endif
	    if (interactive)
		printf("1) Drop file\n2) Rename file\n3) Auto-rename\n"
		       "4) Keep it\n");
	    else
#ifdef __REACTOS__
	    if (rw)
#endif
		printf("  Auto-renaming it.\n");
#ifdef __REACTOS__
	    if (rw || interactive) {
#endif
	    switch (interactive ? get_key("1234", "?") : '3') {
	    case '1':
		drop_file(fs, *walk);
		walk = &(*walk)->next;
		continue;
	    case '2':
		rename_file(*walk);
		redo = 1;
		break;
	    case '3':
		auto_rename(*walk);
		printf("  Renamed to %s\n", file_name((*walk)->dir_ent.name));
		break;
	    case '4':
		break;
#ifdef __REACTOS__
		    }
#endif
	    }
	}
	/* don't check for duplicates of the volume label */
	if (!((*walk)->dir_ent.attr & ATTR_VOLUME)) {
	    scan = &(*walk)->next;
	    skip = 0;
	    while (*scan && !skip) {
		if (!((*scan)->dir_ent.attr & ATTR_VOLUME) &&
		    !memcmp((*walk)->dir_ent.name, (*scan)->dir_ent.name,
			    MSDOS_NAME)) {
		    printf("%s\n  Duplicate directory entry.\n  First  %s\n",
			   path_name(*walk), file_stat(*walk));
		    printf("  Second %s\n", file_stat(*scan));
		    if (interactive)
			printf
			    ("1) Drop first\n2) Drop second\n3) Rename first\n"
			     "4) Rename second\n5) Auto-rename first\n"
			     "6) Auto-rename second\n");
		    else
#ifdef __REACTOS__
		    if (rw)
#endif
			printf("  Auto-renaming second.\n");
#ifdef __REACTOS__
		    if (rw || interactive) {
#endif
		    switch (interactive ? get_key("123456", "?") : '6') {
		    case '1':
			drop_file(fs, *walk);
			*walk = (*walk)->next;
			skip = 1;
			break;
		    case '2':
			drop_file(fs, *scan);
			*scan = (*scan)->next;
			continue;
		    case '3':
			rename_file(*walk);
			printf("  Renamed to %s\n", path_name(*walk));
			redo = 1;
			break;
		    case '4':
			rename_file(*scan);
			printf("  Renamed to %s\n", path_name(*walk));
			redo = 1;
			break;
		    case '5':
			auto_rename(*walk);
			printf("  Renamed to %s\n",
			       file_name((*walk)->dir_ent.name));
			break;
		    case '6':
			auto_rename(*scan);
			printf("  Renamed to %s\n",
			       file_name((*scan)->dir_ent.name));
			break;
#ifdef __REACTOS__
			    }
#endif
		    }
		}
		scan = &(*scan)->next;
	    }
	    if (skip)
		continue;
	}
	if (!redo)
	    walk = &(*walk)->next;
	else {
	    walk = root;
	    dot = dotdot = redo = 0;
	}
    }
    if (dots && !dot)
	printf("%s\n  \".\" is missing. Can't fix this yet.\n",
	       path_name(parent));
    if (dots && !dotdot)
	printf("%s\n  \"..\" is missing. Can't fix this yet.\n",
	       path_name(parent));
    return 0;
}

/**
 * Check a dentry's cluster chain for bad clusters.
 * If requested, we verify readability and mark unreadable clusters as bad.
 *
 * @param[inout]    fs          Information about the filesystem
 * @param[in]       file        dentry to check
 * @param[in]       read_test   Nonzero == verify that dentry's clusters can
 *                              be read
 */
static void test_file(DOS_FS * fs, DOS_FILE * file, int read_test)
{
    DOS_FILE *owner;
    uint32_t walk, prev, clusters, next_clu;

    prev = clusters = 0;
    for (walk = FSTART(file, fs); walk > 1 && walk < fs->data_clusters + 2;
	 walk = next_clu) {
	next_clu = next_cluster(fs, walk);

	/* In this stage we are checking only for a loop within our own
	 * cluster chain.
	 * Cross-linking of clusters is handled in check_file()
	 */
	if ((owner = get_owner(fs, walk))) {
	    if (owner == file) {
		printf("%s\n  Circular cluster chain. Truncating to %lu "
		       "cluster%s.\n", path_name(file), (unsigned long)clusters,
		       clusters == 1 ? "" : "s");
		if (prev)
		    set_fat(fs, prev, -1);
		else if (!file->offset)
		    die("Bad FAT32 root directory! (bad start cluster)\n");
		else
		    MODIFY_START(file, 0, fs);
	    }
	    break;
	}
	if (bad_cluster(fs, walk))
	    break;
	if (read_test) {
	    if (fs_test(cluster_start(fs, walk), fs->cluster_size)) {
		prev = walk;
		clusters++;
	    } else {
		printf("%s\n  Cluster %lu (%lu) is unreadable. Skipping it.\n",
		       path_name(file), (unsigned long)clusters, (unsigned long)walk);
		if (prev)
		    set_fat(fs, prev, next_cluster(fs, walk));
		else
		    MODIFY_START(file, next_cluster(fs, walk), fs);
		set_fat(fs, walk, -2);
	    }
	} else {
	    prev = walk;
	    clusters++;
	}
	set_owner(fs, walk, file);
    }
    /* Revert ownership (for now) */
    for (walk = FSTART(file, fs); walk > 1 && walk < fs->data_clusters + 2;
	 walk = next_cluster(fs, walk))
	if (bad_cluster(fs, walk))
	    break;
	else if (get_owner(fs, walk) == file)
	    set_owner(fs, walk, NULL);
	else
	    break;
}

static void undelete(DOS_FS * fs, DOS_FILE * file)
{
    uint32_t clusters, left, prev, walk;

    clusters = left = (le32toh(file->dir_ent.size) + fs->cluster_size - 1) /
	fs->cluster_size;
    prev = 0;

    walk = FSTART(file, fs);

    while (left && (walk >= 2) && (walk < fs->data_clusters + 2)) {

	FAT_ENTRY curEntry;
	get_fat(&curEntry, fs->fat, walk, fs);

	if (!curEntry.value)
	    break;

	left--;
	if (prev)
	    set_fat(fs, prev, walk);
	prev = walk;
	walk++;
    }
    if (prev)
	set_fat(fs, prev, -1);
    else
	MODIFY_START(file, 0, fs);
    if (left)
	printf("Warning: Did only undelete %lu of %lu cluster%s.\n",
	       (unsigned long)clusters - left, (unsigned long)clusters, clusters == 1 ? "" : "s");

}

static void new_dir(void)
{
    lfn_reset();
}

/**
 * Create a description for a referenced dentry and insert it in our dentry
 * tree. Then, go check the dentry's cluster chain for bad clusters and
 * cluster loops.
 *
 * @param[inout]    fs      Information about the filesystem
 * @param[out]      chain
 * @param[in]       parent  Information about parent directory of this file
 *                          NULL == no parent ('file' is root directory)
 * @param[in]       offset  Partition-relative byte offset of directory entry of interest
 *                          0 == Root directory
 * @param           cp
 */
static void add_file(DOS_FS * fs, DOS_FILE *** chain, DOS_FILE * parent,
		     off_t offset, FDSC ** cp)
{
    DOS_FILE *new;
    DIR_ENT de;
    FD_TYPE type;

    if (offset)
	fs_read(offset, sizeof(DIR_ENT), &de);
    else {
	/* Construct a DIR_ENT for the root directory */
	memset(&de, 0, sizeof de);
	memcpy(de.name, "           ", MSDOS_NAME);
	de.attr = ATTR_DIR;
	de.start = htole16(fs->root_cluster & 0xffff);
	de.starthi = htole16((fs->root_cluster >> 16) & 0xffff);
    }
    if ((type = file_type(cp, (char *)de.name)) != fdt_none) {
	if (type == fdt_undelete && (de.attr & ATTR_DIR))
	    die("Can't undelete directories.");
	file_modify(cp, (char *)de.name);
	fs_write(offset, 1, &de);
    }
    if (IS_FREE(de.name)) {
	lfn_check_orphaned();
	return;
    }
    if (de.attr == VFAT_LN_ATTR) {
	lfn_add_slot(&de, offset);
	return;
    }
    new = qalloc(&mem_queue, sizeof(DOS_FILE));
    new->lfn = lfn_get(&de, &new->lfn_offset);
    new->offset = offset;
    memcpy(&new->dir_ent, &de, sizeof(de));
    new->next = new->first = NULL;
    new->parent = parent;
    if (type == fdt_undelete)
	undelete(fs, new);
    **chain = new;
    *chain = &new->next;
    if (list) {
	printf("Checking file %s", path_name(new));
	if (new->lfn)
	    printf(" (%s)", file_name(new->dir_ent.name));	/* (8.3) */
	printf("\n");
    }
    /* Don't include root directory, '.', or '..' in the total file count */
    if (offset &&
	strncmp((const char *)de.name, MSDOS_DOT, MSDOS_NAME) != 0 &&
	strncmp((const char *)de.name, MSDOS_DOTDOT, MSDOS_NAME) != 0)
	++n_files;
    test_file(fs, new, test);	/* Bad cluster check */
}

static int subdirs(DOS_FS * fs, DOS_FILE * parent, FDSC ** cp);

static int scan_dir(DOS_FS * fs, DOS_FILE * this, FDSC ** cp)
{
    DOS_FILE **chain;
    int i;
    uint32_t clu_num;

    chain = &this->first;
    i = 0;
    clu_num = FSTART(this, fs);
    new_dir();
    while (clu_num > 0 && clu_num != -1) {
	add_file(fs, &chain, this,
		 cluster_start(fs, clu_num) + (i % fs->cluster_size), cp);
	i += sizeof(DIR_ENT);
	if (!(i % fs->cluster_size))
	    if ((clu_num = next_cluster(fs, clu_num)) == 0 || clu_num == -1)
		break;
    }
    lfn_check_orphaned();
    if (check_dir(fs, &this->first, this->offset))
	return 0;
    if (check_files(fs, this->first))
	return 1;
    return subdirs(fs, this, cp);
}

/**
 * Recursively scan subdirectories of the specified parent directory.
 *
 * @param[inout]    fs      Information about the filesystem
 * @param[in]       parent  Identifies the directory to scan
 * @param[in]       cp
 *
 * @return  0   Success
 * @return  1   Error
 */
static int subdirs(DOS_FS * fs, DOS_FILE * parent, FDSC ** cp)
{
    DOS_FILE *walk;

    for (walk = parent ? parent->first : root; walk; walk = walk->next)
	if (walk->dir_ent.attr & ATTR_DIR)
	    if (strncmp((const char *)walk->dir_ent.name, MSDOS_DOT, MSDOS_NAME)
		&& strncmp((const char *)walk->dir_ent.name, MSDOS_DOTDOT,
			   MSDOS_NAME))
		if (scan_dir(fs, walk, file_cd(cp, (char *)walk->dir_ent.name)))
		    return 1;
    return 0;
}

/**
 * Scan all directory and file information for errors.
 *
 * @param[inout]    fs      Information about the filesystem
 *
 * @return  0   Success
 * @return  1   Error
 */
int scan_root(DOS_FS * fs)
{
    DOS_FILE **chain;
    int i;

    root = NULL;
    chain = &root;
    new_dir();
    if (fs->root_cluster) {
	add_file(fs, &chain, NULL, 0, &fp_root);
    } else {
	for (i = 0; i < fs->root_entries; i++)
	    add_file(fs, &chain, NULL, fs->root_start + i * sizeof(DIR_ENT),
		     &fp_root);
    }
    lfn_check_orphaned();
    (void)check_dir(fs, &root, 0);
    if (check_files(fs, root))
	return 1;
    return subdirs(fs, NULL, &fp_root);
}
