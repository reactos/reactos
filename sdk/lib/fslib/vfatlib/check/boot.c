/* boot.c - Read and analyze ia PC/MS-DOS boot sector

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
#ifdef __REACTOS__
#include <ntstrsafe.h>
#endif // __REACTOS__

#define NDEBUG
#include <debug.h>

#define ROUND_TO_MULTIPLE(n,m) ((n) && (m) ? (n)+(m)-1-((n)-1)%(m) : 0)
    /* don't divide by zero */

/* cut-over cluster counts for FAT12 and FAT16 */
#define FAT12_THRESHOLD  4085
#define FAT16_THRESHOLD 65525

static struct {
    uint8_t media;
    const char *descr;
} mediabytes[] = {
    {
    0xf0, "5.25\" or 3.5\" HD floppy"}, {
    0xf8, "hard disk"}, {
    0xf9, "3,5\" 720k floppy 2s/80tr/9sec or "
	    "5.25\" 1.2M floppy 2s/80tr/15sec"}, {
    0xfa, "5.25\" 320k floppy 1s/80tr/8sec"}, {
    0xfb, "3.5\" 640k floppy 2s/80tr/8sec"}, {
    0xfc, "5.25\" 180k floppy 1s/40tr/9sec"}, {
    0xfd, "5.25\" 360k floppy 2s/40tr/9sec"}, {
    0xfe, "5.25\" 160k floppy 1s/40tr/8sec"}, {
0xff, "5.25\" 320k floppy 2s/40tr/8sec"},};

/* Unaligned fields must first be accessed byte-wise */
#define GET_UNALIGNED_W(f)			\
    ( (uint16_t)f[0] | ((uint16_t)f[1]<<8) )

static const char *get_media_descr(unsigned char media)
{
    int i;

    for (i = 0; i < sizeof(mediabytes) / sizeof(*mediabytes); ++i) {
	if (mediabytes[i].media == media)
	    return (mediabytes[i].descr);
    }
    return ("undefined");
}

static void dump_boot(DOS_FS * fs, struct boot_sector *b, unsigned lss)
{
    unsigned short sectors;

    printf("Boot sector contents:\n");
    if (!atari_format) {
	char id[9];
	strncpy(id, (const char *)b->system_id, 8);
	id[8] = 0;
	printf("System ID \"%s\"\n", id);
    } else {
	/* On Atari, a 24 bit serial number is stored at offset 8 of the boot
	 * sector */
	printf("Serial number 0x%x\n",
	       b->system_id[5] | (b->system_id[6] << 8) | (b->
							   system_id[7] << 16));
    }
    printf("Media byte 0x%02x (%s)\n", b->media, get_media_descr(b->media));
    printf("%10d bytes per logical sector\n", GET_UNALIGNED_W(b->sector_size));
    printf("%10d bytes per cluster\n", fs->cluster_size);
    printf("%10d reserved sector%s\n", le16toh(b->reserved),
	   le16toh(b->reserved) == 1 ? "" : "s");
    printf("First FAT starts at byte %llu (sector %llu)\n",
	   (unsigned long long)fs->fat_start,
	   (unsigned long long)fs->fat_start / lss);
    printf("%10d FATs, %d bit entries\n", b->fats, fs->fat_bits);
    printf("%10lld bytes per FAT (= %llu sectors)\n", (long long)fs->fat_size,
	   (long long)fs->fat_size / lss);
    if (!fs->root_cluster) {
	printf("Root directory starts at byte %llu (sector %llu)\n",
	       (unsigned long long)fs->root_start,
	       (unsigned long long)fs->root_start / lss);
	printf("%10d root directory entries\n", fs->root_entries);
    } else {
	printf("Root directory start at cluster %lu (arbitrary size)\n",
	       (unsigned long)fs->root_cluster);
    }
    printf("Data area starts at byte %llu (sector %llu)\n",
	   (unsigned long long)fs->data_start,
	   (unsigned long long)fs->data_start / lss);
    printf("%10lu data clusters (%llu bytes)\n",
	   (unsigned long)fs->data_clusters,
	   (unsigned long long)fs->data_clusters * fs->cluster_size);
    printf("%u sectors/track, %u heads\n", le16toh(b->secs_track),
	   le16toh(b->heads));
    printf("%10u hidden sectors\n", atari_format ?
	   /* On Atari, the hidden field is only 16 bit wide and unused */
	   (((unsigned char *)&b->hidden)[0] |
	    ((unsigned char *)&b->hidden)[1] << 8) : le32toh(b->hidden));
    sectors = GET_UNALIGNED_W(b->sectors);
    printf("%10u sectors total\n", sectors ? sectors : le32toh(b->total_sect));
}

static void check_backup_boot(DOS_FS * fs, struct boot_sector *b, unsigned int lss)
{
    struct boot_sector b2;

    if (!fs->backupboot_start) {
	printf("There is no backup boot sector.\n");
	if (le16toh(b->reserved) < 3) {
	    printf("And there is no space for creating one!\n");
	    return;
	}
	if (interactive)
	    printf("1) Create one\n2) Do without a backup\n");
	else
	    printf("  Auto-creating backup boot block.\n");
	if (!interactive || get_key("12", "?") == '1') {
	    unsigned int bbs;
	    /* The usual place for the backup boot sector is sector 6. Choose
	     * that or the last reserved sector. */
	    if (le16toh(b->reserved) >= 7 && le16toh(b->info_sector) != 6)
		bbs = 6;
	    else {
		bbs = le16toh(b->reserved) - 1;
		if (bbs == le16toh(b->info_sector))
		    --bbs;	/* this is never 0, as we checked reserved >= 3! */
	    }
	    fs->backupboot_start = bbs * lss;
	    b->backup_boot = htole16(bbs);
	    fs_write(fs->backupboot_start, sizeof(*b), b);
	    fs_write(offsetof(struct boot_sector, backup_boot),
		     sizeof(b->backup_boot), &b->backup_boot);
	    printf("Created backup of boot sector in sector %d\n", bbs);
	    return;
	} else
	    return;
    }

    fs_read(fs->backupboot_start, sizeof(b2), &b2);
    if (memcmp(b, &b2, sizeof(b2)) != 0) {
	/* there are any differences */
	uint8_t *p, *q;
	int i, pos, first = 1;
	char buf[20];

	printf("There are differences between boot sector and its backup.\n");
	printf("This is mostly harmless. Differences: (offset:original/backup)\n  ");
	pos = 2;
	for (p = (uint8_t *) b, q = (uint8_t *) & b2, i = 0; i < sizeof(b2);
	     ++p, ++q, ++i) {
	    if (*p != *q) {
#ifndef __REACTOS__
		sprintf(buf, "%s%u:%02x/%02x", first ? "" : ", ",
#else
		RtlStringCbPrintfA(buf, sizeof(buf), "%s%u:%02x/%02x", first ? "" : ", ",
#endif // __REACTOS__
			(unsigned)(p - (uint8_t *) b), *p, *q);
		if (pos + strlen(buf) > 78)
		    printf("\n  "), pos = 2;
		printf("%s", buf);
		pos += strlen(buf);
		first = 0;
	    }
	}
	printf("\n");

	if (interactive)
	    printf("1) Copy original to backup\n"
		   "2) Copy backup to original\n" "3) No action\n");
	else
	    printf("  Not automatically fixing this.\n");
	switch (interactive ? get_key("123", "?") : '3') {
	case '1':
	    fs_write(fs->backupboot_start, sizeof(*b), b);
	    break;
	case '2':
	    fs_write(0, sizeof(b2), &b2);
	    break;
	default:
	    break;
	}
    }
}

static void init_fsinfo(struct info_sector *i)
{
    memset(i, 0, sizeof (struct info_sector));
    i->magic = htole32(0x41615252);
    i->signature = htole32(0x61417272);
    i->free_clusters = htole32(-1);
    i->next_cluster = htole32(2);
    i->boot_sign = htole32(0xaa550000);
}

static void read_fsinfo(DOS_FS * fs, struct boot_sector *b, unsigned int lss)
{
    struct info_sector i;

    if (!b->info_sector) {
	printf("No FSINFO sector\n");
	if (interactive)
	    printf("1) Create one\n2) Do without FSINFO\n");
	else
	    printf("  Not automatically creating it.\n");
	if (interactive && get_key("12", "?") == '1') {
	    /* search for a free reserved sector (not boot sector and not
	     * backup boot sector) */
	    uint32_t s;
	    for (s = 1; s < le16toh(b->reserved); ++s)
		if (s != le16toh(b->backup_boot))
		    break;
	    if (s > 0 && s < le16toh(b->reserved)) {
		init_fsinfo(&i);
		fs_write((off_t)s * lss, sizeof(i), &i);
		b->info_sector = htole16(s);
		fs_write(offsetof(struct boot_sector, info_sector),
			 sizeof(b->info_sector), &b->info_sector);
		if (fs->backupboot_start)
		    fs_write(fs->backupboot_start +
			     offsetof(struct boot_sector, info_sector),
			     sizeof(b->info_sector), &b->info_sector);
	    } else {
		printf("No free reserved sector found -- "
		       "no space for FSINFO sector!\n");
		return;
	    }
	} else
	    return;
    }

    fs->fsinfo_start = le16toh(b->info_sector) * lss;
    fs_read(fs->fsinfo_start, sizeof(i), &i);

    if (i.magic != htole32(0x41615252) ||
	i.signature != htole32(0x61417272) || i.boot_sign != htole32(0xaa550000)) {
	printf("FSINFO sector has bad magic number(s):\n");
	if (i.magic != htole32(0x41615252))
	    printf("  Offset %llu: 0x%08x != expected 0x%08x\n",
		   (unsigned long long)offsetof(struct info_sector, magic),
		   le32toh(i.magic), 0x41615252);
	if (i.signature != htole32(0x61417272))
	    printf("  Offset %llu: 0x%08x != expected 0x%08x\n",
		   (unsigned long long)offsetof(struct info_sector, signature),
		   le32toh(i.signature), 0x61417272);
	if (i.boot_sign != htole32(0xaa550000))
	    printf("  Offset %llu: 0x%08x != expected 0x%08x\n",
		   (unsigned long long)offsetof(struct info_sector, boot_sign),
		   le32toh(i.boot_sign), 0xaa550000);
	if (interactive)
	    printf("1) Correct\n2) Don't correct (FSINFO invalid then)\n");
	else
	    printf("  Auto-correcting it.\n");
	if (!interactive || get_key("12", "?") == '1') {
	    init_fsinfo(&i);
	    fs_write(fs->fsinfo_start, sizeof(i), &i);
	} else
	    fs->fsinfo_start = 0;
    }

    if (fs->fsinfo_start)
	fs->free_clusters = le32toh(i.free_clusters);
}

static char print_fat_dirty_state(void)
{
    printf("Dirty bit is set. Fs was not properly unmounted and"
	   " some data may be corrupt.\n");

    if (interactive) {
	printf("1) Remove dirty bit\n" "2) No action\n");
	return get_key("12", "?");
#ifndef __REACTOS__
    } else
#else
    } else if (rw) {
#endif
	printf(" Automatically removing dirty bit.\n");
    return '1';
#ifdef __REACTOS__
    }
    return '2';
#endif
}

static void check_fat_state_bit(DOS_FS * fs, void *b)
{
    if (fs->fat_bits == 32) {
	struct boot_sector *b32 = b;

	if (b32->reserved3 & FAT_STATE_DIRTY) {
	    printf("0x41: ");
	    if (print_fat_dirty_state() == '1') {
		b32->reserved3 &= ~FAT_STATE_DIRTY;
		fs_write(0, sizeof(*b32), b32);
	    }
	}
    } else {
	struct boot_sector_16 *b16 = b;

	if (b16->reserved2 & FAT_STATE_DIRTY) {
	    printf("0x25: ");
	    if (print_fat_dirty_state() == '1') {
		b16->reserved2 &= ~FAT_STATE_DIRTY;
		fs_write(0, sizeof(*b16), b16);
	    }
	}
    }
}

void read_boot(DOS_FS * fs)
{
    struct boot_sector b;
    unsigned total_sectors;
    unsigned int logical_sector_size, sectors;
    off_t fat_length;
    unsigned total_fat_entries;
    off_t data_size;

    fs_read(0, sizeof(b), &b);
    logical_sector_size = GET_UNALIGNED_W(b.sector_size);
    if (!logical_sector_size)
	die("Logical sector size is zero.");

    /* This was moved up because it's the first thing that will fail */
    /* if the platform needs special handling of unaligned multibyte accesses */
    /* but such handling isn't being provided. See GET_UNALIGNED_W() above. */
    if (logical_sector_size & (SECTOR_SIZE - 1))
	die("Logical sector size (%d bytes) is not a multiple of the physical "
	    "sector size.", logical_sector_size);

    fs->cluster_size = b.cluster_size * logical_sector_size;
    if (!fs->cluster_size)
	die("Cluster size is zero.");
    if (b.fats != 2 && b.fats != 1)
	die("Currently, only 1 or 2 FATs are supported, not %d.\n", b.fats);
    fs->nfats = b.fats;
    sectors = GET_UNALIGNED_W(b.sectors);
    total_sectors = sectors ? sectors : le32toh(b.total_sect);
    if (verbose)
	printf("Checking we can access the last sector of the filesystem\n");
    /* Can't access last odd sector anyway, so round down */
    fs_test((off_t)((total_sectors & ~1) - 1) * logical_sector_size,
	    logical_sector_size);

    fat_length = le16toh(b.fat_length) ?
	le16toh(b.fat_length) : le32toh(b.fat32_length);
    if (!fat_length)
	die("FAT size is zero.");

    fs->fat_start = (off_t)le16toh(b.reserved) * logical_sector_size;
    fs->root_start = ((off_t)le16toh(b.reserved) + b.fats * fat_length) *
	logical_sector_size;
    fs->root_entries = GET_UNALIGNED_W(b.dir_entries);
    fs->data_start = fs->root_start + ROUND_TO_MULTIPLE(fs->root_entries <<
							MSDOS_DIR_BITS,
							logical_sector_size);

    data_size = (off_t)total_sectors * logical_sector_size - fs->data_start;
    if (data_size < fs->cluster_size)
	die("Filesystem has no space for any data clusters");

    fs->data_clusters = data_size / fs->cluster_size;
    fs->root_cluster = 0;	/* indicates standard, pre-FAT32 root dir */
    fs->fsinfo_start = 0;	/* no FSINFO structure */
    fs->free_clusters = -1;	/* unknown */
    if (!b.fat_length && b.fat32_length) {
	fs->fat_bits = 32;
	fs->root_cluster = le32toh(b.root_cluster);
	if (!fs->root_cluster && fs->root_entries)
	    /* M$ hasn't specified this, but it looks reasonable: If
	     * root_cluster is 0 but there is a separate root dir
	     * (root_entries != 0), we handle the root dir the old way. Give a
	     * warning, but convertig to a root dir in a cluster chain seems
	     * to complex for now... */
	    printf("Warning: FAT32 root dir not in cluster chain! "
		   "Compatibility mode...\n");
	else if (!fs->root_cluster && !fs->root_entries)
	    die("No root directory!");
	else if (fs->root_cluster && fs->root_entries)
	    printf("Warning: FAT32 root dir is in a cluster chain, but "
		   "a separate root dir\n"
		   "  area is defined. Cannot fix this easily.\n");
	if (fs->data_clusters < FAT16_THRESHOLD)
	    printf("Warning: Filesystem is FAT32 according to fat_length "
		   "and fat32_length fields,\n"
		   "  but has only %lu clusters, less than the required "
		   "minimum of %d.\n"
		   "  This may lead to problems on some systems.\n",
		   (unsigned long)fs->data_clusters, FAT16_THRESHOLD);

	check_fat_state_bit(fs, &b);
	fs->backupboot_start = le16toh(b.backup_boot) * logical_sector_size;
	check_backup_boot(fs, &b, logical_sector_size);

	read_fsinfo(fs, &b, logical_sector_size);
    } else if (!atari_format) {
	/* On real MS-DOS, a 16 bit FAT is used whenever there would be too
	 * much clusers otherwise. */
	fs->fat_bits = (fs->data_clusters >= FAT12_THRESHOLD) ? 16 : 12;
	if (fs->data_clusters >= FAT16_THRESHOLD)
	    die("Too many clusters (%lu) for FAT16 filesystem.",
		    (unsigned long)fs->data_clusters);
	check_fat_state_bit(fs, &b);
    } else {
	/* On Atari, things are more difficult: GEMDOS always uses 12bit FATs
	 * on floppies, and always 16 bit on harddisks. */
	fs->fat_bits = 16;	/* assume 16 bit FAT for now */
	/* If more clusters than fat entries in 16-bit fat, we assume
	 * it's a real MSDOS FS with 12-bit fat. */
	if (fs->data_clusters + 2 > fat_length * logical_sector_size * 8 / 16 ||
	    /* if it has one of the usual floppy sizes -> 12bit FAT  */
	    (total_sectors == 720 || total_sectors == 1440 ||
	     total_sectors == 2880))
	    fs->fat_bits = 12;
    }
    /* On FAT32, the high 4 bits of a FAT entry are reserved */
    fs->eff_fat_bits = (fs->fat_bits == 32) ? 28 : fs->fat_bits;
    fs->fat_size = fat_length * logical_sector_size;

    fs->label = calloc(12, sizeof(uint8_t));
    if (fs->fat_bits == 12 || fs->fat_bits == 16) {
	struct boot_sector_16 *b16 = (struct boot_sector_16 *)&b;
	if (b16->extended_sig == 0x29)
	    memmove(fs->label, b16->label, 11);
	else
#ifdef __REACTOS__
	{
	    free(fs->label);
#endif
	    fs->label = NULL;
#ifdef __REACTOS__
	}
#endif
    } else if (fs->fat_bits == 32) {
	if (b.extended_sig == 0x29)
	    memmove(fs->label, &b.label, 11);
	else
#ifdef __REACTOS__
	{
	    free(fs->label);
#endif
	    fs->label = NULL;
#ifdef __REACTOS__
	}
#endif
    }

    total_fat_entries = (uint64_t)fs->fat_size * 8 / fs->fat_bits;
    if (fs->data_clusters > total_fat_entries - 2)
	die("Filesystem has %u clusters but only space for %u FAT entries.",
	    fs->data_clusters, total_fat_entries - 2);
    if (!fs->root_entries && !fs->root_cluster)
	die("Root directory has zero size.");
    if (fs->root_entries & (MSDOS_DPS - 1))
	die("Root directory (%d entries) doesn't span an integral number of "
	    "sectors.", fs->root_entries);
    if (logical_sector_size & (SECTOR_SIZE - 1))
	die("Logical sector size (%u bytes) is not a multiple of the physical "
	    "sector size.", logical_sector_size);
#if 0				/* linux kernel doesn't check that either */
    /* ++roman: On Atari, these two fields are often left uninitialized */
    if (!atari_format && (!b.secs_track || !b.heads))
	die("Invalid disk format in boot sector.");
#endif
    if (verbose)
	dump_boot(fs, &b, logical_sector_size);
}

#ifndef __REACTOS__
static void write_boot_label(DOS_FS * fs, char *label)
{
    if (fs->fat_bits == 12 || fs->fat_bits == 16) {
	struct boot_sector_16 b16;

	fs_read(0, sizeof(b16), &b16);
	if (b16.extended_sig != 0x29) {
	    b16.extended_sig = 0x29;
	    b16.serial = 0;
	    memmove(b16.fs_type, fs->fat_bits == 12 ? "FAT12   " : "FAT16   ",
		    8);
	}
	memmove(b16.label, label, 11);
	fs_write(0, sizeof(b16), &b16);
    } else if (fs->fat_bits == 32) {
	struct boot_sector b;

	fs_read(0, sizeof(b), &b);
	if (b.extended_sig != 0x29) {
	    b.extended_sig = 0x29;
	    b.serial = 0;
	    memmove(b.fs_type, "FAT32   ", 8);
	}
	memmove(b.label, label, 11);
	fs_write(0, sizeof(b), &b);
	if (fs->backupboot_start)
	    fs_write(fs->backupboot_start, sizeof(b), &b);
    }
}

off_t find_volume_de(DOS_FS * fs, DIR_ENT * de)
{
    uint32_t cluster;
    off_t offset;
    int i;

    if (fs->root_cluster) {
	for (cluster = fs->root_cluster;
	     cluster != 0 && cluster != -1;
	     cluster = next_cluster(fs, cluster)) {
	    offset = cluster_start(fs, cluster);
	    for (i = 0; i * sizeof(DIR_ENT) < fs->cluster_size; i++) {
		fs_read(offset, sizeof(DIR_ENT), de);
		if (de->attr != VFAT_LN_ATTR && de->attr & ATTR_VOLUME)
		    return offset;
		offset += sizeof(DIR_ENT);
	    }
	}
    } else {
	for (i = 0; i < fs->root_entries; i++) {
	    offset = fs->root_start + i * sizeof(DIR_ENT);
	    fs_read(offset, sizeof(DIR_ENT), de);
	    if (de->attr != VFAT_LN_ATTR && de->attr & ATTR_VOLUME)
		return offset;
	}
    }

    return 0;
}

static void write_volume_label(DOS_FS * fs, char *label)
{
    time_t now = time(NULL);
    struct tm *mtime = localtime(&now);
    off_t offset;
    int created;
    DIR_ENT de;

    created = 0;
    offset = find_volume_de(fs, &de);
    if (offset == 0) {
	created = 1;
	offset = alloc_rootdir_entry(fs, &de, label, 0);
    }
    memcpy(de.name, label, 11);
    de.time = htole16((unsigned short)((mtime->tm_sec >> 1) +
				       (mtime->tm_min << 5) +
				       (mtime->tm_hour << 11)));
    de.date = htole16((unsigned short)(mtime->tm_mday +
				       ((mtime->tm_mon + 1) << 5) +
				       ((mtime->tm_year - 80) << 9)));
    if (created) {
	de.attr = ATTR_VOLUME;
	de.ctime_ms = 0;
	de.ctime = de.time;
	de.cdate = de.date;
	de.adate = de.date;
	de.starthi = 0;
	de.start = 0;
	de.size = 0;
    }

    fs_write(offset, sizeof(DIR_ENT), &de);
}

void write_label(DOS_FS * fs, char *label)
{
    int l = strlen(label);

    while (l < 11)
	label[l++] = ' ';

    write_boot_label(fs, label);
    write_volume_label(fs, label);
}
#endif
