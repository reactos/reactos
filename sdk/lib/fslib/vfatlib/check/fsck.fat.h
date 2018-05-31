/* fsck.fat.h  -  Common data structures and global variables

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

#ifndef _DOSFSCK_H
#define _DOSFSCK_H

#ifndef __REACTOS__
#include <sys/types.h>
#include <fcntl.h>
#include <stddef.h>
#endif
#include <stdint.h>

#ifdef __REACTOS__
#ifdef _WIN32

typedef unsigned int __u32;
typedef unsigned __int64 __u64;

#define le16toh(v) (v)
#define le32toh(v) (v)
#define htole16(v) (v)
#define htole32(v) (v)

#endif

#ifdef _M_IX86
#include "byteorder.h"
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#include "byteswap.h"
#else
#define le16toh(v) (v)
#define le32toh(v) (v)
#define htole16(v) (v)
#define htole32(v) (v)
#endif /* __BIG_ENDIAN */

#endif

#ifndef __REACTOS__
#include "endian_compat.h"
#else
#ifndef offsetof
#define offsetof(t,e)	((int)&(((t *)0)->e))
#endif

#include "rosglue.h"
#endif

#include "msdos_fs.h"

#define VFAT_LN_ATTR (ATTR_RO | ATTR_HIDDEN | ATTR_SYS | ATTR_VOLUME)

#define FAT_STATE_DIRTY 0x01

#ifdef __REACTOS__
#include <pshpack1.h>
#endif

/* ++roman: Use own definition of boot sector structure -- the kernel headers'
 * name for it is msdos_boot_sector in 2.0 and fat_boot_sector in 2.1 ... */
struct boot_sector {
    uint8_t ignored[3];		/* Boot strap short or near jump */
    uint8_t system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
    uint8_t sector_size[2];	/* bytes per logical sector */
    uint8_t cluster_size;	/* sectors/cluster */
    uint16_t reserved;		/* reserved sectors */
    uint8_t fats;		/* number of FATs */
    uint8_t dir_entries[2];	/* root directory entries */
    uint8_t sectors[2];		/* number of sectors */
    uint8_t media;		/* media code (unused) */
    uint16_t fat_length;	/* sectors/FAT */
    uint16_t secs_track;	/* sectors per track */
    uint16_t heads;		/* number of heads */
    uint32_t hidden;		/* hidden sectors (unused) */
    uint32_t total_sect;	/* number of sectors (if sectors == 0) */

    /* The following fields are only used by FAT32 */
    uint32_t fat32_length;	/* sectors/FAT */
    uint16_t flags;		/* bit 8: fat mirroring, low 4: active fat */
    uint8_t version[2];		/* major, minor filesystem version */
    uint32_t root_cluster;	/* first cluster in root directory */
    uint16_t info_sector;	/* filesystem info sector */
    uint16_t backup_boot;	/* backup boot sector */
    uint8_t reserved2[12];	/* Unused */

    uint8_t drive_number;	/* Logical Drive Number */
    uint8_t reserved3;		/* Unused */

    uint8_t extended_sig;	/* Extended Signature (0x29) */
    uint32_t serial;		/* Serial number */
    uint8_t label[11];		/* FS label */
    uint8_t fs_type[8];		/* FS Type */

    /* fill up to 512 bytes */
    uint8_t junk[422];
} __attribute__ ((packed));

struct boot_sector_16 {
    uint8_t ignored[3];		/* Boot strap short or near jump */
    uint8_t system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
    uint8_t sector_size[2];	/* bytes per logical sector */
    uint8_t cluster_size;	/* sectors/cluster */
    uint16_t reserved;		/* reserved sectors */
    uint8_t fats;		/* number of FATs */
    uint8_t dir_entries[2];	/* root directory entries */
    uint8_t sectors[2];		/* number of sectors */
    uint8_t media;		/* media code (unused) */
    uint16_t fat_length;	/* sectors/FAT */
    uint16_t secs_track;	/* sectors per track */
    uint16_t heads;		/* number of heads */
    uint32_t hidden;		/* hidden sectors (unused) */
    uint32_t total_sect;	/* number of sectors (if sectors == 0) */

    uint8_t drive_number;	/* Logical Drive Number */
    uint8_t reserved2;		/* Unused */

    uint8_t extended_sig;	/* Extended Signature (0x29) */
    uint32_t serial;		/* Serial number */
    uint8_t label[11];		/* FS label */
    uint8_t fs_type[8];		/* FS Type */

    /* fill up to 512 bytes */
    uint8_t junk[450];
} __attribute__ ((packed));

struct info_sector {
    uint32_t magic;		/* Magic for info sector ('RRaA') */
    uint8_t reserved1[480];
    uint32_t signature;		/* 0x61417272 ('rrAa') */
    uint32_t free_clusters;	/* Free cluster count.  -1 if unknown */
    uint32_t next_cluster;	/* Most recently allocated cluster. */
    uint8_t reserved2[12];
    uint32_t boot_sign;
};

typedef struct {
    uint8_t name[MSDOS_NAME];	/* name including extension */
    uint8_t attr;		/* attribute bits */
    uint8_t lcase;		/* Case for base and extension */
    uint8_t ctime_ms;		/* Creation time, milliseconds */
    uint16_t ctime;		/* Creation time */
    uint16_t cdate;		/* Creation date */
    uint16_t adate;		/* Last access date */
    uint16_t starthi;		/* High 16 bits of cluster in FAT32 */
    uint16_t time, date, start;	/* time, date and first cluster */
    uint32_t size;		/* file size (in bytes) */
} __attribute__ ((packed)) DIR_ENT;

#ifdef __REACTOS__
#include <poppack.h>
#endif

typedef struct _dos_file {
    DIR_ENT dir_ent;
    char *lfn;
    off_t offset;
    off_t lfn_offset;
    struct _dos_file *parent;	/* parent directory */
    struct _dos_file *next;	/* next entry */
    struct _dos_file *first;	/* first entry (directory only) */
} DOS_FILE;

typedef struct {
    uint32_t value;
    uint32_t reserved;
} FAT_ENTRY;

typedef struct {
    int nfats;
    off_t fat_start;
    off_t fat_size;		/* unit is bytes */
    unsigned int fat_bits;	/* size of a FAT entry */
    unsigned int eff_fat_bits;	/* # of used bits in a FAT entry */
    uint32_t root_cluster;	/* 0 for old-style root dir */
    off_t root_start;
    unsigned int root_entries;
    off_t data_start;
    unsigned int cluster_size;
    uint32_t data_clusters;	/* not including two reserved cluster numbers */
    off_t fsinfo_start;		/* 0 if not present */
    long free_clusters;
    off_t backupboot_start;	/* 0 if not present */
    unsigned char *fat;
    DOS_FILE **cluster_owner;
    char *label;
} DOS_FS;

#ifndef __REACTOS__
extern int interactive, rw, list, verbose, test, write_immed;
extern int atari_format;
extern unsigned n_files;
extern void *mem_queue;
#endif

/* value to use as end-of-file marker */
#define FAT_EOF(fs)	((atari_format ? 0xfff : 0xff8) | FAT_EXTD(fs))
#define FAT_IS_EOF(fs,v) ((uint32_t)(v) >= (0xff8|FAT_EXTD(fs)))
/* value to mark bad clusters */
#define FAT_BAD(fs)	(0xff7 | FAT_EXTD(fs))
/* range of values used for bad clusters */
#define FAT_MIN_BAD(fs)	((atari_format ? 0xff0 : 0xff7) | FAT_EXTD(fs))
#define FAT_MAX_BAD(fs)	((atari_format ? 0xff7 : 0xff7) | FAT_EXTD(fs))
#define FAT_IS_BAD(fs,v) ((v) >= FAT_MIN_BAD(fs) && (v) <= FAT_MAX_BAD(fs))

/* return -16 as a number with fs->fat_bits bits */
#define FAT_EXTD(fs)	(((1 << fs->eff_fat_bits)-1) & ~0xf)

/* marker for files with no 8.3 name */
#define FAT_NO_83NAME 32

#endif
