/*
 * PROJECT:          Mke2fs
 * FILE:             Mke2fs.h
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

#ifndef __MKE2FS__INCLUDE__
#define __MKE2FS__INCLUDE__


/* INCLUDES **************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <ntddk.h>
#include <ntdddisk.h>

#include <napi/teb.h>

#include "time.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"

#include "types.h"
#include "ext2_fs.h"

#include "getopt.h"

#define NTSYSAPI

/* DEFINITIONS ***********************************************************/

#define SECTOR_SIZE (Ext2Sys->DiskGeometry.BytesPerSector)

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;
#endif /* GUID_DEFINED */

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#ifndef uuid_t
#define uuid_t UUID
#endif
#endif

#ifndef bool
#define bool    BOOLEAN
#endif

#ifndef true
#define true    TRUE
#endif

#ifndef false
#define false   FALSE
#endif


#define EXT2_CHECK_MAGIC(struct, code) \
      if ((struct)->magic != (code)) return (code)

/*
 * ext2fs_scan flags
 */
#define EXT2_SF_CHK_BADBLOCKS   0x0001
#define EXT2_SF_BAD_INODE_BLK   0x0002
#define EXT2_SF_BAD_EXTRA_BYTES 0x0004
#define EXT2_SF_SKIP_MISSING_ITABLE 0x0008

/*
 * ext2fs_check_if_mounted flags
 */
#define EXT2_MF_MOUNTED     1
#define EXT2_MF_ISROOT      2
#define EXT2_MF_READONLY    4
#define EXT2_MF_SWAP        8

/*
 * Ext2/linux mode flags.  We define them here so that we don't need
 * to depend on the OS's sys/stat.h, since we may be compiling on a
 * non-Linux system.
 */

#define LINUX_S_IFMT  00170000
#define LINUX_S_IFSOCK 0140000
#define LINUX_S_IFLNK    0120000
#define LINUX_S_IFREG  0100000
#define LINUX_S_IFBLK  0060000
#define LINUX_S_IFDIR  0040000
#define LINUX_S_IFCHR  0020000
#define LINUX_S_IFIFO  0010000
#define LINUX_S_ISUID  0004000
#define LINUX_S_ISGID  0002000
#define LINUX_S_ISVTX  0001000

#define LINUX_S_IRWXU 00700
#define LINUX_S_IRUSR 00400
#define LINUX_S_IWUSR 00200
#define LINUX_S_IXUSR 00100

#define LINUX_S_IRWXG 00070
#define LINUX_S_IRGRP 00040
#define LINUX_S_IWGRP 00020
#define LINUX_S_IXGRP 00010

#define LINUX_S_IRWXO 00007
#define LINUX_S_IROTH 00004
#define LINUX_S_IWOTH 00002
#define LINUX_S_IXOTH 00001

#define LINUX_S_ISLNK(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFLNK)
#define LINUX_S_ISREG(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFREG)
#define LINUX_S_ISDIR(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFDIR)
#define LINUX_S_ISCHR(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFCHR)
#define LINUX_S_ISBLK(m)    (((m) & LINUX_S_IFMT) == LINUX_S_IFBLK)
#define LINUX_S_ISFIFO(m)   (((m) & LINUX_S_IFMT) == LINUX_S_IFIFO)
#define LINUX_S_ISSOCK(m)   (((m) & LINUX_S_IFMT) == LINUX_S_IFSOCK)


#define EXT2_FIRST_INODE(s) EXT2_FIRST_INO(s)

typedef struct _ext2fs_bitmap {
    __u32       start, end;
    __u32       real_end;
    char*       bitmap;
} EXT2_BITMAP, *PEXT2_BITMAP;

typedef EXT2_BITMAP EXT2_GENERIC_BITMAP, *PEXT2_GENERIC_BITMAP;
typedef EXT2_BITMAP EXT2_INODE_BITMAP, *PEXT2_INODE_BITMAP;
typedef EXT2_BITMAP EXT2_BLOCK_BITMAP, *PEXT2_BLOCK_BITMAP;

typedef struct ext2_acl_entry   EXT2_ACL_ENTRY, *PEXT2_ACL_ENTRY;
typedef struct ext2_acl_header  EXT2_ACL_HEADER, *PEXT2_ACL_HEADER;
typedef struct ext2_dir_entry   EXT2_DIR_ENTRY, *PEXT2_DIR_ENTRY;
typedef struct ext2_dir_entry_2 EXT2_DIR_ENTRY2, *PEXT2_DIR_ENTRY2;
typedef struct ext2_dx_countlimit   EXT2_DX_CL, *PEXT2_DX_CL;
typedef struct ext2_dx_entry    EXT2_DX_ENTRY, *PEXT2_DX_ENTRY;
typedef struct ext2_dx_root_info    EXT2_DX_RI, *PEXT2_DX_RI;
typedef struct ext2_inode   EXT2_INODE, *PEXT2_INODE;
typedef struct ext2_group_desc  EXT2_GROUP_DESC, *PEXT2_GROUP_DESC;
typedef struct ext2_super_block EXT2_SUPER_BLOCK, *PEXT2_SUPER_BLOCK;

/*
 * Badblocks list
 */
struct ext2_struct_badblocks_list {
    int     num;
    int     size;
    ULONG   *list;
    int     badblocks_flags;
};

typedef struct ext2_struct_badblocks_list EXT2_BADBLK_LIST, *PEXT2_BADBLK_LIST;

typedef struct _ext2_filesys
{
    int                 flags;
    int                 blocksize;
    int                 fragsize;
    ULONG               group_desc_count;
    unsigned long       desc_blocks;
    PEXT2_GROUP_DESC    group_desc;
    PEXT2_SUPER_BLOCK   ext2_sb;
    unsigned long       inode_blocks_per_group;
    PEXT2_INODE_BITMAP  inode_map;
    PEXT2_BLOCK_BITMAP  block_map;

    EXT2_BADBLK_LIST    badblocks;
/*
    ext2_dblist         dblist;
*/
    __u32               stride; /* for mke2fs */

    __u32               umask;

    /*
     * Reserved for future expansion
     */
    __u32               reserved[8];

    /*
     * Reserved for the use of the calling application.
     */
    void *              priv_data;

    HANDLE              MediaHandle;

    DISK_GEOMETRY       DiskGeometry;

    PARTITION_INFORMATION   PartInfo;

} EXT2_FILESYS, *PEXT2_FILESYS;

// Block Description List
typedef struct _EXT2_BDL {
    LONGLONG    Lba;
    ULONG       Offset;
    ULONG       Length;
} EXT2_BDL, *PEXT2_BDL;

/*
 * Where the master copy of the superblock is located, and how big
 * superblocks are supposed to be.  We define SUPERBLOCK_SIZE because
 * the size of the superblock structure is not necessarily trustworthy
 * (some versions have the padding set up so that the superblock is
 * 1032 bytes long).
 */
#define SUPERBLOCK_OFFSET   1024
#define SUPERBLOCK_SIZE     1024


bool create_bad_block_inode(PEXT2_FILESYS fs, PEXT2_BADBLK_LIST bb_list);


//
// Definitions
//

#define FSCTL_REQUEST_OPLOCK_LEVEL_1    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_OPLOCK_LEVEL_2    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_BATCH_OPLOCK      CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_ACKNOWLEDGE  CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPBATCH_ACK_CLOSE_PENDING CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_NOTIFY       CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LOCK_VOLUME               CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)
// decommissioned fsctl value                                              9
#define FSCTL_IS_VOLUME_MOUNTED         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_PATHNAME_VALID         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 11, METHOD_BUFFERED, FILE_ANY_ACCESS) // PATHNAME_BUFFER,
#define FSCTL_MARK_VOLUME_DIRTY         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
// decommissioned fsctl value                                             13
#define FSCTL_QUERY_RETRIEVAL_POINTERS  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 14,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_GET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 16, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
// decommissioned fsctl value                                             17
// decommissioned fsctl value                                             18
#define FSCTL_MARK_AS_SYSTEM_HIVE       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 19,  METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_ACK_NO_2     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_INVALIDATE_VOLUMES        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_FAT_BPB             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 22, METHOD_BUFFERED, FILE_ANY_ACCESS) // , FSCTL_QUERY_FAT_BPB_BUFFER
#define FSCTL_REQUEST_FILTER_OPLOCK     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 23, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_FILESYSTEM_GET_STATISTICS CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 24, METHOD_BUFFERED, FILE_ANY_ACCESS) // , FILESYSTEM_STATISTICS


//
//  Disk I/O Routines
//

NTSYSAPI
NTSTATUS
NTAPI
NtReadFile(HANDLE FileHandle,
    HANDLE Event OPTIONAL,
    PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    PVOID ApcContext OPTIONAL,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer,
    ULONG Length,
    PLARGE_INTEGER ByteOffset OPTIONAL,
    PULONG Key OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
NtWriteFile(HANDLE FileHandle,
    HANDLE Event OPTIONAL,
    PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    PVOID ApcContext OPTIONAL,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer,
    ULONG Length,
    PLARGE_INTEGER ByteOffset OPTIONAL,
    PULONG Key OPTIONAL);

NTSYSAPI
NTSTATUS
NTAPI
NtClose(HANDLE Handle);

NTSYSAPI
NTSTATUS
NTAPI
NtCreateFile(PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize OPTIONAL,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer OPTIONAL,
    ULONG EaLength);


NTSYSAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    IN HANDLE  FileHandle,
    IN HANDLE  Event,
    IN PIO_APC_ROUTINE  ApcRoutine,
    IN PVOID  ApcContext,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN ULONG  IoControlCode,
    IN PVOID  InputBuffer,
    IN ULONG  InputBufferLength,
    OUT PVOID  OutputBuffer,
    OUT ULONG  OutputBufferLength
    ); 

NTSYSAPI
NTSTATUS
NTAPI
NtFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
);


NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
    IN HANDLE  FileHandle,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    OUT PVOID  FileInformation,
    IN ULONG  Length,
    IN FILE_INFORMATION_CLASS  FileInformationClass
    );

//
// Bitmap Routines
//


//
//  BitMap routines.  The following structure, routines, and macros are
//  for manipulating bitmaps.  The user is responsible for allocating a bitmap
//  structure (which is really a header) and a buffer (which must be longword
//  aligned and multiple longwords in size).
//

//
//  The following routine initializes a new bitmap.  It does not alter the
//  data currently in the bitmap.  This routine must be called before
//  any other bitmap routine/macro.
//

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap (
    PRTL_BITMAP BitMapHeader,
    PULONG BitMapBuffer,
    ULONG SizeOfBitMap
    );

//
//  The following two routines either clear or set all of the bits
//  in a bitmap.
//

NTSYSAPI
VOID
NTAPI
RtlClearAllBits (
    PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
VOID
NTAPI
RtlSetAllBits (
    PRTL_BITMAP BitMapHeader
    );

//
//  The following two routines locate a contiguous region of either
//  clear or set bits within the bitmap.  The region will be at least
//  as large as the number specified, and the search of the bitmap will
//  begin at the specified hint index (which is a bit index within the
//  bitmap, zero based).  The return value is the bit index of the located
//  region (zero based) or -1 (i.e., 0xffffffff) if such a region cannot
//  be located
//

NTSYSAPI
ULONG
NTAPI
RtlFindClearBits (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindSetBits (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

//
//  The following two routines locate a contiguous region of either
//  clear or set bits within the bitmap and either set or clear the bits
//  within the located region.  The region will be as large as the number
//  specified, and the search for the region will begin at the specified
//  hint index (which is a bit index within the bitmap, zero based).  The
//  return value is the bit index of the located region (zero based) or
//  -1 (i.e., 0xffffffff) if such a region cannot be located.  If a region
//  cannot be located then the setting/clearing of the bitmap is not performed.
//

NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

//
//  The following two routines clear or set bits within a specified region
//  of the bitmap.  The starting index is zero based.
//

NTSYSAPI
VOID
NTAPI
RtlClearBits (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToClear
    );

NTSYSAPI
VOID
NTAPI
RtlSetBits (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToSet
    );

//
//  The following routine locates a set of contiguous regions of clear
//  bits within the bitmap.  The caller specifies whether to return the
//  longest runs or just the first found lcoated.  The following structure is
//  used to denote a contiguous run of bits.  The two routines return an array
//  of this structure, one for each run located.
//

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns (
    PRTL_BITMAP BitMapHeader,
    PRTL_BITMAP_RUN RunArray,
    ULONG SizeOfRunArray,
    BOOLEAN LocateLongestRuns
    );
//
//  The following routine locates the longest contiguous region of
//  clear bits within the bitmap.  The returned starting index value
//  denotes the first contiguous region located satisfying our requirements
//  The return value is the length (in bits) of the longest region found.
//

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear (
    PRTL_BITMAP BitMapHeader,
    PULONG StartingIndex
    );

//
//  The following routine locates the first contiguous region of
//  clear bits within the bitmap.  The returned starting index value
//  denotes the first contiguous region located satisfying our requirements
//  The return value is the length (in bits) of the region found.
//

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear (
    PRTL_BITMAP BitMapHeader,
    PULONG StartingIndex
    );

//
//  The following macro returns the value of the bit stored within the
//  bitmap at the specified location.  If the bit is set a value of 1 is
//  returned otherwise a value of 0 is returned.
//
//      ULONG
//      RtlCheckBit (
//          PRTL_BITMAP BitMapHeader,
//          ULONG BitPosition
//          );
//
//
//  To implement CheckBit the macro retrieves the longword containing the
//  bit in question, shifts the longword to get the bit in question into the
//  low order bit position and masks out all other bits.
//

#define RtlCheckBit(BMH,BP) ((((BMH)->Buffer[(BP) / 32]) >> ((BP) % 32)) & 0x1)

//
//  The following two procedures return to the caller the total number of
//  clear or set bits within the specified bitmap.
//

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits (
    PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits (
    PRTL_BITMAP BitMapHeader
    );

//
//  The following two procedures return to the caller a boolean value
//  indicating if the specified range of bits are all clear or set.
//

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG Length
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG Length
    );

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

//
//  The following two procedures return to the caller a value indicating
//  the position within a ULONGLONG of the most or least significant non-zero
//  bit.  A value of zero results in a return value of -1.
//

NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit (
    IN ULONGLONG Set
    );

NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit (
    IN ULONGLONG Set
    );


//
// Random routines ...
//

NTSYSAPI
ULONG
NTAPI
RtlRandom(
    IN OUT PULONG  Seed
    ); 

//
// Time routines ...
//

NTSYSAPI
CCHAR
NTAPI
NtQuerySystemTime(
    OUT PLARGE_INTEGER  CurrentTime
    );


NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
    IN PLARGE_INTEGER  Time,
    OUT PULONG  ElapsedSeconds
    );


NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
    IN ULONG  ElapsedSeconds,
    OUT PLARGE_INTEGER  Time
    );

//
// Heap routines...
//

#define GetProcessHeap() (NtCurrentTeb()->Peb->ProcessHeap)

PVOID STDCALL
RtlAllocateHeap (
	HANDLE	Heap,
	ULONG	Flags,
	ULONG	Size
	);

BOOLEAN
STDCALL
RtlFreeHeap (
	HANDLE	Heap,
	ULONG	Flags,
	PVOID	Address
	);

/*
 *  Bitmap.c
 */

#define ext2_mark_block_bitmap ext2_mark_bitmap
#define ext2_mark_inode_bitmap ext2_mark_bitmap
#define ext2_unmark_block_bitmap ext2_unmark_bitmap
#define ext2_unmark_inode_bitmap ext2_unmark_bitmap

bool ext2_set_bit(int nr, void * addr);
bool ext2_clear_bit(int nr, void * addr);
bool ext2_test_bit(int nr, void * addr);

bool ext2_mark_bitmap(PEXT2_BITMAP bitmap, ULONG bitno);
bool ext2_unmark_bitmap(PEXT2_BITMAP bitmap, ULONG bitno);

bool ext2_test_block_bitmap(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG block);

bool ext2_test_block_bitmap_range(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG block, int num);

bool ext2_test_inode_bitmap(PEXT2_BLOCK_BITMAP bitmap,
                        ULONG inode);


bool ext2_allocate_block_bitmap(PEXT2_FILESYS pExt2Sys);
bool ext2_allocate_inode_bitmap(PEXT2_FILESYS pExt2Sys);
void ext2_free_inode_bitmap(PEXT2_FILESYS pExt2Sys);
void ext2_free_block_bitmap(PEXT2_FILESYS pExt2Sys);

bool ext2_write_block_bitmap (PEXT2_FILESYS fs);
bool ext2_write_inode_bitmap (PEXT2_FILESYS fs);

bool ext2_write_bitmaps(PEXT2_FILESYS fs);

//bool read_bitmaps(PEXT2_FILESYS fs, int do_inode, int do_block);
bool ext2_read_inode_bitmap (PEXT2_FILESYS fs);
bool ext2_read_block_bitmap(PEXT2_FILESYS fs);
bool ext2_read_bitmaps(PEXT2_FILESYS fs);


/*
 *  Disk.c
 */

NTSTATUS
Ext2OpenDevice( PEXT2_FILESYS    Ext2Sys,
                PUNICODE_STRING  DeviceName );

NTSTATUS
Ext2CloseDevice( PEXT2_FILESYS  Ext2Sys);

NTSTATUS 
Ext2ReadDisk( PEXT2_FILESYS  Ext2Sys,
              ULONGLONG      Offset,
              ULONG          Length,
              PVOID          Buffer     );

NTSTATUS
Ext2WriteDisk( PEXT2_FILESYS  Ext2Sys,
               ULONGLONG      Offset,
               ULONG          Length,
               PVOID          Buffer );

NTSTATUS
Ext2GetMediaInfo( PEXT2_FILESYS Ext2Sys );


NTSTATUS
Ext2LockVolume( PEXT2_FILESYS Ext2Sys );

NTSTATUS
Ext2UnLockVolume( PEXT2_FILESYS Ext2Sys );

NTSTATUS
Ext2DisMountVolume( PEXT2_FILESYS Ext2Sys );


/*
 *  Group.c
 */

bool ext2_allocate_group_desc(PEXT2_FILESYS pExt2Sys);
void ext2_free_group_desc(PEXT2_FILESYS pExt2Sys);
bool ext2_bg_has_super(PEXT2_SUPER_BLOCK pExt2Sb, int group_block);

/*
 *  Inode.c
 */

bool ext2_get_inode_lba(PEXT2_FILESYS pExt2Sys, ULONG no, LONGLONG *offset);
bool ext2_load_inode(PEXT2_FILESYS pExt2Sys, ULONG no, PEXT2_INODE pInode);
bool ext2_save_inode(PEXT2_FILESYS pExt2Sys, ULONG no, PEXT2_INODE pInode);
bool ext2_new_inode(PEXT2_FILESYS fs, ULONG dir, int mode,
                      PEXT2_INODE_BITMAP map, ULONG *ret);
bool ext2_expand_inode(PEXT2_FILESYS pExt2Sys, PEXT2_INODE, ULONG newBlk);

bool ext2_read_inode (PEXT2_FILESYS pExt2Sys,
            ULONG               ino,
            ULONG               offset,
            PVOID               Buffer,
            ULONG               size,
            PULONG              dwReturn );
bool ext2_write_inode (PEXT2_FILESYS pExt2Sys,
            ULONG               ino,
            ULONG               offset,
            PVOID               Buffer,
            ULONG               size,
            PULONG              dwReturn );

bool ext2_add_entry(PEXT2_FILESYS pExt2Sys, ULONG parent, ULONG inode, int filetype, char *name);
bool ext2_reserve_inodes(PEXT2_FILESYS fs);
/*
 *  Memory.c
 */

//
// Return the group # of an inode number
//
int ext2_group_of_ino(PEXT2_FILESYS fs, ULONG ino);

//
// Return the group # of a block
//
int ext2_group_of_blk(PEXT2_FILESYS fs, ULONG blk);

/*
 *  Badblock.c
 */


void ext2_inode_alloc_stats2(PEXT2_FILESYS fs, ULONG ino, int inuse, int isdir);
void ext2_inode_alloc_stats(PEXT2_FILESYS fs, ULONG ino, int inuse);
void ext2_block_alloc_stats(PEXT2_FILESYS fs, ULONG blk, int inuse);

bool ext2_allocate_tables(PEXT2_FILESYS pExt2Sys);
bool ext2_allocate_group_table(PEXT2_FILESYS fs, ULONG group,
                      PEXT2_BLOCK_BITMAP bmap);
bool ext2_get_free_blocks(PEXT2_FILESYS fs, ULONG start, ULONG finish,
                 int num, PEXT2_BLOCK_BITMAP map, ULONG *ret);
bool write_inode_tables(PEXT2_FILESYS fs);

bool ext2_new_block(PEXT2_FILESYS fs, ULONG goal,
               PEXT2_BLOCK_BITMAP map, ULONG *ret);
bool ext2_alloc_block(PEXT2_FILESYS fs, ULONG goal, ULONG *ret);
bool ext2_new_dir_block(PEXT2_FILESYS fs, ULONG dir_ino,
                   ULONG parent_ino, char **block);
bool ext2_write_block(PEXT2_FILESYS fs, ULONG block, void *inbuf);
bool ext2_read_block(PEXT2_FILESYS fs, ULONG block, void *inbuf);

/*
 *  Mke2fs.c
 */

bool parase_cmd(int argc, char *argv[], PEXT2_FILESYS pExt2Sys);

bool zero_blocks(PEXT2_FILESYS fs, ULONG blk, ULONG num,
                 ULONG *ret_blk, ULONG *ret_count);

ULONG
Ext2DataBlocks(PEXT2_FILESYS Ext2Sys, ULONG TotalBlocks);

ULONG
Ext2TotalBlocks(PEXT2_FILESYS Ext2Sys, ULONG DataBlocks);



/*
 *  Super.c
 */

void ext2_print_super(PEXT2_SUPER_BLOCK pExt2Sb);
bool ext2_initialize_sb(PEXT2_FILESYS pExt2Sys);


/*
 *  Super.c
 */

LONGLONG ext2_nt_time (ULONG i_time);
ULONG ext2_unix_time (LONGLONG n_time);

/*
 *  Uuid.c
 */

void uuid_generate(__u8 * uuid);

#endif //__MKE2FS__INCLUDE__
