/*
   Filename:     mkdosfs.c
   Version:      0.3b (Yggdrasil)
   Author:       Dave Hudson
   Started:      24th August 1994
   Last Updated: 7th May 1998
   Updated by:   Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
   Target O/S:   Linux (2.x)

   Description: Utility to allow an MS-DOS filesystem to be created
   under Linux.  A lot of the basic structure of this program has been
   borrowed from Remy Card's "mke2fs" code.

   As far as possible the aim here is to make the "mkdosfs" command
   look almost identical to the other Linux filesystem make utilties,
   eg bad blocks are still specified as blocks, not sectors, but when
   it comes down to it, DOS is tied to the idea of a sector (512 bytes
   as a rule), and not the block.  For example the boot block does not
   occupy a full cluster.

   Fixes/additions May 1998 by Roman Hodek
   <Roman.Hodek@informatik.uni-erlangen.de>:
   - Atari format support
   - New options -A, -S, -C
   - Support for filesystems > 2GB
   - FAT32 support

   Port to work under Windows NT/2K/XP Dec 2002 by
   Jens-Uwe Mager <jum@anubis.han.de>

   Copying:     Copyright 1993, 1994 David Hudson (dave@humbug.demon.co.uk)

   Portions copyright 1992, 1993 Remy Card (card@masi.ibp.fr)
   and 1991 Linus Torvalds (torvalds@klaava.helsinki.fi)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */


/* Include the header files */

#include "../version.h"

#ifdef _WIN32
#define _WIN32_WINNT	0x0400
#include <windows.h>
#include <winioctl.h>
#define __LITTLE_ENDIAN	1234
#define __BIG_ENDIAN	4321
#define __BYTE_ORDER	__LITTLE_ENDIAN
#define inline
#define __attribute__(x)
#define BLOCK_SIZE		512
#else
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <linux/fd.h>
#include <endian.h>
#include <mntent.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#if __BYTE_ORDER == __BIG_ENDIAN

#include <asm/byteorder.h>
#ifdef __le16_to_cpu
/* ++roman: 2.1 kernel headers define these function, they're probably more
 * efficient then coding the swaps machine-independently. */
#define CF_LE_W	__le16_to_cpu
#define CF_LE_L	__le32_to_cpu
#define CT_LE_W	__cpu_to_le16
#define CT_LE_L	__cpu_to_le32
#else
#define CF_LE_W(v) ((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))
#define CF_LE_L(v) (((unsigned)(v)>>24) | (((unsigned)(v)>>8)&0xff00) | \
               (((unsigned)(v)<<8)&0xff0000) | ((unsigned)(v)<<24))
#define CT_LE_W(v) CF_LE_W(v)
#define CT_LE_L(v) CF_LE_L(v)
#endif /* defined(__le16_to_cpu) */

#else

#define CF_LE_W(v) (v)
#define CF_LE_L(v) (v)
#define CT_LE_W(v) (v)
#define CT_LE_L(v) (v)

#endif /* __BIG_ENDIAN */

#ifdef _WIN32

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned __int64 __u64;
typedef __int64 loff_t;
typedef __int64 ll_t;

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;
int getopt(int argc, char *const argv[], const char * optstring);

static int is_device = 0;

#define open	WIN32open
#define close	WIN32close
#define read	WIN32read
#define write	WIN32write
#define llseek	WIN32llseek

#define O_SHORT_LIVED   _O_SHORT_LIVED
#define O_ACCMODE       3
#define O_NONE          3
#define O_BACKUP        0x10000
#define O_SHARED        0x20000

static int WIN32open(const char *path, int oflag, ...)
{
	HANDLE fh;
	DWORD desiredAccess;
	DWORD shareMode;
	DWORD creationDisposition;
	DWORD flagsAttributes = FILE_ATTRIBUTE_NORMAL;
	SECURITY_ATTRIBUTES securityAttributes;
	va_list ap;
	int pmode;
	int trunc = FALSE;

	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.lpSecurityDescriptor = NULL;
	securityAttributes.bInheritHandle = oflag & O_NOINHERIT ? FALSE : TRUE;
	switch (oflag & O_ACCMODE) {
	case O_RDONLY:
		desiredAccess = GENERIC_READ;
		shareMode = FILE_SHARE_READ;
		break;
	case O_WRONLY:
		desiredAccess = GENERIC_WRITE;
		shareMode = 0;
		break;
	case O_RDWR:
		desiredAccess = GENERIC_READ|GENERIC_WRITE;
		shareMode = 0;
		break;
	case O_NONE:
		desiredAccess = 0;
		shareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
	}
	if (oflag & O_APPEND) {
		desiredAccess |= FILE_APPEND_DATA|SYNCHRONIZE;
		shareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
	}
	if (oflag & O_SHARED)
		shareMode |= FILE_SHARE_READ|FILE_SHARE_WRITE;
        switch (oflag & (O_CREAT|O_EXCL|O_TRUNC)) {
	case 0:
	case O_EXCL:
		creationDisposition = OPEN_EXISTING;
		break;
	case O_CREAT:
		creationDisposition = OPEN_ALWAYS;
		break;
	case O_CREAT|O_EXCL:
	case O_CREAT|O_TRUNC|O_EXCL:
		creationDisposition = CREATE_NEW;
		break;
	case O_TRUNC:
	case O_TRUNC|O_EXCL:
		creationDisposition = TRUNCATE_EXISTING;
		break;
	case O_CREAT|O_TRUNC:
		creationDisposition = OPEN_ALWAYS;
		trunc = TRUE;
		break;
        }
	if (oflag & O_CREAT) {
		va_start(ap, oflag);
		pmode = va_arg(ap, int);
		va_end(ap);
		if ((pmode & 0222) == 0)
			flagsAttributes |= FILE_ATTRIBUTE_READONLY;
	}
	if (oflag & O_TEMPORARY) {
		flagsAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
		desiredAccess |= DELETE;
	}
	if (oflag & O_SHORT_LIVED)
		flagsAttributes |= FILE_ATTRIBUTE_TEMPORARY;
	if (oflag & O_SEQUENTIAL)
		flagsAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
	else if (oflag & O_RANDOM)
		flagsAttributes |= FILE_FLAG_RANDOM_ACCESS;
	if (oflag & O_BACKUP)
		flagsAttributes |= FILE_FLAG_BACKUP_SEMANTICS;
	if ((fh = CreateFile(path, desiredAccess, shareMode, &securityAttributes,
				creationDisposition, flagsAttributes, NULL)) == INVALID_HANDLE_VALUE) {
		errno = GetLastError();
		return -1;
	}
	if (trunc) {
		if (!SetEndOfFile(fh)) {
			errno = GetLastError();
			CloseHandle(fh);
			DeleteFile(path);
			return -1;
		}
	}
	return (int)fh;
}

static int WIN32close(int fd)
{
	if (!CloseHandle((HANDLE)fd)) {
		errno = GetLastError();
		return -1;
	}
	return 0;
}

static int WIN32read(int fd, void *buf, unsigned int len)
{
	DWORD actualLen;

	if (!ReadFile((HANDLE)fd, buf, (DWORD)len, &actualLen, NULL)) {
		errno = GetLastError();
		if (errno == ERROR_BROKEN_PIPE)
			return 0;
		else
			return -1;
	}
	return (int)actualLen;
}

static int WIN32write(int fd, void *buf, unsigned int len)
{
	DWORD actualLen;

	if (!WriteFile((HANDLE)fd, buf, (DWORD)len, &actualLen, NULL)) {
		errno = GetLastError();
		return -1;
	}
	return (int)actualLen;
}

static loff_t WIN32llseek(int fd, loff_t offset, int whence)
{
	long lo, hi;
	DWORD err;

	lo = offset & 0xffffffff;
	hi = offset >> 32;
	lo = SetFilePointer((HANDLE)fd, lo, &hi, whence);
	if (lo == 0xFFFFFFFF && (err = GetLastError()) != NO_ERROR) {
		errno = err;
		return -1;
	}
	return ((loff_t)hi << 32) | (off_t)lo;
}

int fsctl(int fd, int code)
{
	DWORD ret;
	if (!DeviceIoControl((HANDLE)fd, code, NULL, 0, NULL, 0, &ret, NULL)) {
		errno = GetLastError();
		return -1;
	}
	return 0;
}

#else

#define O_NOINHERIT    0
#define O_TEMPORARY    0
#define O_SHORT_LIVED  0
#define O_SEQUENTIAL   0
#define O_RANDOM       0
#define O_BACKUP       0
#define O_SHARED       0
#ifndef O_NONE
# define O_NONE        0
#endif

typedef long long ll_t;
/* Use the _llseek system call directly, because there (once?) was a bug in
 * the glibc implementation of it. */
#include <linux/unistd.h>
#if defined(__alpha) || defined(__ia64__)
/* On alpha, the syscall is simply lseek, because it's a 64 bit system. */
static loff_t llseek( int fd, loff_t offset, int whence )
{
    return lseek(fd, offset, whence);
}
#else
# ifndef __NR__llseek
# error _llseek system call not present
# endif
static _syscall5( int, _llseek, uint, fd, ulong, hi, ulong, lo,
		  loff_t *, res, uint, wh );
static loff_t llseek( int fd, loff_t offset, int whence )
{
    loff_t actual;

    if (_llseek(fd, offset>>32, offset&0xffffffff, &actual, whence) != 0)
	return (loff_t)-1;
    return actual;
}
#endif

#endif

/* Constant definitions */

#define TRUE 1			/* Boolean constants */
#define FALSE 0

#define TEST_BUFFER_BLOCKS 16
#define HARD_SECTOR_SIZE   512
#define SECTORS_PER_BLOCK ( BLOCK_SIZE / HARD_SECTOR_SIZE )


/* Macro definitions */

/* Report a failure message and return a failure error code */

#define die( str ) fatal_error( "%s: " str "\n" )


/* Mark a cluster in the FAT as bad */

#define mark_sector_bad( sector ) mark_FAT_sector( sector, FAT_BAD )

/* Compute ceil(a/b) */

inline int
cdiv (int a, int b)
{
  return (a + b - 1) / b;
}

/* MS-DOS filesystem structures -- I included them here instead of
   including linux/msdos_fs.h since that doesn't include some fields we
   need */

#define ATTR_RO      1		/* read-only */
#define ATTR_HIDDEN  2		/* hidden */
#define ATTR_SYS     4		/* system */
#define ATTR_VOLUME  8		/* volume label */
#define ATTR_DIR     16		/* directory */
#define ATTR_ARCH    32		/* archived */

#define ATTR_NONE    0		/* no attribute bits */
#define ATTR_UNUSED  (ATTR_VOLUME | ATTR_ARCH | ATTR_SYS | ATTR_HIDDEN)
	/* attribute bits that are copied "as is" */

/* FAT values */
#define FAT_EOF      (atari_format ? 0x0fffffff : 0x0ffffff8)
#define FAT_BAD      0x0ffffff7

#define MSDOS_EXT_SIGN 0x29	/* extended boot sector signature */
#define MSDOS_FAT12_SIGN "FAT12   "	/* FAT12 filesystem signature */
#define MSDOS_FAT16_SIGN "FAT16   "	/* FAT16 filesystem signature */
#define MSDOS_FAT32_SIGN "FAT32   "	/* FAT32 filesystem signature */

#define BOOT_SIGN 0xAA55	/* Boot sector magic number */

#define MAX_CLUST_12	((1 << 12) - 16)
#define MAX_CLUST_16	((1 << 16) - 16)
/* M$ says the high 4 bits of a FAT32 FAT entry are reserved and don't belong
 * to the cluster number. So the max. cluster# is based on 2^28 */
#define MAX_CLUST_32	((1 << 28) - 16)

#define FAT12_THRESHOLD	4078

#define OLDGEMDOS_MAX_SECTORS	32765
#define GEMDOS_MAX_SECTORS	65531
#define GEMDOS_MAX_SECTOR_SIZE	(16*1024)

#define BOOTCODE_SIZE		448
#define BOOTCODE_FAT32_SIZE	420

/* __attribute__ ((packed)) is used on all structures to make gcc ignore any
 * alignments */

#ifdef _WIN32
#pragma pack(push, 1)
#endif
struct msdos_volume_info {
  __u8		drive_number;	/* BIOS drive number */
  __u8		RESERVED;	/* Unused */
  __u8		ext_boot_sign;	/* 0x29 if fields below exist (DOS 3.3+) */
  __u8		volume_id[4];	/* Volume ID number */
  __u8		volume_label[11];/* Volume label */
  __u8		fs_type[8];	/* Typically FAT12 or FAT16 */
} __attribute__ ((packed));

struct msdos_boot_sector
{
  __u8	        boot_jump[3];	/* Boot strap short or near jump */
  __u8          system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
  __u8          sector_size[2];	/* bytes per logical sector */
  __u8          cluster_size;	/* sectors/cluster */
  __u16         reserved;	/* reserved sectors */
  __u8          fats;		/* number of FATs */
  __u8          dir_entries[2];	/* root directory entries */
  __u8          sectors[2];	/* number of sectors */
  __u8          media;		/* media code (unused) */
  __u16         fat_length;	/* sectors/FAT */
  __u16         secs_track;	/* sectors per track */
  __u16         heads;		/* number of heads */
  __u32         hidden;		/* hidden sectors (unused) */
  __u32         total_sect;	/* number of sectors (if sectors == 0) */
  union {
    struct {
      struct msdos_volume_info vi;
      __u8	boot_code[BOOTCODE_SIZE];
    } __attribute__ ((packed)) _oldfat;
    struct {
      __u32	fat32_length;	/* sectors/FAT */
      __u16	flags;		/* bit 8: fat mirroring, low 4: active fat */
      __u8	version[2];	/* major, minor filesystem version */
      __u32	root_cluster;	/* first cluster in root directory */
      __u16	info_sector;	/* filesystem info sector */
      __u16	backup_boot;	/* backup boot sector */
      __u16	reserved2[6];	/* Unused */
      struct msdos_volume_info vi;
      __u8	boot_code[BOOTCODE_FAT32_SIZE];
    } __attribute__ ((packed)) _fat32;
  } __attribute__ ((packed)) fstype;
  __u16		boot_sign;
} __attribute__ ((packed));
#define fat32	fstype._fat32
#define oldfat	fstype._oldfat

struct fat32_fsinfo {
  __u32		reserved1;	/* Nothing as far as I can tell */
  __u32		signature;	/* 0x61417272L */
  __u32		free_clusters;	/* Free cluster count.  -1 if unknown */
  __u32		next_cluster;	/* Most recently allocated cluster.
				 * Unused under Linux. */
  __u32		reserved2[4];
};

struct msdos_dir_entry
  {
    char	name[8], ext[3];	/* name and extension */
    __u8        attr;			/* attribute bits */
    __u8	lcase;			/* Case for base and extension */
    __u8	ctime_ms;		/* Creation time, milliseconds */
    __u16	ctime;			/* Creation time */
    __u16	cdate;			/* Creation date */
    __u16	adate;			/* Last access date */
    __u16	starthi;		/* high 16 bits of first cl. (FAT32) */
    __u16	time, date, start;	/* time, date and first cluster */
    __u32	size;			/* file size (in bytes) */
  } __attribute__ ((packed));

#ifdef _WIN32
#pragma pack(pop)
#endif

/* The "boot code" we put into the filesystem... it writes a message and
   tells the user to try again */

char dummy_boot_jump[3] = { 0xeb, 0x3c, 0x90 };

char dummy_boot_jump_m68k[2] = { 0x60, 0x1c };

char dummy_boot_code[BOOTCODE_SIZE] =
  "\x0e"			/* push cs */
  "\x1f"			/* pop ds */
  "\xbe\x5b\x7c"		/* mov si, offset message_txt */
				/* write_msg: */
  "\xac"			/* lodsb */
  "\x22\xc0"			/* and al, al */
  "\x74\x0b"			/* jz key_press */
  "\x56"			/* push si */
  "\xb4\x0e"			/* mov ah, 0eh */
  "\xbb\x07\x00"		/* mov bx, 0007h */
  "\xcd\x10"			/* int 10h */
  "\x5e"			/* pop si */
  "\xeb\xf0"			/* jmp write_msg */
				/* key_press: */
  "\x32\xe4"			/* xor ah, ah */
  "\xcd\x16"			/* int 16h */
  "\xcd\x19"			/* int 19h */
  "\xeb\xfe"			/* foo: jmp foo */
				/* message_txt: */

  "This is not a bootable disk.  Please insert a bootable floppy and\r\n"
  "press any key to try again ... \r\n";

#define MESSAGE_OFFSET 29	/* Offset of message in above code */

/* Global variables - the root of all evil :-) - see these and weep! */

static char *program_name = "mkdosfs";	/* Name of the program */
static char *device_name = NULL;	/* Name of the device on which to create the filesystem */
static int atari_format = 0;	/* Use Atari variation of MS-DOS FS format */
static int check = FALSE;	/* Default to no readablity checking */
static int verbose = 0;		/* Default to verbose mode off */
static long volume_id;		/* Volume ID number */
static time_t create_time;	/* Creation time */
static char volume_name[] = "           "; /* Volume name */
static int blocks;		/* Number of blocks in filesystem */
static int sector_size = 512;	/* Size of a logical sector */
static int sector_size_set = 0; /* User selected sector size */
static int backup_boot = 0;	/* Sector# of backup boot sector */
static int reserved_sectors = 0;/* Number of reserved sectors */
static int badblocks = 0;	/* Number of bad blocks in the filesystem */
static int nr_fats = 2;		/* Default number of FATs to produce */
static int size_fat = 0;	/* Size in bits of FAT entries */
static int size_fat_by_user = 0; /* 1 if FAT size user selected */
static int dev = -1;		/* FS block device file handle */
static int  ignore_full_disk = 0; /* Ignore warning about 'full' disk devices */
static unsigned int currently_testing = 0;	/* Block currently being tested (if autodetect bad blocks) */
static struct msdos_boot_sector bs;	/* Boot sector data */
static int start_data_sector;	/* Sector number for the start of the data area */
static int start_data_block;	/* Block number for the start of the data area */
static unsigned char *fat;	/* File allocation table */
static unsigned char *info_sector;	/* FAT32 info sector */
static struct msdos_dir_entry *root_dir;	/* Root directory */
static int size_root_dir;	/* Size of the root directory in bytes */
static int sectors_per_cluster = 0;	/* Number of sectors per disk cluster */
static int root_dir_entries = 0;	/* Number of root directory entries */
static char *blank_sector;		/* Blank sector - all zeros */


/* Function prototype definitions */

static void fatal_error (const char *fmt_string) __attribute__((noreturn));
static void mark_FAT_cluster (int cluster, unsigned int value);
static void mark_FAT_sector (int sector, unsigned int value);
static long do_check (char *buffer, int try, unsigned int current_block);
static void alarm_intr (int alnum);
static void check_blocks (void);
static void get_list_blocks (char *filename);
static int valid_offset (int fd, loff_t offset);
static int count_blocks (char *filename);
static void check_mount (char *device_name);
#ifdef _WIN32
static void establish_params (void);
#else
static void establish_params (int device_num, int size);
#endif
static void setup_tables (void);
static void write_tables (void);


/* The function implementations */

/* Handle the reporting of fatal errors.  Volatile to let gcc know that this doesn't return */

static void
fatal_error (const char *fmt_string)
{
  fprintf (stderr, fmt_string, program_name, device_name);
  exit (1);			/* The error exit code is 1! */
}


/* Mark the specified cluster as having a particular value */

static void
mark_FAT_cluster (int cluster, unsigned int value)
{
  switch( size_fat ) {
    case 12:
      value &= 0x0fff;
      if (((cluster * 3) & 0x1) == 0)
	{
	  fat[3 * cluster / 2] = (unsigned char) (value & 0x00ff);
	  fat[(3 * cluster / 2) + 1] = (unsigned char) ((fat[(3 * cluster / 2) + 1] & 0x00f0)
						 | ((value & 0x0f00) >> 8));
	}
      else
	{
	  fat[3 * cluster / 2] = (unsigned char) ((fat[3 * cluster / 2] & 0x000f) | ((value & 0x000f) << 4));
	  fat[(3 * cluster / 2) + 1] = (unsigned char) ((value & 0x0ff0) >> 4);
	}
      break;

    case 16:
      value &= 0xffff;
      fat[2 * cluster] = (unsigned char) (value & 0x00ff);
      fat[(2 * cluster) + 1] = (unsigned char) (value >> 8);
      break;

    case 32:
      value &= 0xfffffff;
      fat[4 * cluster] =       (unsigned char)  (value & 0x000000ff);
      fat[(4 * cluster) + 1] = (unsigned char) ((value & 0x0000ff00) >> 8);
      fat[(4 * cluster) + 2] = (unsigned char) ((value & 0x00ff0000) >> 16);
      fat[(4 * cluster) + 3] = (unsigned char) ((value & 0xff000000) >> 24);
      break;

    default:
      die("Bad FAT size (not 12, 16, or 32)");
  }
}


/* Mark a specified sector as having a particular value in it's FAT entry */

static void
mark_FAT_sector (int sector, unsigned int value)
{
  int cluster;

  cluster = (sector - start_data_sector) / (int) (bs.cluster_size) /
	    (sector_size/HARD_SECTOR_SIZE);
  if (cluster < 0)
    die ("Invalid cluster number in mark_FAT_sector: probably bug!");

  mark_FAT_cluster (cluster, value);
}


/* Perform a test on a block.  Return the number of blocks that could be read successfully */

static long
do_check (char *buffer, int try, unsigned int current_block)
{
  long got;

  if (llseek (dev, (loff_t)current_block * BLOCK_SIZE, SEEK_SET) /* Seek to the correct location */
      != (loff_t)current_block * BLOCK_SIZE)
    die ("seek failed during testing for blocks");

  got = read (dev, buffer, try * BLOCK_SIZE);	/* Try reading! */
  if (got < 0)
    got = 0;

  if (got & (BLOCK_SIZE - 1))
    printf ("Unexpected values in do_check: probably bugs\n");
  got /= BLOCK_SIZE;

  return got;
}

#ifndef _WIN32
/* Alarm clock handler - display the status of the quest for bad blocks!  Then retrigger the alarm for five senconds
   later (so we can come here again) */

static void
alarm_intr (int alnum)
{
  if (currently_testing >= blocks)
    return;

  signal (SIGALRM, alarm_intr);
  alarm (5);
  if (!currently_testing)
    return;

  printf ("%d... ", currently_testing);
  fflush (stdout);
}
#endif

static void
check_blocks (void)
{
  int try, got;
  int i;
  static char blkbuf[BLOCK_SIZE * TEST_BUFFER_BLOCKS];

  if (verbose)
    {
      printf ("Searching for bad blocks ");
      fflush (stdout);
    }
  currently_testing = 0;
#ifndef _WIN32
  if (verbose)
    {
      signal (SIGALRM, alarm_intr);
      alarm (5);
    }
#endif
  try = TEST_BUFFER_BLOCKS;
  while (currently_testing < blocks)
    {
      if (currently_testing + try > blocks)
	try = blocks - currently_testing;
      got = do_check (blkbuf, try, currently_testing);
      currently_testing += got;
      if (got == try)
	{
	  try = TEST_BUFFER_BLOCKS;
	  continue;
	}
      else
	try = 1;
      if (currently_testing < start_data_block)
	die ("bad blocks before data-area: cannot make fs");

      for (i = 0; i < SECTORS_PER_BLOCK; i++)	/* Mark all of the sectors in the block as bad */
	mark_sector_bad (currently_testing * SECTORS_PER_BLOCK + i);
      badblocks++;
      currently_testing++;
    }

  if (verbose)
    printf ("\n");

  if (badblocks)
    printf ("%d bad block%s\n", badblocks,
	    (badblocks > 1) ? "s" : "");
}


static void
get_list_blocks (char *filename)
{
  int i;
  FILE *listfile;
  unsigned long blockno;

  listfile = fopen (filename, "r");
  if (listfile == (FILE *) NULL)
    die ("Can't open file of bad blocks");

  while (!feof (listfile))
    {
      fscanf (listfile, "%ld\n", &blockno);
      for (i = 0; i < SECTORS_PER_BLOCK; i++)	/* Mark all of the sectors in the block as bad */
	mark_sector_bad (blockno * SECTORS_PER_BLOCK + i);
      badblocks++;
    }
  fclose (listfile);

  if (badblocks)
    printf ("%d bad block%s\n", badblocks,
	    (badblocks > 1) ? "s" : "");
}


#ifndef _WIN32
/* Given a file descriptor and an offset, check whether the offset is a valid offset for the file - return FALSE if it
   isn't valid or TRUE if it is */

static int
valid_offset (int fd, loff_t offset)
{
  char ch;

  if (llseek (fd, offset, SEEK_SET) < 0)
    return FALSE;
  if (read (fd, &ch, 1) < 1)
    return FALSE;
  return TRUE;
}
#endif


/* Given a filename, look to see how many blocks of BLOCK_SIZE are present, returning the answer */

static int
count_blocks (char *filename)
{
#ifdef _WIN32
	int fd;
	DISK_GEOMETRY geom;
	BY_HANDLE_FILE_INFORMATION hinfo;
	DWORD ret;
	loff_t len = 0;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		perror(filename);
		exit(1);
	}
	/*
	 * This should probably use IOCTL_DISK_GET_LENGTH_INFO here, but
	 * this ioctl is only available in XP and up.
	 */
	if (is_device) {
		if (!DeviceIoControl((HANDLE)fd, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geom, sizeof(geom), &ret, NULL)) {
			errno = GetLastError();
			die("unable to get length for '%s'");
		}
		len = geom.Cylinders.QuadPart*geom.TracksPerCylinder*geom.SectorsPerTrack*BLOCK_SIZE;
	} else {
		if (!GetFileInformationByHandle((HANDLE)fd, &hinfo)) {
				errno = GetLastError();
				die("unable to get length for '%s'");
		}
		len = ((loff_t)hinfo.nFileSizeHigh << 32) | (loff_t)hinfo.nFileSizeLow;
	}
	close(fd);
	return len/BLOCK_SIZE;
#else
  loff_t high, low;
  int fd;

  if ((fd = open (filename, O_RDONLY)) < 0)
    {
      perror (filename);
      exit (1);
    }
  low = 0;

  for (high = 1; valid_offset (fd, high); high *= 2)
    low = high;
  while (low < high - 1)
    {
      const loff_t mid = (low + high) / 2;

      if (valid_offset (fd, mid))
	low = mid;
      else
	high = mid;
    }
  valid_offset (fd, 0);
  close (fd);

  return (low + 1) / BLOCK_SIZE;
#endif
}


/* Check to see if the specified device is currently mounted - abort if it is */

static void
check_mount (char *device_name)
{
#ifndef _WIN32
  FILE *f;
  struct mntent *mnt;

  if ((f = setmntent (MOUNTED, "r")) == NULL)
    return;
  while ((mnt = getmntent (f)) != NULL)
    if (strcmp (device_name, mnt->mnt_fsname) == 0)
      die ("%s contains a mounted file system.");
  endmntent (f);
#endif
}


/* Establish the geometry and media parameters for the device */
#ifdef _WIN32
static void
establish_params (void)
{
	DISK_GEOMETRY geometry;
	DWORD ret;

	if (!is_device) {
		bs.media = (char) 0xf8; /* Set up the media descriptor for a hard drive */
		bs.dir_entries[0] = (char) 0;
		bs.dir_entries[1] = (char) 2;
		/* For FAT32, use 4k clusters on sufficiently large file systems,
		 * otherwise 1 sector per cluster. This is also what M$'s format
		 * command does for FAT32. */
		bs.cluster_size = (char)
		 (size_fat == 32 ?
	     ((ll_t)blocks*SECTORS_PER_BLOCK >= 512*1024 ? 8 : 1) :
	      4); /* FAT12 and FAT16: start at 4 sectors per cluster */
		return;
	}
	if (!DeviceIoControl((HANDLE)dev, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geometry, sizeof(geometry), &ret, NULL)) {
		errno = GetLastError();
		die ("unable to get geometry for '%s'");
	}
    bs.secs_track = geometry.SectorsPerTrack;
    bs.heads = geometry.TracksPerCylinder;
	switch (geometry.MediaType) {
	case F3_1Pt44_512:
		bs.media = (char) 0xf9;
		bs.cluster_size = (char) 2;
		bs.dir_entries[0] = (char) 112;
		bs.dir_entries[1] = (char) 0;
		break;
	case F3_2Pt88_512:
		bs.media = (char) 0xf0;
		bs.cluster_size = (char)(atari_format ? 2 : 1);
		bs.dir_entries[0] = (char) 224;
		bs.dir_entries[1] = (char) 0;
		break;
	case F3_720_512:
		bs.media = (char) 0xfd;
		bs.cluster_size = (char) 2;
		bs.dir_entries[0] = (char) 112;
		bs.dir_entries[1] = (char) 0;
		break;
	default:
		bs.media = (char) 0xf8; /* Set up the media descriptor for a hard drive */
		bs.dir_entries[0] = (char) 0;
		bs.dir_entries[1] = (char) 2;
		/* For FAT32, use 4k clusters on sufficiently large file systems,
		 * otherwise 1 sector per cluster. This is also what M$'s format
		 * command does for FAT32. */
		bs.cluster_size = (char)
		 (size_fat == 32 ?
	     ((ll_t)blocks*SECTORS_PER_BLOCK >= 512*1024 ? 8 : 1) :
	      4); /* FAT12 and FAT16: start at 4 sectors per cluster */
	}
}
#else
static void
establish_params (int device_num,int size)
{
  long loop_size;
  struct hd_geometry geometry;
  struct floppy_struct param;

  if ((0 == device_num) || ((device_num & 0xff00) == 0x0200))
    /* file image or floppy disk */
    {
      if (0 == device_num)
	{
	  param.size = size/512;
	  switch(param.size)
	    {
	    case 720:
	      param.sect = 9 ;
	      param.head = 2;
	      break;
	    case 1440:
	      param.sect = 9;
	      param.head = 2;
	      break;
	    case 2400:
	      param.sect = 15;
	      param.head = 2;
	      break;
	    case 2880:
	      param.sect = 18;
	      param.head = 2;
	      break;
	    case 5760:
	      param.sect = 36;
	      param.head = 2;
	      break;
	    default:
	      /* fake values */
	      param.sect = 32;
	      param.head = 64;
	      break;
	    }

	}
      else 	/* is a floppy diskette */
	{
	  if (ioctl (dev, FDGETPRM, &param))	/*  Can we get the diskette geometry? */
	    die ("unable to get diskette geometry for '%s'");
	}
      bs.secs_track = CT_LE_W(param.sect);	/*  Set up the geometry information */
      bs.heads = CT_LE_W(param.head);
      switch (param.size)	/*  Set up the media descriptor byte */
	{
	case 720:		/* 5.25", 2, 9, 40 - 360K */
	  bs.media = (char) 0xfd;
	  bs.cluster_size = (char) 2;
	  bs.dir_entries[0] = (char) 112;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 1440:		/* 3.5", 2, 9, 80 - 720K */
	  bs.media = (char) 0xf9;
	  bs.cluster_size = (char) 2;
	  bs.dir_entries[0] = (char) 112;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 2400:		/* 5.25", 2, 15, 80 - 1200K */
	  bs.media = (char) 0xf9;
	  bs.cluster_size = (char)(atari_format ? 2 : 1);
	  bs.dir_entries[0] = (char) 224;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 5760:		/* 3.5", 2, 36, 80 - 2880K */
	  bs.media = (char) 0xf0;
	  bs.cluster_size = (char) 2;
	  bs.dir_entries[0] = (char) 224;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 2880:		/* 3.5", 2, 18, 80 - 1440K */
	floppy_default:
	  bs.media = (char) 0xf0;
	  bs.cluster_size = (char)(atari_format ? 2 : 1);
	  bs.dir_entries[0] = (char) 224;
	  bs.dir_entries[1] = (char) 0;
	  break;

	default:		/* Anything else */
	  if (0 == device_num)
	      goto def_hd_params;
	  else
	      goto floppy_default;
	}
    }
  else if ((device_num & 0xff00) == 0x0700) /* This is a loop device */
    {
      /* Can we get the loop geometry? This is in 512 byte blocks, always? */
      if (ioctl (dev, BLKGETSIZE, &loop_size))
	die ("unable to get loop geometry for '%s'");
      loop_size = loop_size >> 1;

      switch (loop_size)  /* Assuming the loop device -> floppy later */
	{
	case 720:		/* 5.25", 2, 9, 40 - 360K */
	  bs.secs_track = CF_LE_W(9);
	  bs.heads = CF_LE_W(2);
	  bs.media = (char) 0xfd;
	  bs.cluster_size = (char) 2;
	  bs.dir_entries[0] = (char) 112;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 1440:		/* 3.5", 2, 9, 80 - 720K */
	  bs.secs_track = CF_LE_W(9);
	  bs.heads = CF_LE_W(2);
	  bs.media = (char) 0xf9;
	  bs.cluster_size = (char) 2;
	  bs.dir_entries[0] = (char) 112;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 2400:		/* 5.25", 2, 15, 80 - 1200K */
	  bs.secs_track = CF_LE_W(15);
	  bs.heads = CF_LE_W(2);
	  bs.media = (char) 0xf9;
	  bs.cluster_size = (char)(atari_format ? 2 : 1);
	  bs.dir_entries[0] = (char) 224;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 5760:		/* 3.5", 2, 36, 80 - 2880K */
	  bs.secs_track = CF_LE_W(36);
	  bs.heads = CF_LE_W(2);
	  bs.media = (char) 0xf0;
	  bs.cluster_size = (char) 2;
	  bs.dir_entries[0] = (char) 224;
	  bs.dir_entries[1] = (char) 0;
	  break;

	case 2880:		/* 3.5", 2, 18, 80 - 1440K */
	  bs.secs_track = CF_LE_W(18);
	  bs.heads = CF_LE_W(2);
	  bs.media = (char) 0xf0;
	  bs.cluster_size = (char)(atari_format ? 2 : 1);
	  bs.dir_entries[0] = (char) 224;
	  bs.dir_entries[1] = (char) 0;
	  break;

	default:		/* Anything else: default hd setup */
	  printf("Loop device does not match a floppy size, using "
		 "default hd params\n");
	  bs.secs_track = CT_LE_W(32); /* these are fake values... */
	  bs.heads = CT_LE_W(64);
	  goto def_hd_params;
	}
    }
  else
    /* Must be a hard disk then! */
    {
      /* Can we get the drive geometry? (Note I'm not too sure about */
      /* whether to use HDIO_GETGEO or HDIO_REQ) */
      if (ioctl (dev, HDIO_GETGEO, &geometry))
	die ("unable to get drive geometry for '%s'");
      bs.secs_track = CT_LE_W(geometry.sectors);	/* Set up the geometry information */
      bs.heads = CT_LE_W(geometry.heads);
    def_hd_params:
      bs.media = (char) 0xf8; /* Set up the media descriptor for a hard drive */
      bs.dir_entries[0] = (char) 0;	/* Default to 512 entries */
      bs.dir_entries[1] = (char) 2;
      /* For FAT32, use 4k clusters on sufficiently large file systems,
       * otherwise 1 sector per cluster. This is also what M$'s format
       * command does for FAT32. */
      bs.cluster_size = (char)
	    (size_fat == 32 ?
	     ((ll_t)blocks*SECTORS_PER_BLOCK >= 512*1024 ? 8 : 1) :
	     4); /* FAT12 and FAT16: start at 4 sectors per cluster */
    }
}
#endif


/* Create the filesystem data tables */

static void
setup_tables (void)
{
  unsigned num_sectors;
  unsigned cluster_count = 0, fat_length;
  unsigned fatdata;			/* Sectors for FATs + data area */
  struct tm *ctime;
  struct msdos_volume_info *vi = (size_fat == 32 ? &bs.fat32.vi : &bs.oldfat.vi);

  if (atari_format)
      /* On Atari, the first few bytes of the boot sector are assigned
       * differently: The jump code is only 2 bytes (and m68k machine code
       * :-), then 6 bytes filler (ignored), then 3 byte serial number. */
    strncpy( bs.system_id-1, "mkdosf", 6 );
  else
    strcpy (bs.system_id, "mkdosfs");
  if (sectors_per_cluster)
    bs.cluster_size = (char) sectors_per_cluster;
  if (size_fat == 32)
    {
      /* Under FAT32, the root dir is in a cluster chain, and this is
       * signalled by bs.dir_entries being 0. */
      bs.dir_entries[0] = bs.dir_entries[1] = (char) 0;
      root_dir_entries = 0;
    }
  else if (root_dir_entries)
    {
      /* Override default from establish_params() */
      bs.dir_entries[0] = (char) (root_dir_entries & 0x00ff);
      bs.dir_entries[1] = (char) ((root_dir_entries & 0xff00) >> 8);
    }
  else
    root_dir_entries = bs.dir_entries[0] + (bs.dir_entries[1] << 8);

  if (atari_format) {
    bs.system_id[5] = (unsigned char) (volume_id & 0x000000ff);
    bs.system_id[6] = (unsigned char) ((volume_id & 0x0000ff00) >> 8);
    bs.system_id[7] = (unsigned char) ((volume_id & 0x00ff0000) >> 16);
  }
  else {
    vi->volume_id[0] = (unsigned char) (volume_id & 0x000000ff);
    vi->volume_id[1] = (unsigned char) ((volume_id & 0x0000ff00) >> 8);
    vi->volume_id[2] = (unsigned char) ((volume_id & 0x00ff0000) >> 16);
    vi->volume_id[3] = (unsigned char) (volume_id >> 24);
  }

  if (!atari_format) {
    memcpy(vi->volume_label, volume_name, 11);

    memcpy(bs.boot_jump, dummy_boot_jump, 3);
    /* Patch in the correct offset to the boot code */
    bs.boot_jump[1] = ((size_fat == 32 ?
			(char *)&bs.fat32.boot_code :
			(char *)&bs.oldfat.boot_code) -
		       (char *)&bs) - 2;

    if (size_fat == 32) {
	if (dummy_boot_code[BOOTCODE_FAT32_SIZE-1])
	  printf ("Warning: message too long; truncated\n");
	dummy_boot_code[BOOTCODE_FAT32_SIZE-1] = 0;
	memcpy(bs.fat32.boot_code, dummy_boot_code, BOOTCODE_FAT32_SIZE);
    }
    else {
	memcpy(bs.oldfat.boot_code, dummy_boot_code, BOOTCODE_SIZE);
    }
    bs.boot_sign = CT_LE_W(BOOT_SIGN);
  }
  else {
    memcpy(bs.boot_jump, dummy_boot_jump_m68k, 2);
  }
  if (verbose >= 2)
    printf( "Boot jump code is %02x %02x\n",
	    bs.boot_jump[0], bs.boot_jump[1] );

  if (!reserved_sectors)
      reserved_sectors = (size_fat == 32) ? 32 : 1;
  else {
      if (size_fat == 32 && reserved_sectors < 2)
	  die("On FAT32 at least 2 reserved sectors are needed.");
  }
  bs.reserved = CT_LE_W(reserved_sectors);
  if (verbose >= 2)
    printf( "Using %d reserved sectors\n", reserved_sectors );
  bs.fats = (char) nr_fats;
  if (!atari_format || size_fat == 32)
    bs.hidden = CT_LE_L(0);
  else
    /* In Atari format, hidden is a 16 bit field */
    memset( &bs.hidden, 0, 2 );

  num_sectors = (ll_t)blocks*BLOCK_SIZE/sector_size;
  if (!atari_format) {
    unsigned fatlength12, fatlength16, fatlength32;
    unsigned maxclust12, maxclust16, maxclust32;
    unsigned clust12, clust16, clust32;
    int maxclustsize;

    fatdata = num_sectors - cdiv (root_dir_entries * 32, sector_size) -
	      reserved_sectors;

    if (sectors_per_cluster)
      bs.cluster_size = maxclustsize = sectors_per_cluster;
    else
      /* An initial guess for bs.cluster_size should already be set */
      maxclustsize = 128;

    if (verbose >= 2)
      printf( "%d sectors for FAT+data, starting with %d sectors/cluster\n",
	      fatdata, bs.cluster_size );
    do {
      if (verbose >= 2)
	printf( "Trying with %d sectors/cluster:\n", bs.cluster_size );

      /* The factor 2 below avoids cut-off errors for nr_fats == 1.
       * The "nr_fats*3" is for the reserved first two FAT entries */
      clust12 = 2*((ll_t) fatdata *sector_size + nr_fats*3) /
	(2*(int) bs.cluster_size * sector_size + nr_fats*3);
      fatlength12 = cdiv (((clust12+2) * 3 + 1) >> 1, sector_size);
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clust12 = (fatdata - nr_fats*fatlength12)/bs.cluster_size;
      maxclust12 = (fatlength12 * 2 * sector_size) / 3;
      if (maxclust12 > MAX_CLUST_12)
	maxclust12 = MAX_CLUST_12;
      if (verbose >= 2)
	printf( "FAT12: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n",
		clust12, fatlength12, maxclust12, MAX_CLUST_12 );
      if (clust12 > maxclust12-2) {
	clust12 = 0;
	if (verbose >= 2)
	  printf( "FAT12: too much clusters\n" );
      }

      clust16 = ((ll_t) fatdata *sector_size + nr_fats*4) /
	((int) bs.cluster_size * sector_size + nr_fats*2);
      fatlength16 = cdiv ((clust16+2) * 2, sector_size);
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clust16 = (fatdata - nr_fats*fatlength16)/bs.cluster_size;
      maxclust16 = (fatlength16 * sector_size) / 2;
      if (maxclust16 > MAX_CLUST_16)
	maxclust16 = MAX_CLUST_16;
      if (verbose >= 2)
	printf( "FAT16: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n",
		clust16, fatlength16, maxclust16, MAX_CLUST_16 );
      if (clust16 > maxclust16-2) {
	if (verbose >= 2)
	  printf( "FAT16: too much clusters\n" );
	clust16 = 0;
      }
      /* The < 4078 avoids that the filesystem will be misdetected as having a
       * 12 bit FAT. */
      if (clust16 < FAT12_THRESHOLD && !(size_fat_by_user && size_fat == 16)) {
	if (verbose >= 2)
	  printf( clust16 < FAT12_THRESHOLD ?
		  "FAT16: would be misdetected as FAT12\n" :
		  "FAT16: too much clusters\n" );
	clust16 = 0;
      }

      clust32 = ((ll_t) fatdata *sector_size + nr_fats*8) /
	((int) bs.cluster_size * sector_size + nr_fats*4);
      fatlength32 = cdiv ((clust32+2) * 4, sector_size);
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clust32 = (fatdata - nr_fats*fatlength32)/bs.cluster_size;
      maxclust32 = (fatlength32 * sector_size) / 4;
      if (maxclust32 > MAX_CLUST_32)
	maxclust32 = MAX_CLUST_32;
      if (verbose >= 2)
	printf( "FAT32: #clu=%u, fatlen=%u, maxclu=%u, limit=%u\n",
		clust32, fatlength32, maxclust32, MAX_CLUST_32 );
      if (clust32 > maxclust32) {
	clust32 = 0;
	if (verbose >= 2)
	  printf( "FAT32: too much clusters\n" );
      }

      if ((clust12 && (size_fat == 0 || size_fat == 12)) ||
	  (clust16 && (size_fat == 0 || size_fat == 16)) ||
	  (clust32 && size_fat == 32))
	break;

      bs.cluster_size <<= 1;
    } while (bs.cluster_size && bs.cluster_size <= maxclustsize);

    /* Use the optimal FAT size if not specified;
     * FAT32 is (not yet) choosen automatically */
    if (!size_fat) {
	size_fat = (clust16 > clust12) ? 16 : 12;
	if (verbose >= 2)
	  printf( "Choosing %d bits for FAT\n", size_fat );
    }

    switch (size_fat) {
      case 12:
	cluster_count = clust12;
	fat_length = fatlength12;
	bs.fat_length = CT_LE_W(fatlength12);
	memcpy(vi->fs_type, MSDOS_FAT12_SIGN, 8);
	break;

      case 16:
	if (clust16 < FAT12_THRESHOLD) {
	    if (size_fat_by_user) {
		fprintf( stderr, "WARNING: Not enough clusters for a "
			 "16 bit FAT! The filesystem will be\n"
			 "misinterpreted as having a 12 bit FAT without "
			 "mount option \"fat=16\".\n" );
	    }
	    else {
		fprintf( stderr, "This filesystem has an unfortunate size. "
			 "A 12 bit FAT cannot provide\n"
			 "enough clusters, but a 16 bit FAT takes up a little "
			 "bit more space so that\n"
			 "the total number of clusters becomes less than the "
			 "threshold value for\n"
			 "distinction between 12 and 16 bit FATs.\n" );
		die( "Make the file system a bit smaller manually." );
	    }
	}
	cluster_count = clust16;
	fat_length = fatlength16;
	bs.fat_length = CT_LE_W(fatlength16);
	memcpy(vi->fs_type, MSDOS_FAT16_SIGN, 8);
	break;

      case 32:
	cluster_count = clust32;
	fat_length = fatlength32;
	bs.fat_length = CT_LE_W(0);
	bs.fat32.fat32_length = CT_LE_L(fatlength32);
	memcpy(vi->fs_type, MSDOS_FAT32_SIGN, 8);
	break;

      default:
	die("FAT not 12, 16 or 32 bits");
    }
  }
  else {
    unsigned clusters, maxclust;

    /* GEMDOS always uses a 12 bit FAT on floppies, and always a 16 bit FAT on
     * hard disks. So use 12 bit if the size of the file system suggests that
     * this fs is for a floppy disk, if the user hasn't explicitly requested a
     * size.
     */
    if (!size_fat)
      size_fat = (num_sectors == 1440 || num_sectors == 2400 ||
		  num_sectors == 2880 || num_sectors == 5760) ? 12 : 16;
    if (verbose >= 2)
      printf( "Choosing %d bits for FAT\n", size_fat );

    /* Atari format: cluster size should be 2, except explicitly requested by
     * the user, since GEMDOS doesn't like other cluster sizes very much.
     * Instead, tune the sector size for the FS to fit.
     */
    bs.cluster_size = sectors_per_cluster ? sectors_per_cluster : 2;
    if (!sector_size_set) {
      while( num_sectors > GEMDOS_MAX_SECTORS ) {
	num_sectors >>= 1;
	sector_size <<= 1;
      }
    }
    if (verbose >= 2)
      printf( "Sector size must be %d to have less than %d log. sectors\n",
	      sector_size, GEMDOS_MAX_SECTORS );

    /* Check if there are enough FAT indices for how much clusters we have */
    do {
      fatdata = num_sectors - cdiv (root_dir_entries * 32, sector_size) -
		reserved_sectors;
      /* The factor 2 below avoids cut-off errors for nr_fats == 1 and
       * size_fat == 12
       * The "2*nr_fats*size_fat/8" is for the reserved first two FAT entries
       */
      clusters = (2*((ll_t)fatdata*sector_size - 2*nr_fats*size_fat/8)) /
		 (2*((int)bs.cluster_size*sector_size + nr_fats*size_fat/8));
      fat_length = cdiv( (clusters+2)*size_fat/8, sector_size );
      /* Need to recalculate number of clusters, since the unused parts of the
       * FATS and data area together could make up space for an additional,
       * not really present cluster. */
      clusters = (fatdata - nr_fats*fat_length)/bs.cluster_size;
      maxclust = (fat_length*sector_size*8)/size_fat;
      if (verbose >= 2)
	printf( "ss=%d: #clu=%d, fat_len=%d, maxclu=%d\n",
		sector_size, clusters, fat_length, maxclust );

      /* last 10 cluster numbers are special (except FAT32: 4 high bits rsvd);
       * first two numbers are reserved */
      if (maxclust <= (size_fat == 32 ? MAX_CLUST_32 : (1<<size_fat)-0x10) &&
	  clusters <= maxclust-2)
	break;
      if (verbose >= 2)
	printf( clusters > maxclust-2 ?
		"Too many clusters\n" : "FAT too big\n" );

      /* need to increment sector_size once more to  */
      if (sector_size_set)
	  die( "With this sector size, the maximum number of FAT entries "
	       "would be exceeded." );
      num_sectors >>= 1;
      sector_size <<= 1;
    } while( sector_size <= GEMDOS_MAX_SECTOR_SIZE );

    if (sector_size > GEMDOS_MAX_SECTOR_SIZE)
      die( "Would need a sector size > 16k, which GEMDOS can't work with");

    cluster_count = clusters;
    if (size_fat != 32)
	bs.fat_length = CT_LE_W(fat_length);
    else {
	bs.fat_length = 0;
	bs.fat32.fat32_length = CT_LE_L(fat_length);
    }
  }

  bs.sector_size[0] = (char) (sector_size & 0x00ff);
  bs.sector_size[1] = (char) ((sector_size & 0xff00) >> 8);

  if (size_fat == 32)
    {
      /* set up additional FAT32 fields */
      bs.fat32.flags = CT_LE_W(0);
      bs.fat32.version[0] = 0;
      bs.fat32.version[1] = 0;
      bs.fat32.root_cluster = CT_LE_L(2);
      bs.fat32.info_sector = CT_LE_W(1);
      if (!backup_boot)
	backup_boot = (reserved_sectors >= 7) ? 6 :
		      (reserved_sectors >= 2) ? reserved_sectors-1 : 0;
      else
	{
	  if (backup_boot == 1)
	    die("Backup boot sector must be after sector 1");
	  else if (backup_boot >= reserved_sectors)
	    die("Backup boot sector must be a reserved sector");
	}
      if (verbose >= 2)
	printf( "Using sector %d as backup boot sector (0 = none)\n",
		backup_boot );
      bs.fat32.backup_boot = CT_LE_W(backup_boot);
      memset( &bs.fat32.reserved2, 0, sizeof(bs.fat32.reserved2) );
    }

  if (atari_format) {
      /* Just some consistency checks */
      if (num_sectors >= GEMDOS_MAX_SECTORS)
	  die( "GEMDOS can't handle more than 65531 sectors" );
      else if (num_sectors >= OLDGEMDOS_MAX_SECTORS)
	  printf( "Warning: More than 32765 sector need TOS 1.04 "
		  "or higher.\n" );
  }
  if (num_sectors >= 65536)
    {
      bs.sectors[0] = (char) 0;
      bs.sectors[1] = (char) 0;
      bs.total_sect = CT_LE_L(num_sectors);
    }
  else
    {
      bs.sectors[0] = (char) (num_sectors & 0x00ff);
      bs.sectors[1] = (char) ((num_sectors & 0xff00) >> 8);
      if (!atari_format)
	  bs.total_sect = CT_LE_L(0);
    }

  if (!atari_format)
    vi->ext_boot_sign = MSDOS_EXT_SIGN;

  if (!cluster_count)
    {
      if (sectors_per_cluster)	/* If yes, die if we'd spec'd sectors per cluster */
	die ("Too many clusters for file system - try more sectors per cluster");
      else
	die ("Attempting to create a too large file system");
    }


  /* The two following vars are in hard sectors, i.e. 512 byte sectors! */
  start_data_sector = (reserved_sectors + nr_fats * fat_length) *
		      (sector_size/HARD_SECTOR_SIZE);
  start_data_block = (start_data_sector + SECTORS_PER_BLOCK - 1) /
		     SECTORS_PER_BLOCK;

  if (blocks < start_data_block + 32)	/* Arbitrary undersize file system! */
    die ("Too few blocks for viable file system");

  if (verbose)
    {
      printf("%s has %d head%s and %d sector%s per track,\n",
	     device_name, CF_LE_W(bs.heads), (CF_LE_W(bs.heads) != 1) ? "s" : "",
	     CF_LE_W(bs.secs_track), (CF_LE_W(bs.secs_track) != 1) ? "s" : "");
      printf("logical sector size is %d,\n",sector_size);
      printf("using 0x%02x media descriptor, with %d sectors;\n",
	     (int) (bs.media), num_sectors);
      printf("file system has %d %d-bit FAT%s and %d sector%s per cluster.\n",
	     (int) (bs.fats), size_fat, (bs.fats != 1) ? "s" : "",
	     (int) (bs.cluster_size), (bs.cluster_size != 1) ? "s" : "");
      printf ("FAT size is %d sector%s, and provides %d cluster%s.\n",
	      fat_length, (fat_length != 1) ? "s" : "",
	      cluster_count, (cluster_count != 1) ? "s" : "");
      if (size_fat != 32)
	printf ("Root directory contains %d slots.\n",
		(int) (bs.dir_entries[0]) + (int) (bs.dir_entries[1]) * 256);
      printf ("Volume ID is %08lx, ", volume_id &
	      (atari_format ? 0x00ffffff : 0xffffffff));
      if ( strcmp(volume_name, "           ") )
	printf("volume label %s.\n", volume_name);
      else
	printf("no volume label.\n");
    }

  /* Make the file allocation tables! */

  if ((fat = (unsigned char *) malloc (fat_length * sector_size)) == NULL)
    die ("unable to allocate space for FAT image in memory");

  memset( fat, 0, fat_length * sector_size );

  mark_FAT_cluster (0, 0xffffffff);	/* Initial fat entries */
  mark_FAT_cluster (1, 0xffffffff);
  fat[0] = (unsigned char) bs.media;	/* Put media type in first byte! */
  if (size_fat == 32) {
    /* Mark cluster 2 as EOF (used for root dir) */
    mark_FAT_cluster (2, FAT_EOF);
  }

  /* Make the root directory entries */

  size_root_dir = (size_fat == 32) ?
		  bs.cluster_size*sector_size :
		  (((int)bs.dir_entries[1]*256+(int)bs.dir_entries[0]) *
		   sizeof (struct msdos_dir_entry));
  if ((root_dir = (struct msdos_dir_entry *) malloc (size_root_dir)) == NULL)
    {
      free (fat);		/* Tidy up before we die! */
      die ("unable to allocate space for root directory in memory");
    }

  memset(root_dir, 0, size_root_dir);
  if ( memcmp(volume_name, "           ", 11) )
    {
      struct msdos_dir_entry *de = &root_dir[0];
      memcpy(de->name, volume_name, 11);
      de->attr = ATTR_VOLUME;
      ctime = localtime(&create_time);
      de->time = CT_LE_W((unsigned short)((ctime->tm_sec >> 1) +
			  (ctime->tm_min << 5) + (ctime->tm_hour << 11)));
      de->date = CT_LE_W((unsigned short)(ctime->tm_mday +
					  ((ctime->tm_mon+1) << 5) +
					  ((ctime->tm_year-80) << 9)));
      de->ctime_ms = 0;
      de->ctime = de->time;
      de->cdate = de->date;
      de->adate = de->date;
      de->starthi = CT_LE_W(0);
      de->start = CT_LE_W(0);
      de->size = CT_LE_L(0);
    }

  if (size_fat == 32) {
    /* For FAT32, create an info sector */
    struct fat32_fsinfo *info;

    if (!(info_sector = malloc( sector_size )))
      die("Out of memory");
    memset(info_sector, 0, sector_size);
    /* fsinfo structure is at offset 0x1e0 in info sector by observation */
    info = (struct fat32_fsinfo *)(info_sector + 0x1e0);

    /* Info sector magic */
    info_sector[0] = 'R';
    info_sector[1] = 'R';
    info_sector[2] = 'a';
    info_sector[3] = 'A';

    /* Magic for fsinfo structure */
    info->signature = CT_LE_L(0x61417272);
    /* We've allocated cluster 2 for the root dir. */
    info->free_clusters = CT_LE_L(cluster_count - 1);
    info->next_cluster = CT_LE_L(2);

    /* Info sector also must have boot sign */
    *(__u16 *)(info_sector + 0x1fe) = CT_LE_W(BOOT_SIGN);
  }

  if (!(blank_sector = malloc( sector_size )))
      die( "Out of memory" );
  memset(blank_sector, 0, sector_size);
}


/* Write the new filesystem's data tables to wherever they're going to end up! */

#define error(str)				\
  do {						\
    free (fat);					\
    if (info_sector) free (info_sector);	\
    free (root_dir);				\
    die (str);					\
  } while(0)

#define seekto(pos,errstr)						\
  do {									\
    loff_t __pos = (pos);						\
    if (llseek (dev, __pos, SEEK_SET) != __pos)				\
	error ("seek to " errstr " failed whilst writing tables");	\
  } while(0)

#define writebuf(buf,size,errstr)			\
  do {							\
    int __size = (size);				\
    if (write (dev, buf, __size) != __size)		\
	error ("failed whilst writing " errstr);	\
  } while(0)


static void
write_tables (void)
{
  int x;
  int fat_length;
#ifdef _WIN32
  int blk;
#endif

  fat_length = (size_fat == 32) ?
	       CF_LE_L(bs.fat32.fat32_length) : CF_LE_W(bs.fat_length);

  seekto( 0, "start of device" );
  /* clear all reserved sectors */
  for( x = 0; x < reserved_sectors; ++x )
    writebuf( blank_sector, sector_size, "reserved sector" );
  /* seek back to sector 0 and write the boot sector */
  seekto( 0, "boot sector" );
  writebuf( (char *) &bs, sizeof (struct msdos_boot_sector), "boot sector" );
  /* on FAT32, write the info sector and backup boot sector */
  if (size_fat == 32)
    {
      seekto( CF_LE_W(bs.fat32.info_sector)*sector_size, "info sector" );
      writebuf( info_sector, 512, "info sector" );
      if (backup_boot != 0)
	{
	  seekto( backup_boot*sector_size, "backup boot sector" );
	  writebuf( (char *) &bs, sizeof (struct msdos_boot_sector),
		    "backup boot sector" );
	}
    }
  /* seek to start of FATS and write them all */
  seekto( reserved_sectors*sector_size, "first FAT" );
  for (x = 1; x <= nr_fats; x++)
#ifdef _WIN32
	  /*
	   * WIN32 appearently has problems writing very large chunks directly
	   * to disk devices. To not produce errors because of resource shortages
	   * split up the write in sector size chunks.
	   */
	  for (blk = 0; blk < fat_length; blk++)
		  writebuf(fat+blk*sector_size, sector_size, "FAT");
#else
    writebuf( fat, fat_length * sector_size, "FAT" );
#endif
  /* Write the root directory directly after the last FAT. This is the root
   * dir area on FAT12/16, and the first cluster on FAT32. */
  writebuf( (char *) root_dir, size_root_dir, "root directory" );

  if (info_sector) free( info_sector );
  free (root_dir);   /* Free up the root directory space from setup_tables */
  free (fat);  /* Free up the fat table space reserved during setup_tables */
}


/* Report the command usage and return a failure error code */

void
usage (void)
{
  fatal_error("\
Usage: mkdosfs [-A] [-c] [-C] [-v] [-I] [-l bad-block-file] [-b backup-boot-sector]\n\
       [-m boot-msg-file] [-n volume-name] [-i volume-id]\n\
       [-s sectors-per-cluster] [-S logical-sector-size] [-f number-of-FATs]\n\
       [-F fat-size] [-r root-dir-entries] [-R reserved-sectors]\n\
       /dev/name [blocks]\n");
}

/*
 * ++roman: On m68k, check if this is an Atari; if yes, turn on Atari variant
 * of MS-DOS filesystem by default.
 */
static void check_atari( void )
{
#ifdef __mc68000__
    FILE *f;
    char line[128], *p;

    if (!(f = fopen( "/proc/hardware", "r" ))) {
	perror( "/proc/hardware" );
	return;
    }

    while( fgets( line, sizeof(line), f ) ) {
	if (strncmp( line, "Model:", 6 ) == 0) {
	    p = line + 6;
	    p += strspn( p, " \t" );
	    if (strncmp( p, "Atari ", 6 ) == 0)
		atari_format = 1;
	    break;
	}
    }
    fclose( f );
#endif
}

/* The "main" entry point into the utility - we pick up the options and attempt to process them in some sort of sensible
   way.  In the event that some/all of the options are invalid we need to tell the user so that something can be done! */

int
main (int argc, char **argv)
{
  int c;
  char *tmp;
  char *listfile = NULL;
  FILE *msgfile;
#ifdef _WIN32
  static char dev_buf[] = "\\\\.\\X:";
#else
  struct stat statbuf;
#endif
  int i = 0, pos, ch;
  int create = 0;

  if (argc && *argv) {		/* What's the program name? */
    char *p;
    program_name = *argv;
#ifdef _WIN32
    if ((p = strrchr( program_name, '\\' )))
#else
    if ((p = strrchr( program_name, '/' )))
#endif
	program_name = p+1;
  }

  time(&create_time);
  volume_id = (long)create_time;	/* Default volume ID = creation time */
  check_atari();

  printf ("%s " VERSION " (" VERSION_DATE ")\n"
#ifdef _WIN32
	  "Win32 port by Jens-Uwe Mager <jum@anubis.han.de>\n"
#endif
	   , program_name);

  while ((c = getopt (argc, argv, "AcCf:F:Ii:l:m:n:r:R:s:S:v")) != EOF)
    /* Scan the command line for options */
    switch (c)
      {
      case 'A':		/* toggle Atari format */
	atari_format = !atari_format;
	break;

      case 'b':		/* b : location of backup boot sector */
	backup_boot = (int) strtol (optarg, &tmp, 0);
	if (*tmp || backup_boot < 2 || backup_boot > 0xffff)
	  {
	    printf ("Bad location for backup boot sector : %s\n", optarg);
	    usage ();
	  }
	break;

      case 'c':		/* c : Check FS as we build it */
	check = TRUE;
	break;

      case 'C':		/* C : Create a new file */
	create = TRUE;
	break;

      case 'f':		/* f : Choose number of FATs */
	nr_fats = (int) strtol (optarg, &tmp, 0);
	if (*tmp || nr_fats < 1 || nr_fats > 4)
	  {
	    printf ("Bad number of FATs : %s\n", optarg);
	    usage ();
	  }
	break;

      case 'F':		/* F : Choose FAT size */
	size_fat = (int) strtol (optarg, &tmp, 0);
	if (*tmp || (size_fat != 12 && size_fat != 16 && size_fat != 32))
	  {
	    printf ("Bad FAT type : %s\n", optarg);
	    usage ();
	  }
	size_fat_by_user = 1;
	break;

      case 'I':
	ignore_full_disk = 1;
	break;

      case 'i':		/* i : specify volume ID */
	volume_id = strtol(optarg, &tmp, 16);
	if ( *tmp )
	  {
	    printf("Volume ID must be a hexadecimal number\n");
	    usage();
	  }
	break;

      case 'l':		/* l : Bad block filename */
	listfile = optarg;
	break;

      case 'm':		/* m : Set boot message */
	if ( strcmp(optarg, "-") )
	  {
	    msgfile = fopen(optarg, "r");
	    if ( !msgfile )
	      perror(optarg);
	  }
	else
	  msgfile = stdin;

	if ( msgfile )
	  {
	    /* The boot code ends at offset 448 and needs a null terminator */
	    i = MESSAGE_OFFSET;
	    pos = 0;		/* We are at beginning of line */
	    do
	      {
		ch = getc(msgfile);
		switch (ch)
		  {
		  case '\r':	/* Ignore CRs */
		  case '\0':	/* and nulls */
		    break;

		  case '\n':	/* LF -> CR+LF if necessary */
		    if ( pos )	/* If not at beginning of line */
		      {
			dummy_boot_code[i++] = '\r';
			pos = 0;
		      }
		    dummy_boot_code[i++] = '\n';
		    break;

		  case '\t':	/* Expand tabs */
		    do
		      {
			dummy_boot_code[i++] = ' ';
			pos++;
		      }
		    while ( pos % 8 && i < BOOTCODE_SIZE-1 );
		    break;

		  case EOF:
		    dummy_boot_code[i++] = '\0'; /* Null terminator */
		    break;

		  default:
		    dummy_boot_code[i++] = ch; /* Store character */
		    pos++;	/* Advance position */
		    break;
		  }
	      }
	    while ( ch != EOF && i < BOOTCODE_SIZE-1 );

	    /* Fill up with zeros */
	    while( i < BOOTCODE_SIZE-1 )
		dummy_boot_code[i++] = '\0';
	    dummy_boot_code[BOOTCODE_SIZE-1] = '\0'; /* Just in case */

	    if ( ch != EOF )
	      printf ("Warning: message too long; truncated\n");

	    if ( msgfile != stdin )
	      fclose(msgfile);
	  }
	break;

      case 'n':		/* n : Volume name */
	sprintf(volume_name, "%-11.11s", optarg);
	break;

      case 'r':		/* r : Root directory entries */
	root_dir_entries = (int) strtol (optarg, &tmp, 0);
	if (*tmp || root_dir_entries < 16 || root_dir_entries > 32768)
	  {
	    printf ("Bad number of root directory entries : %s\n", optarg);
	    usage ();
	  }
	break;

      case 'R':		/* R : number of reserved sectors */
	reserved_sectors = (int) strtol (optarg, &tmp, 0);
	if (*tmp || reserved_sectors < 1 || reserved_sectors > 0xffff)
	  {
	    printf ("Bad number of reserved sectors : %s\n", optarg);
	    usage ();
	  }
	break;

      case 's':		/* s : Sectors per cluster */
	sectors_per_cluster = (int) strtol (optarg, &tmp, 0);
	if (*tmp || (sectors_per_cluster != 1 && sectors_per_cluster != 2
		     && sectors_per_cluster != 4 && sectors_per_cluster != 8
		   && sectors_per_cluster != 16 && sectors_per_cluster != 32
		&& sectors_per_cluster != 64 && sectors_per_cluster != 128))
	  {
	    printf ("Bad number of sectors per cluster : %s\n", optarg);
	    usage ();
	  }
	break;

      case 'S':		/* S : Sector size */
	sector_size = (int) strtol (optarg, &tmp, 0);
	if (*tmp || (sector_size != 512 && sector_size != 1024 &&
		     sector_size != 2048 && sector_size != 4096 &&
		     sector_size != 8192 && sector_size != 16384 &&
		     sector_size != 32768))
	  {
	    printf ("Bad logical sector size : %s\n", optarg);
	    usage ();
	  }
	sector_size_set = 1;
	break;

      case 'v':		/* v : Verbose execution */
	++verbose;
	break;

      default:
	printf( "Unknown option: %c\n", c );
	usage ();
      }

  if (optind >= argc)
	  usage();
  device_name = argv[optind];	/* Determine the number of blocks in the FS */
#ifdef _WIN32
  if (device_name[1] == ':' && device_name[2] == '\0') {
	  dev_buf[4] = device_name[0];
	  device_name = dev_buf;
	  is_device = 1;
  }
#endif
  if (!create)
    i = count_blocks (device_name); /*  Have a look and see! */
  if (optind == argc - 2)	/*  Either check the user specified number */
    {
      blocks = (int) strtol (argv[optind + 1], &tmp, 0);
      if (!create && blocks != i)
	{
	  fprintf (stderr, "Warning: block count mismatch: ");
	  fprintf (stderr, "found %d but assuming %d.\n",i,blocks);
	}
    }
  else if (optind == argc - 1)	/*  Or use value found */
    {
      if (create)
	die( "Need intended size with -C." );
      blocks = i;
      tmp = "";
    }
  else
    usage ();
  if (*tmp)
    {
      printf ("Bad block count : %s\n", argv[optind + 1]);
      usage ();
    }

  if (check && listfile)	/* Auto and specified bad block handling are mutually */
    die ("-c and -l are incompatible");		/* exclusive of each other! */

  if (!create) {
    check_mount (device_name);	/* Is the device already mounted? */
    dev = open (device_name, O_RDWR|O_SHARED);	/* Is it a suitable device to build the FS on? */
    if (dev < 0)
      die ("unable to open %s");
#ifdef _WIN32
	if (is_device) {
		if (fsctl(dev, FSCTL_LOCK_VOLUME) == -1)
			die("unable to lock %s");
	}
#endif
  }
  else {
      loff_t offset = blocks*BLOCK_SIZE - 1;
      char null = 0;
      /* create the file */
      dev = open( device_name, O_RDWR|O_CREAT|O_TRUNC, 0775 );
      if (dev < 0)
	die("unable to create %s");
      /* seek to the intended end-1, and write one byte. this creates a
       * sparse-as-possible file of appropriate size. */
      if (llseek( dev, offset, SEEK_SET ) != offset)
	die( "seek failed" );
      if (write( dev, &null, 1 ) < 0)
	die( "write failed" );
      if (llseek( dev, 0, SEEK_SET ) != 0)
	die( "seek failed" );
  }

#ifdef _WIN32
  if (!is_device)
	  check = 0;
  establish_params();
#else
  if (fstat (dev, &statbuf) < 0)
    die ("unable to stat %s");
  if (!S_ISBLK (statbuf.st_mode)) {
    statbuf.st_rdev = 0;
    check = 0;
  }
  else
    /*
     * Ignore any 'full' fixed disk devices, if -I is not given.
     * On a MO-disk one doesn't need partitions.  The filesytem can go
     * directly to the whole disk.  Under other OSes this is known as
     * the 'superfloppy' format.  As I don't know how to find out if
     * this is a MO disk I introduce a -I (ignore) switch.  -Joey
     */
    if (!ignore_full_disk && (
	(statbuf.st_rdev & 0xff3f) == 0x0300 || /* hda, hdb */
	(statbuf.st_rdev & 0xff0f) == 0x0800 || /* sd */
	(statbuf.st_rdev & 0xff3f) == 0x0d00 || /* xd */
	(statbuf.st_rdev & 0xff3f) == 0x1600 )  /* hdc, hdd */
	)
      die ("Will not try to make filesystem on '%s'");

  establish_params (statbuf.st_rdev,statbuf.st_size);
                                /* Establish the media parameters */
#endif

  setup_tables ();		/* Establish the file system tables */

  if (check)			/* Determine any bad block locations and mark them */
    check_blocks ();
  else if (listfile)
    get_list_blocks (listfile);

  write_tables ();		/* Write the file system tables away! */

#ifdef _WIN32
	if (is_device) {
		if (fsctl(dev, FSCTL_DISMOUNT_VOLUME) == -1)
			die("unable to dismount %s");
		if (fsctl(dev, FSCTL_UNLOCK_VOLUME) == -1)
			die("unable to unlock %s");
	}
#endif
  exit (0);			/* Terminate with no errors! */
}


/* That's All Folks */
/* Local Variables: */
/* tab-width: 8     */
/* End:             */
