/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             Ext2fs.h
 * PURPOSE:          Header file: ext2 structures
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY:
 */

#ifndef _EXT2_HEADER_
#define _EXT2_HEADER_

/* INCLUDES *************************************************************/
#include <linux/module.h>
#include <ntdddisk.h>
#ifdef __REACTOS__
#include <ndk/rtlfuncs.h>
#include <pseh/pseh2.h>
#endif
#include "stdio.h"
#include <string.h>
#include <linux/ext2_fs.h>
#include <linux/ext3_fs.h>
#include <linux/ext3_fs_i.h>
#include <linux/ext4.h>

/* DEBUG ****************************************************************/
#if DBG
# define EXT2_DEBUG   1
#else
# define EXT2_DEBUG   0
#endif

#define EXT_DEBUG_BREAKPOINT FALSE

#if EXT2_DEBUG && EXT_DEBUG_BREAKPOINT
//#if _X86_
//#define DbgBreak()      __asm int 3
//#else
#define DbgBreak()      KdBreakPoint()
//#endif
#else
#define DbgBreak()
#endif

/* STRUCTS & CONSTS******************************************************/

#define EXT2FSD_VERSION                 "0.62"


//
// Ext2Fsd build options
//

// To support driver dynamics unload

#define EXT2_UNLOAD                     FALSE

// To support inode size expansion (fallocate)

#define EXT2_PRE_ALLOCATION_SUPPORT     TRUE

//
// Constants
//

#define EXT2_MAX_NESTED_LINKS           (8)
#define EXT2_LINKLEN_IN_INODE           (60)
#define EXT2_BLOCK_TYPES                (0x04)

#define MAXIMUM_RECORD_LENGTH           (0x10000)

#define SECTOR_BITS                     (Vcb->SectorBits)
#define SECTOR_SIZE                     (Vcb->DiskGeometry.BytesPerSector)
#define DEFAULT_SECTOR_SIZE             (0x200)

#define SUPER_BLOCK_OFFSET              (0x400)
#define SUPER_BLOCK_SIZE                (0x400)

#define READ_AHEAD_GRANULARITY          (0x10000)

#define SUPER_BLOCK                     (Vcb->SuperBlock)

#define INODE_SIZE                      (Vcb->InodeSize)
#define BLOCK_SIZE                      (Vcb->BlockSize)
#define BLOCK_BITS                      (SUPER_BLOCK->s_log_block_size + 10)
#define GROUP_DESC_SIZE                 (Vcb->sbi.s_desc_size)

#define INODES_COUNT                    (Vcb->SuperBlock->s_inodes_count)

#define INODES_PER_GROUP                (SUPER_BLOCK->s_inodes_per_group)
#define BLOCKS_PER_GROUP                (SUPER_BLOCK->s_blocks_per_group)
#define TOTAL_BLOCKS                    (ext3_blocks_count(SUPER_BLOCK))

#define EXT2_FIRST_DATA_BLOCK           (SUPER_BLOCK->s_first_data_block)

typedef struct ext3_super_block EXT2_SUPER_BLOCK, *PEXT2_SUPER_BLOCK;
typedef struct ext3_inode EXT2_INODE, *PEXT2_INODE;
typedef struct ext4_group_desc EXT2_GROUP_DESC, *PEXT2_GROUP_DESC;
typedef struct ext3_dir_entry EXT2_DIR_ENTRY, *PEXT2_DIR_ENTRY;
typedef struct ext3_dir_entry_2 EXT2_DIR_ENTRY2, *PEXT2_DIR_ENTRY2;

#define CEILING_ALIGNED(T, A, B) (((A) + (B) - 1) & (~((T)(B) - 1)))
#define COCKLOFT_ALIGNED(T, A, B) (((A) + (B)) & (~((T)(B) - 1)))

/* File System Releated *************************************************/

#define DRIVER_NAME      "Ext2Fsd"
#define DEVICE_NAME     L"\\Ext2Fsd"
#define CDROM_NAME      L"\\Ext2CdFsd"

// Registry

#define PARAMETERS_KEY      L"\\Parameters"
#define VOLUMES_KEY         L"\\Volumes"

#define READING_ONLY        L"Readonly"
#define WRITING_SUPPORT     L"WritingSupport"
#define CHECKING_BITMAP     L"CheckingBitmap"
#define EXT3_FORCEWRITING   L"Ext3ForceWriting"
#define CODEPAGE_NAME       L"CodePage"
#define HIDING_PREFIX       L"HidingPrefix"
#define HIDING_SUFFIX       L"HidingSuffix"
#define AUTO_MOUNT          L"AutoMount"
#define MOUNT_POINT         L"MountPoint"

#define DOS_DEVICE_NAME L"\\DosDevices\\Ext2Fsd"

// To support ext2fsd unload routine
#if EXT2_UNLOAD
//
// Private IOCTL to make the driver ready to unload
//
#define IOCTL_PREPARE_TO_UNLOAD \
CTL_CODE(FILE_DEVICE_UNKNOWN, 2048, METHOD_NEITHER, FILE_WRITE_ACCESS)

#endif // EXT2_UNLOAD

#include "common.h"

#ifndef _GNU_NTIFS_
typedef IO_STACK_LOCATION EXTENDED_IO_STACK_LOCATION, *PEXTENDED_IO_STACK_LOCATION;
#endif

#define IsFlagOn(a,b) ((BOOLEAN)(FlagOn(a,b) == b))
#ifndef FlagOn
#define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF)   ((BOOLEAN)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifdef _WIN2K_TARGET_
#define InterlockedOr _InterlockedOr
LONG
_InterlockedAnd (
    IN OUT LONG volatile *Target,
    IN LONG Set
);

#pragma intrinsic (_InterlockedAnd)
#define InterlockedAnd _InterlockedAnd

LONG
_InterlockedXor (
    IN OUT LONG volatile *Target,
    IN LONG Set
);

#pragma intrinsic (_InterlockedXor)
#define InterlockedXor _InterlockedXor

#endif  /* only for win2k */

#if EXT2_DEBUG

#define SetLongFlag(_F,_SF)   Ext2SetFlag((PULONG)&(_F), (ULONG)(_SF))
#define ClearLongFlag(_F,_SF) Ext2ClearFlag((PULONG)&(_F), (ULONG)(_SF))

#ifdef __REACTOS__
static
#endif
__inline
VOID
Ext2SetFlag(PULONG Flags, ULONG FlagBit)
{
    ULONG _ret = InterlockedOr(Flags, FlagBit);
    ASSERT(*Flags == (_ret | FlagBit));
}

#ifdef __REACTOS__
static
#endif
__inline
VOID
Ext2ClearFlag(PULONG Flags, ULONG FlagBit)
{
    ULONG _ret = InterlockedAnd(Flags, ~FlagBit);
    ASSERT(*Flags == (_ret & (~FlagBit)));
}

#else

#define SetLongFlag(_F,_SF)       InterlockedOr(&(_F), (ULONG)(_SF))
#define ClearLongFlag(_F,_SF)     InterlockedAnd(&(_F), ~((ULONG)(_SF)))

#endif  /* release */

#define Ext2RaiseStatus(IRPCONTEXT,STATUS) {  \
    (IRPCONTEXT)->ExceptionCode = (STATUS); \
    ExRaiseStatus((STATUS));                \
}

#define Ext2NormalizeAndRaiseStatus(IRPCONTEXT,STATUS) {                        \
    (IRPCONTEXT)->ExceptionCode = STATUS;                                       \
    if ((STATUS) == STATUS_VERIFY_REQUIRED) { ExRaiseStatus((STATUS)); }        \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STATUS),STATUS_UNEXPECTED_IO_ERROR)); \
}

//
// Define IsWritingToEof for write (append) operations
//

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff

#define IsWritingToEof(Pos) (((Pos).LowPart == FILE_WRITE_TO_END_OF_FILE) && \
                             ((Pos).HighPart == -1 ))

#define IsDirectory(Fcb)    IsMcbDirectory((Fcb)->Mcb)
#define IsSpecialFile(Fcb)  IsMcbSpecialFile((Fcb)->Mcb)
#define IsSymLink(Fcb)      IsMcbSymLink((Fcb)->Mcb)
#define IsInodeSymLink(I)   S_ISLNK((I)->i_mode)
#define IsRoot(Fcb)         IsMcbRoot((Fcb)->Mcb)

//
// Pool Tags
//

#define TAG_VPB  ' bpV'
#define VPB_SIZE sizeof(VPB)

#define EXT2_DATA_MAGIC         'BD2E'
#define EXT2_INAME_MAGIC        'NI2E'
#define EXT2_FNAME_MAGIC        'NF2E'
#define EXT2_VNAME_MAGIC        'NV2E'
#define EXT2_DENTRY_MAGIC       'ED2E'
#define EXT2_DIRSP_MAGIC        'SD2E'
#define EXT2_SB_MAGIC           'BS2E'
#define EXT2_GD_MAGIC           'DG2E'
#define EXT2_FLIST_MAGIC        'LF2E'
#define EXT2_PARAM_MAGIC        'PP2E'
#define EXT2_RWC_MAGIC          'WR2E'

//
// Bug Check Codes Definitions
//

#define EXT2_FILE_SYSTEM   (FILE_SYSTEM)

#define EXT2_BUGCHK_BLOCK               (0x00010000)
#define EXT2_BUGCHK_CLEANUP             (0x00020000)
#define EXT2_BUGCHK_CLOSE               (0x00030000)
#define EXT2_BUGCHK_CMCB                (0x00040000)
#define EXT2_BUGCHK_CREATE              (0x00050000)
#define EXT2_BUGCHK_DEBUG               (0x00060000)
#define EXT2_BUGCHK_DEVCTL              (0x00070000)
#define EXT2_BUGCHK_DIRCTL              (0x00080000)
#define EXT2_BUGCHK_DISPATCH            (0x00090000)
#define EXT2_BUGCHK_EXCEPT              (0x000A0000)
#define EXT2_BUGCHK_EXT2                (0x000B0000)
#define EXT2_BUGCHK_FASTIO              (0x000C0000)
#define EXT2_BUGCHK_FILEINFO            (0x000D0000)
#define EXT2_BUGCHK_FLUSH               (0x000E0000)
#define EXT2_BUGCHK_FSCTL               (0x000F0000)
#define EXT2_BUGCHK_INIT                (0x00100000)
#define EXT2_BUGCHK_LOCK                (0x0011000)
#define EXT2_BUGCHK_MEMORY              (0x0012000)
#define EXT2_BUGCHK_MISC                (0x0013000)
#define EXT2_BUGCHK_READ                (0x00140000)
#define EXT2_BUGCHK_SHUTDOWN            (0x00150000)
#define EXT2_BUGCHK_VOLINFO             (0x00160000)
#define EXT2_BUGCHK_WRITE               (0x00170000)

#define EXT2_BUGCHK_LAST                (0x00170000)

#define Ext2BugCheck(A,B,C,D) { KeBugCheckEx(EXT2_FILE_SYSTEM, A | __LINE__, B, C, D ); }


/* Ext2 file system definions *******************************************/

//
// The second extended file system magic number
//

#define EXT2_SUPER_MAGIC        0xEF53

#define EXT2_MIN_BLOCK          1024
#define EXT2_MIN_FRAG           1024
#define EXT2_MAX_USER_BLKSIZE   65536
//
// Inode flags (Linux uses octad number, but why ? strange!!!)
//

#define S_IFMT   0x0F000            /* 017 0000 */
#define S_IFSOCK 0x0C000            /* 014 0000 */
#define S_IFLNK  0x0A000            /* 012 0000 */
#define S_IFREG  0x08000            /* 010 0000 */
#define S_IFBLK  0x06000            /* 006 0000 */
#define S_IFDIR  0x04000            /* 004 0000 */
#define S_IFCHR  0x02000            /* 002 0000 */
#define S_IFIFO  0x01000            /* 001 0000 */

#define S_ISUID  0x00800            /* 000 4000 */
#define S_ISGID  0x00400            /* 000 2000 */
#define S_ISVTX  0x00200            /* 000 1000 */

#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#define S_ISFIL(m)      (((m) & S_IFMT) == S_IFFIL)
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)

#define S_IPERMISSION_MASK 0x1FF /*  */

#define S_IRWXU  0x1C0              /* 0 0700 */
#define S_IRWNU  0x180              /* 0 0600 */
#define S_IRUSR  0x100              /* 0 0400 */
#define S_IWUSR  0x080              /* 0 0200 */
#define S_IXUSR  0x040              /* 0 0100 */

#define S_IRWXG  0x038              /* 0 0070 */
#define S_IRWNG  0x030              /* 0 0060 */
#define S_IRGRP  0x020              /* 0 0040 */
#define S_IWGRP  0x010              /* 0 0020 */
#define S_IXGRP  0x008              /* 0 0010 */

#define S_IRWXO  0x007              /* 0 0007 */
#define S_IRWNO  0x006              /* 0 0006 */
#define S_IROTH  0x004              /* 0 0004 */
#define S_IWOTH  0x002              /* 0 0002 */
#define S_IXOTH  0x001              /* 0 0001 */

#define S_IRWXUGO   (S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO   (S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO     (S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO     (S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO     (S_IXUSR|S_IXGRP|S_IXOTH)
#define S_IFATTR    (S_IRWNU|S_IRWNG|S_IRWNO)

#define S_ISREADABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IRUSR | S_IRGRP | S_IROTH))
#define S_ISWRITABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IWUSR | S_IWGRP | S_IWOTH))

#define Ext2SetReadable(m) do {(m) = (m) | (S_IRUSR | S_IRGRP | S_IROTH);} while(0)
#define Ext2SetWritable(m) do {(m) = (m) | (S_IWUSR | S_IWGRP | S_IWOTH);} while(0)

#define Ext2SetOwnerWritable(m) do {(m) |= S_IWUSR;} while(0)
#define Ext2SetOwnerReadOnly(m) do {(m) &= ~S_IWUSR;} while(0)

#define Ext2IsOwnerWritable(m)  (((m) & S_IWUSR) == S_IWUSR)
#define Ext2IsOwnerReadOnly(m)  (!(Ext2IsOwnerWritable(m)))

#define Ext2SetReadOnly(m) do {(m) &= ~(S_IWUSR | S_IWGRP | S_IWOTH);} while(0)

/*
 * We need 8-bytes aligned for all the sturctures
 * It's a must for all ERESOURCE allocations
 */

//
// Ext2Fsd Driver Definitions
//

//
// EXT2_IDENTIFIER_TYPE
//
// Identifiers used to mark the structures
//

typedef enum _EXT2_IDENTIFIER_TYPE {
#ifdef _MSC_VER
    EXT2FGD  = ':DGF',
    EXT2VCB  = ':BCV',
    EXT2FCB  = ':BCF',
    EXT2CCB  = ':BCC',
    EXT2ICX  = ':XCI',
    EXT2FSD  = ':DSF',
    EXT2MCB  = ':BCM'
#else
    EXT2FGD  = 0xE2FD0001,
    EXT2VCB  = 0xE2FD0002,
    EXT2FCB  = 0xE2FD0003,
    EXT2CCB  = 0xE2FD0004,
    EXT2ICX  = 0xE2FD0005,
    EXT2FSD  = 0xE2FD0006,
    EXT2MCB  = 0xE2FD0007
#endif
} EXT2_IDENTIFIER_TYPE;

//
// EXT2_IDENTIFIER
//
// Header used to mark the structures
//
typedef struct _EXT2_IDENTIFIER {
    EXT2_IDENTIFIER_TYPE     Type;
    ULONG                    Size;
} EXT2_IDENTIFIER, *PEXT2_IDENTIFIER;


#define NodeType(Ptr) (*((EXT2_IDENTIFIER_TYPE *)(Ptr)))

typedef struct _EXT2_MCB  EXT2_MCB, *PEXT2_MCB;


typedef PVOID   PBCB;

//

//
// EXT2_GLOBAL_DATA
//
// Data that is not specific to a mounted volume
//

typedef struct _EXT2_GLOBAL {

    /* Identifier for this structure */
    EXT2_IDENTIFIER             Identifier;

    /* Syncronization primitive for this structure */
    ERESOURCE                   Resource;

    /* Global flags for the driver: I put it since
       FastIoDispatch isn't 8bytes aligned.  */
    ULONG                       Flags;

    /* Table of pointers to the fast I/O entry points */
    FAST_IO_DISPATCH            FastIoDispatch;

    /* Table of pointers to the Cache Manager callbacks */
    CACHE_MANAGER_CALLBACKS     CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS     CacheManagerNoOpCallbacks;

    /* Pointer to the driver object */
    PDRIVER_OBJECT              DriverObject;

    /* Pointer to the disk device object */
    PDEVICE_OBJECT              DiskdevObject;

    /* Pointer to the cdrom device object */
    PDEVICE_OBJECT              CdromdevObject;

    /* List of mounted volumes */
    LIST_ENTRY                  VcbList;

    /* Cleaning thread related: resource cleaner */
    struct {
        KEVENT                  Engine;
        KEVENT                  Wait;
    } Reaper;

    /* Look Aside table of IRP_CONTEXT, FCB, MCB, CCB */
    NPAGED_LOOKASIDE_LIST       Ext2IrpContextLookasideList;
    NPAGED_LOOKASIDE_LIST       Ext2FcbLookasideList;
    NPAGED_LOOKASIDE_LIST       Ext2CcbLookasideList;
    NPAGED_LOOKASIDE_LIST       Ext2McbLookasideList;
    NPAGED_LOOKASIDE_LIST       Ext2ExtLookasideList;
    NPAGED_LOOKASIDE_LIST       Ext2DentryLookasideList;
    USHORT                      MaxDepth;

    /* User specified global codepage name */
    struct {
        UCHAR                   AnsiName[CODEPAGE_MAXLEN];
        struct nls_table *      PageTable;
    } Codepage;

    /* global hiding patterns */
    BOOLEAN                     bHidingPrefix;
    CHAR                        sHidingPrefix[HIDINGPAT_LEN];
    BOOLEAN                     bHidingSuffix;
    CHAR                        sHidingSuffix[HIDINGPAT_LEN];

    /* Registery path */
    UNICODE_STRING              RegistryPath;

    /* global memory and i/o statistics and memory allocations
       of various sturctures */

    EXT2_PERF_STATISTICS_V2     PerfStat;

} EXT2_GLOBAL, *PEXT2_GLOBAL;

//
// Flags for EXT2_GLOBAL_DATA
//

#define EXT2_UNLOAD_PENDING     0x00000001
#define EXT2_SUPPORT_WRITING    0x00000002
#define EXT3_FORCE_WRITING      0x00000004
#define EXT2_CHECKING_BITMAP    0x00000008
#define EXT2_AUTO_MOUNT         0x00000010

//
// Glboal Ext2Fsd Memory Block
//

extern PEXT2_GLOBAL Ext2Global;

//
// memory allocation statistics
//


#define INC_MEM_COUNT(_i, _p, _s) do { ASSERT(_p); Ext2TraceMemory(TRUE, (int) (_i), (PVOID)(_p), (LONG)(_s)); } while(0)
#define DEC_MEM_COUNT(_i, _p, _s) do { ASSERT(_p); Ext2TraceMemory(FALSE, (int) (_i), (PVOID)(_p), (LONG)(_s)); } while(0)
#define INC_IRP_COUNT(IrpContext) Ext2TraceIrpContext(TRUE, (IrpContext))
#define DEC_IRP_COUNT(IrpContext) Ext2TraceIrpContext(FALSE, (IrpContext))

//
// Driver Extension define
//

#define IsExt2FsDevice(DO) ((DO == Ext2Global->DiskdevObject) || \
                            (DO == Ext2Global->CdromdevObject) )

#ifdef _WIN2K_TARGET_
#define  FSRTL_ADVANCED_FCB_HEADER FSRTL_COMMON_FCB_HEADER
#endif

typedef struct _EXT2_FCBVCB {

    // Command header for Vcb and Fcb
    FSRTL_ADVANCED_FCB_HEADER   Header;

#ifndef _WIN2K_TARGET_
    FAST_MUTEX                  Mutex;
#endif

    // Ext2Fsd identifier
    EXT2_IDENTIFIER             Identifier;


    // Locking resources
    ERESOURCE                   MainResource;
    ERESOURCE                   PagingIoResource;

} EXT2_FCBVCB, *PEXT2_FCBVCB;

//
// EXT2_VCB Volume Control Block
//
// Data that represents a mounted logical volume
// It is allocated as the device extension of the volume device object
//
typedef struct _EXT2_VCB {

    /* Common header */
    EXT2_FCBVCB;

    // Resource for metadata (super block, tables)
    ERESOURCE                   MetaLock;

    // Resource for Mcb (Meta data control block)
    ERESOURCE                   McbLock;

    // Entry of Mcb Tree (Root Node)
    PEXT2_MCB                   McbTree;

    // Mcb list
    LIST_ENTRY                  McbList;
    ULONG                       NumOfMcb;

    // Link list to Global
    LIST_ENTRY                  Next;

    // Section objects
    SECTION_OBJECT_POINTERS     SectionObject;

    // Dirty Mcbs of modifications for volume stream
    LARGE_MCB                   Extents;

    // List of FCBs for open files on this volume
    ULONG                       FcbCount;
    LIST_ENTRY                  FcbList;
    KSPIN_LOCK                  FcbLock;

    // Share Access for the file object
    SHARE_ACCESS                ShareAccess;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    // for both files on this volume and open instances of the
    // volume itself.
    ULONG                       ReferenceCount;     /* total ref count */
    ULONG                       OpenHandleCount;    /* all handles */

    ULONG                       OpenVolumeCount;    /* volume handle */

    // Disk change count
    ULONG                       ChangeCount;

    // Pointer to the VPB in the target device object
    PVPB                        Vpb;
    PVPB                        Vpb2;

    // The FileObject of Volume used to lock the volume
    PFILE_OBJECT                LockFile;

    // List of IRPs pending on directory change notify requests
    LIST_ENTRY                  NotifyList;

    // Pointer to syncronization primitive for this list
    PNOTIFY_SYNC                NotifySync;

    // This volumes device object
    PDEVICE_OBJECT              DeviceObject;

    // The physical device object (the disk)
    PDEVICE_OBJECT              TargetDeviceObject;

    // The physical device object (the disk)
    PDEVICE_OBJECT              RealDevice;

    // Information about the physical device object
    DISK_GEOMETRY               DiskGeometry;
    PARTITION_INFORMATION       PartitionInformation;

    BOOLEAN                     IsExt3fs;
    PEXT2_SUPER_BLOCK           SuperBlock;

    /*
        // Bitmap Block per group
        PRTL_BITMAP                 BlockBitMaps;
        PRTL_BITMAP                 InodeBitMaps;
    */

    // Block / Cluster size
    ULONG                       BlockSize;

    // Sector size in bits
    ULONG                       SectorBits;

    // Aligned size (Page or Block)
    ULONGLONG                   IoUnitSize;

    // Bits of aligned size
    ULONG                       IoUnitBits;

    // Inode size
    ULONG                       InodeSize;

    // Inode lookaside list
    NPAGED_LOOKASIDE_LIST       InodeLookasideList;

    // Flags for the volume
    ULONG                       Flags;

    // Streaming File Object
    PFILE_OBJECT                Volume;

    // User specified codepage name per volume
    struct {
        UCHAR                   AnsiName[CODEPAGE_MAXLEN];
        struct nls_table *      PageTable;
    } Codepage;

    /* patterns to hiding files */
    BOOLEAN                     bHidingPrefix;
    CHAR                        sHidingPrefix[HIDINGPAT_LEN];
    BOOLEAN                     bHidingSuffix;
    CHAR                        sHidingSuffix[HIDINGPAT_LEN];

    /* mountpoint: symlink to DesDevices */
    UCHAR                       DrvLetter;

    struct block_device         bd;
    struct super_block          sb;
    struct ext3_sb_info         sbi;

    /* Maximum file size in blocks ... */
    ULONG                       max_blocks_per_layer[EXT2_BLOCK_TYPES];
    ULONG                       max_data_blocks;
    loff_t                      max_bitmap_bytes;
    loff_t                      max_bytes;
} EXT2_VCB, *PEXT2_VCB;

//
// Flags for EXT2_VCB
//
#define VCB_INITIALIZED         0x00000001
#define VCB_VOLUME_LOCKED       0x00000002
#define VCB_MOUNTED             0x00000004
#define VCB_DISMOUNT_PENDING    0x00000008
#define VCB_NEW_VPB             0x00000010
#define VCB_BEING_CLOSED        0x00000020

#define VCB_FORCE_WRITING       0x00004000
#define VCB_DEVICE_REMOVED      0x00008000
#define VCB_JOURNAL_RECOVER     0x00080000
#define VCB_ARRIVAL_NOTIFIED    0x00800000
#define VCB_READ_ONLY           0x08000000
#define VCB_WRITE_PROTECTED     0x10000000
#define VCB_FLOPPY_DISK         0x20000000
#define VCB_REMOVAL_PREVENTED   0x40000000
#define VCB_REMOVABLE_MEDIA     0x80000000


#define IsVcbInited(Vcb)   (IsFlagOn((Vcb)->Flags, VCB_INITIALIZED))
#define IsMounted(Vcb)     (IsFlagOn((Vcb)->Flags, VCB_MOUNTED))
#define IsDispending(Vcb)  (IsFlagOn((Vcb)->Flags, VCB_DISMOUNT_PENDING))
#define IsVcbReadOnly(Vcb) (IsFlagOn((Vcb)->Flags, VCB_READ_ONLY) ||    \
                            IsFlagOn((Vcb)->Flags, VCB_WRITE_PROTECTED))


#define IsExt3ForceWrite()   (IsFlagOn(Ext2Global->Flags, EXT3_FORCE_WRITING))
#define IsVcbForceWrite(Vcb) (IsFlagOn((Vcb)->Flags, VCB_FORCE_WRITING))
#define CanIWrite(Vcb)       (IsExt3ForceWrite() || (!IsVcbReadOnly(Vcb) && IsVcbForceWrite(Vcb)))
#define IsLazyWriter(Fcb)    ((Fcb)->LazyWriterThread == PsGetCurrentThread())
//
// EXT2_FCB File Control Block
//
// Data that represents an open file
// There is a single instance of the FCB for every open file
//
typedef struct _EXT2_FCB {

    /* Common header */
    EXT2_FCBVCB;

    // List of FCBs for this volume
    LIST_ENTRY                      Next;

    SECTION_OBJECT_POINTERS         SectionObject;

    // Share Access for the file object
    SHARE_ACCESS                    ShareAccess;

    // List of byte-range locks for this file
    FILE_LOCK                       FileLockAnchor;

    // oplock information management structure
    OPLOCK                          Oplock;

    // Lazy writer thread context
    PETHREAD                        LazyWriterThread;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    ULONG                           OpenHandleCount;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    ULONG                           ReferenceCount;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    // But only for Files with FO_NO_INTERMEDIATE_BUFFERING flag
    ULONG                           NonCachedOpenCount;

    // Flags for the FCB
    ULONG                           Flags;

    // Pointer to the inode
    struct inode                   *Inode;

    // Vcb
    PEXT2_VCB                       Vcb;

    // Mcb Node ...
    PEXT2_MCB                       Mcb;

} EXT2_FCB, *PEXT2_FCB;

//
// Flags for EXT2_FCB
//

#define FCB_FROM_POOL               0x00000001
#define FCB_PAGE_FILE               0x00000002
#define FCB_FILE_MODIFIED           0x00000020
#define FCB_STATE_BUSY              0x00000040
#define FCB_ALLOC_IN_CREATE         0x00000080
#define FCB_ALLOC_IN_WRITE          0x00000100
#define FCB_ALLOC_IN_SETINFO        0x00000200

#define FCB_DELETE_PENDING          0x80000000

//
// Mcb Node
//

struct _EXT2_MCB {

    // Identifier for this structure
    EXT2_IDENTIFIER                 Identifier;

    // Flags
    ULONG                           Flags;

    // Link List Info
    PEXT2_MCB                       Parent; // Parent
    PEXT2_MCB                       Next;   // Brothers

    union {
        PEXT2_MCB                   Child;  // Children Mcb nodes
        PEXT2_MCB                   Target; // Target Mcb of symlink
    };

    // Mcb Node Info

    // -> Fcb
    PEXT2_FCB                       Fcb;

    // Short name
    UNICODE_STRING                  ShortName;

    // Full name with path
    UNICODE_STRING                  FullName;

    // File attribute
    ULONG                           FileAttr;

    // reference count
    ULONG                           Refercount;

    // Extents zone
    LARGE_MCB                       Extents;

	// Metablocks
    LARGE_MCB                       MetaExts;

    // Time stamps
    LARGE_INTEGER                   CreationTime;
    LARGE_INTEGER                   LastWriteTime;
    LARGE_INTEGER                   ChangeTime;
    LARGE_INTEGER                   LastAccessTime;

    // List Link to Vcb->McbList
    LIST_ENTRY                      Link;

    struct inode                    Inode;
    struct dentry                  *de;
};

//
// Flags for MCB
//
#define MCB_FROM_POOL               0x00000001
#define MCB_VCB_LINK                0x00000002
#define MCB_ENTRY_TREE              0x00000004
#define MCB_FILE_DELETED            0x00000008

#define MCB_ZONE_INITED             0x20000000
#define MCB_TYPE_SPECIAL            0x40000000  /* unresolved symlink + device node */
#define MCB_TYPE_SYMLINK            0x80000000

#define IsMcbUsed(Mcb)          ((Mcb)->Refercount > 0)
#define IsMcbSymLink(Mcb)       IsFlagOn((Mcb)->Flags, MCB_TYPE_SYMLINK)
#define IsZoneInited(Mcb)       IsFlagOn((Mcb)->Flags, MCB_ZONE_INITED)
#define IsMcbSpecialFile(Mcb)   IsFlagOn((Mcb)->Flags, MCB_TYPE_SPECIAL)
#define IsMcbRoot(Mcb)          ((Mcb)->Inode.i_ino == EXT2_ROOT_INO)
#define IsMcbReadonly(Mcb)      IsFlagOn((Mcb)->FileAttr, FILE_ATTRIBUTE_READONLY)
#define IsMcbDirectory(Mcb)     IsFlagOn((Mcb)->FileAttr, FILE_ATTRIBUTE_DIRECTORY)
#define IsFileDeleted(Mcb)      IsFlagOn((Mcb)->Flags, MCB_FILE_DELETED)

#define IsLinkInvalid(Mcb)      (IsMcbSymLink(Mcb) && IsFileDeleted(Mcb->Target))

/*
 * routines for reference count management
 */

#define Ext2ReferXcb(_C)  InterlockedIncrement(_C)
#define Ext2DerefXcb(_C)  DEC_OBJ_CNT(_C)

#ifdef __REACTOS__
static
#endif
__inline ULONG DEC_OBJ_CNT(PULONG _C) {
    if (*_C > 0) {
        return InterlockedDecrement(_C);
    } else {
        DbgBreak();
    }
    return 0;
}

#if EXT2_DEBUG
VOID
Ext2TraceMcb(PCHAR fn, USHORT lc, USHORT add, PEXT2_MCB Mcb);
#define Ext2ReferMcb(Mcb) Ext2TraceMcb(__FUNCTION__, __LINE__, TRUE, Mcb)
#define Ext2DerefMcb(Mcb) Ext2TraceMcb(__FUNCTION__, __LINE__, FALSE, Mcb)
#else
#define Ext2ReferMcb(Mcb) Ext2ReferXcb(&Mcb->Refercount)
#define Ext2DerefMcb(Mcb) Ext2DerefXcb(&Mcb->Refercount)
#endif

//
// EXT2_CCB Context Control Block
//
// Data that represents one instance of an open file
// There is one instance of the CCB for every instance of an open file
//
typedef struct _EXT2_CCB {

    // Identifier for this structure
    EXT2_IDENTIFIER     Identifier;

    // Flags
    ULONG               Flags;

    // Mcb of it's symbol link
    PEXT2_MCB           SymLink;

    // State that may need to be maintained
    UNICODE_STRING      DirectorySearchPattern;

    /* Open handle control block */
    struct file         filp;

} EXT2_CCB, *PEXT2_CCB;

//
// Flags for CCB
//

#define CCB_FROM_POOL               0x00000001
#define CCB_VOLUME_DASD_PURGE       0x00000002
#define CCB_LAST_WRITE_UPDATED      0x00000004

#define CCB_DELETE_ON_CLOSE         0x00000010

#define CCB_ALLOW_EXTENDED_DASD_IO  0x80000000

//
// EXT2_IRP_CONTEXT
//
// Used to pass information about a request between the drivers functions
//
typedef struct ext2_icb {

    // Identifier for this structure
    EXT2_IDENTIFIER     Identifier;

    // Pointer to the IRP this request describes
    PIRP                Irp;

    // Flags
    ULONG               Flags;

    // The major and minor function code for the request
    UCHAR               MajorFunction;
    UCHAR               MinorFunction;

    // The device object
    PDEVICE_OBJECT      DeviceObject;

    // The real device object
    PDEVICE_OBJECT      RealDevice;

    // The file object
    PFILE_OBJECT        FileObject;

    PEXT2_FCB           Fcb;
    PEXT2_CCB           Ccb;

    // If the request is top level
    BOOLEAN             IsTopLevel;

    // Used if the request needs to be queued for later processing
    WORK_QUEUE_ITEM     WorkQueueItem;

    // If an exception is currently in progress
    BOOLEAN             ExceptionInProgress;

    // The exception code when an exception is in progress
    NTSTATUS            ExceptionCode;

} EXT2_IRP_CONTEXT, *PEXT2_IRP_CONTEXT;


#define IRP_CONTEXT_FLAG_FROM_POOL       (0x00000001)
#define IRP_CONTEXT_FLAG_WAIT            (0x00000002)
#define IRP_CONTEXT_FLAG_WRITE_THROUGH   (0x00000004)
#define IRP_CONTEXT_FLAG_FLOPPY          (0x00000008)
#define IRP_CONTEXT_FLAG_DISABLE_POPUPS  (0x00000020)
#define IRP_CONTEXT_FLAG_DEFERRED        (0x00000040)
#define IRP_CONTEXT_FLAG_VERIFY_READ     (0x00000080)
#define IRP_CONTEXT_STACK_IO_CONTEXT     (0x00000100)
#define IRP_CONTEXT_FLAG_REQUEUED        (0x00000200)
#define IRP_CONTEXT_FLAG_USER_IO         (0x00000400)
#define IRP_CONTEXT_FLAG_DELAY_CLOSE     (0x00000800)
#define IRP_CONTEXT_FLAG_FILE_BUSY       (0x00001000)


#define Ext2CanIWait() (!IrpContext || IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))

//
// EXT2_ALLOC_HEADER
//
// In the checked version of the driver this header is put in the beginning of
// every memory allocation
//
typedef struct _EXT2_ALLOC_HEADER {
    EXT2_IDENTIFIER Identifier;
} EXT2_ALLOC_HEADER, *PEXT2_ALLOC_HEADER;

typedef struct _FCB_LIST_ENTRY {
    PEXT2_FCB    Fcb;
    LIST_ENTRY   Next;
} FCB_LIST_ENTRY, *PFCB_LIST_ENTRY;


// Block Description List
typedef struct _EXT2_EXTENT {
    LONGLONG    Lba;
    ULONG       Offset;
    ULONG       Length;
    PIRP        Irp;
    struct _EXT2_EXTENT * Next;
} EXT2_EXTENT, *PEXT2_EXTENT;


/* FUNCTIONS DECLARATION *****************************************************/

// Include this so we don't need the latest WDK to build the driver.
#ifndef FSCTL_GET_RETRIEVAL_POINTER_BASE
#define FSCTL_GET_RETRIEVAL_POINTER_BASE    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 141, METHOD_BUFFERED, FILE_ANY_ACCESS) // RETRIEVAL_POINTER_BASE
#endif

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanExt2Wait(IRP) IoIsOperationSynchronous(Irp)

//
// memory allocation statistics
//

#ifdef __REACTOS__
static
#endif
__inline
VOID
Ext2TraceMemory(BOOLEAN _n, int _i, PVOID _p, LONG _s)
{
    if (_n) {
        InterlockedIncrement(&Ext2Global->PerfStat.Current.Slot[_i]);
        InterlockedIncrement(&Ext2Global->PerfStat.Total.Slot[_i]);
        InterlockedExchangeAdd(&Ext2Global->PerfStat.Size.Slot[_i], _s);
    } else {
        InterlockedDecrement(&Ext2Global->PerfStat.Current.Slot[_i]);
        InterlockedExchangeAdd(&Ext2Global->PerfStat.Size.Slot[_i], -1 * _s);
    }
}

#ifdef __REACTOS__
static
#endif
__inline
VOID
Ext2TraceIrpContext(BOOLEAN _n, PEXT2_IRP_CONTEXT IrpContext)
{
    if (_n) {
        INC_MEM_COUNT(PS_IRP_CONTEXT, IrpContext, sizeof(EXT2_IRP_CONTEXT));
        InterlockedIncrement(&(Ext2Global->PerfStat.Irps[IrpContext->MajorFunction].Current));
    } else {
        DEC_MEM_COUNT(PS_IRP_CONTEXT, IrpContext, sizeof(EXT2_IRP_CONTEXT));
        InterlockedIncrement(&Ext2Global->PerfStat.Irps[IrpContext->MajorFunction].Processed);
        InterlockedDecrement(&Ext2Global->PerfStat.Irps[IrpContext->MajorFunction].Current);
    }
}

typedef struct _EXT2_FILLDIR_CONTEXT {
    PEXT2_IRP_CONTEXT       efc_irp;
    PUCHAR                  efc_buf;
    ULONG                   efc_size;
    ULONG                   efc_start;
    ULONG                   efc_prev;
    NTSTATUS                efc_status;
    FILE_INFORMATION_CLASS  efc_fi;
    BOOLEAN                 efc_single;
} EXT2_FILLDIR_CONTEXT, *PEXT2_FILLDIR_CONTEXT;

//
// Block.c
//

PMDL
Ext2CreateMdl (
    IN PVOID Buffer,
    IN BOOLEAN bPaged,
    IN ULONG Length,
    IN LOCK_OPERATION Operation
);

VOID
Ext2DestroyMdl (IN PMDL Mdl);

NTSTATUS
Ext2LockUserBuffer (
    IN PIRP             Irp,
    IN ULONG            Length,
    IN LOCK_OPERATION   Operation);
PVOID
Ext2GetUserBuffer (IN PIRP Irp);


NTSTATUS
Ext2ReadWriteBlocks(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB        Vcb,
    IN PEXT2_EXTENT     Extent,
    IN ULONG            Length
    );

NTSTATUS
Ext2ReadSync(
    IN PEXT2_VCB        Vcb,
    IN ULONGLONG        Offset,
    IN ULONG            Length,
    OUT PVOID           Buffer,
    IN BOOLEAN          bVerify );

NTSTATUS
Ext2ReadDisk(
    IN PEXT2_VCB       Vcb,
    IN ULONGLONG       Offset,
    IN ULONG           Size,
    IN PVOID           Buffer,
    IN BOOLEAN         bVerify  );

NTSTATUS
Ext2DiskIoControl (
    IN PDEVICE_OBJECT   DeviceOjbect,
    IN ULONG            IoctlCode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferSize,
    IN OUT PVOID        OutputBuffer,
    IN OUT PULONG       OutputBufferSize );

VOID
Ext2MediaEjectControl (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN BOOLEAN bPrevent );

NTSTATUS
Ext2DiskShutDown(PEXT2_VCB Vcb);


//
// Cleanup.c
//

NTSTATUS
Ext2Cleanup (IN PEXT2_IRP_CONTEXT IrpContext);

//
// Close.c
//

NTSTATUS
Ext2Close (IN PEXT2_IRP_CONTEXT IrpContext);

VOID
Ext2QueueCloseRequest (IN PEXT2_IRP_CONTEXT IrpContext);

VOID NTAPI
Ext2DeQueueCloseRequest (IN PVOID Context);

//
// Cmcb.c
//

BOOLEAN NTAPI
Ext2AcquireForLazyWrite (
    IN PVOID    Context,
    IN BOOLEAN  Wait );
VOID NTAPI
Ext2ReleaseFromLazyWrite (IN PVOID Context);

BOOLEAN NTAPI
Ext2AcquireForReadAhead (
    IN PVOID    Context,
    IN BOOLEAN  Wait );

VOID NTAPI
Ext2ReleaseFromReadAhead (IN PVOID Context);

BOOLEAN NTAPI
Ext2NoOpAcquire (
    IN PVOID Fcb,
    IN BOOLEAN Wait );

VOID NTAPI
Ext2NoOpRelease (IN PVOID Fcb);

VOID NTAPI
Ext2AcquireForCreateSection (
    IN PFILE_OBJECT FileObject
);

VOID NTAPI
Ext2ReleaseForCreateSection (
    IN PFILE_OBJECT FileObject
);

NTSTATUS NTAPI
Ext2AcquireFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER EndingOffset,
    OUT PERESOURCE *ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS NTAPI
Ext2ReleaseFileForModWrite (
    IN PFILE_OBJECT FileObject,
    IN PERESOURCE ResourceToRelease,
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS NTAPI
Ext2AcquireFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
);

NTSTATUS NTAPI
Ext2ReleaseFileForCcFlush (
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject
);


//
// Create.c
//


BOOLEAN
Ext2IsNameValid(PUNICODE_STRING FileName);

NTSTATUS
Ext2FollowLink (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Parent,
    IN PEXT2_MCB            Mcb,
    IN USHORT               Linkdep
);

NTSTATUS
Ext2ScanDir (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Parent,
    IN PUNICODE_STRING      FileName,
    OUT PULONG              Inode,
    struct dentry         **dentry
);

BOOLEAN
Ext2IsSpecialSystemFile(
    IN PUNICODE_STRING FileName,
    IN BOOLEAN         bDirectory
);

NTSTATUS
Ext2LookupFile (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PUNICODE_STRING      FullName,
    IN PEXT2_MCB            Parent,
    OUT PEXT2_MCB *         Ext2Mcb,
    IN USHORT               Linkdep
);

NTSTATUS
Ext2CreateFile(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    OUT PBOOLEAN OpPostIrp
);

NTSTATUS
Ext2CreateVolume(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb );

NTSTATUS
Ext2Create (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2CreateInode(
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           pParentFcb,
    IN ULONG               Type,
    IN ULONG               FileAttr,
    IN PUNICODE_STRING     FileName);


NTSTATUS
Ext2SupersedeOrOverWriteFile(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PFILE_OBJECT      FileObject,
    IN PEXT2_VCB         Vcb,
    IN PEXT2_FCB         Fcb,
    IN PLARGE_INTEGER    AllocationSize,
    IN ULONG             Disposition
);

//
// Debug.c
//

/* debug levels */
#define DL_NVR 0
#define DL_VIT 0x00000001
#define DL_ERR 0x00000002
#define DL_DBG 0x00000004
#define DL_INF 0x00000008
#define DL_FUN 0x00000010
#define DL_LOW 0x00000020
#define DL_REN 0x00000040   /* renaming operation */
#define DL_RES 0x00000080   /* entry reference managment */
#define DL_BLK 0x00000100   /* data block allocation / free */
#define DL_CP  0x00000200   /* code pages (create, querydir) */
#define DL_EXT 0x00000400   /* mcb extents */
#define DL_MAP 0x00000800   /* retrieval points */
#define DL_JNL 0x00001000   /* dump journal operations */
#define DL_HTI 0x00002000   /* htree index */
#define DL_WRN 0x00004000   /* warning */
#define DL_BH  0x00008000   /* buffer head */
#define DL_PNP 0x00010000   /* pnp */
#define DL_IO  0x00020000   /* file i/o */

#define DL_ALL (DL_ERR|DL_VIT|DL_DBG|DL_INF|DL_FUN|DL_LOW|DL_REN|DL_RES|DL_BLK|DL_CP|DL_EXT|DL_MAP|DL_JNL|DL_HTI|DL_WRN|DL_BH|DL_PNP|DL_IO)

#if EXT2_DEBUG && defined(__REACTOS__)
  #define DL_DEFAULT (DL_ERR|DL_VIT|DL_DBG|DL_INF|DL_FUN|DL_LOW|DL_WRN)
#else
  #define DL_DEFAULT (DL_ERR|DL_VIT)
#endif

#if EXT2_DEBUG
extern  ULONG DebugFilter;

VOID
__cdecl
Ext2NiPrintf(
    PCHAR DebugMessage,
    ...
);

#define DEBUG(_DL, arg)    do {if ((_DL) & DebugFilter) Ext2Printf arg;} while(0)
#define DEBUGNI(_DL, arg)  do {if ((_DL) & DebugFilter) Ext2NiPrintf arg;} while(0)

#define Ext2CompleteRequest(Irp, bPrint, PriorityBoost) \
        Ext2DbgPrintComplete(Irp, bPrint); \
        IoCompleteRequest(Irp, PriorityBoost)

#else

#define DEBUG(_DL, arg) do {if ((_DL) & DL_ERR) DbgPrint arg;} while(0)

#define Ext2CompleteRequest(Irp, bPrint, PriorityBoost) \
        IoCompleteRequest(Irp, PriorityBoost)

#endif // EXT2_DEBUG

VOID
__cdecl
Ext2Printf(
    PCHAR DebugMessage,
    ...
);

extern ULONG ProcessNameOffset;

#define Ext2GetCurrentProcessName() ( \
    (PUCHAR) PsGetCurrentProcess() + ProcessNameOffset \
)

ULONG
Ext2GetProcessNameOffset (VOID);

VOID
Ext2DbgPrintCall (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp );

VOID
Ext2DbgPrintComplete (
    IN PIRP Irp,
    IN BOOLEAN bPrint
);

PUCHAR
Ext2NtStatusToString (IN NTSTATUS Status );

PVOID Ext2AllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
);

VOID
Ext2FreePool(
    IN PVOID P,
    IN ULONG Tag
);

//
// Devctl.c
//

NTSTATUS
Ext2ProcessGlobalProperty(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PEXT2_VOLUME_PROPERTY2 Property,
    IN  ULONG Length
);

NTSTATUS
Ext2ProcessVolumeProperty(
    IN  PEXT2_VCB              Vcb,
    IN  PEXT2_VOLUME_PROPERTY2 Property,
    IN  ULONG Length
);

NTSTATUS
Ext2ProcessUserProperty(
    IN PEXT2_IRP_CONTEXT        IrpContext,
    IN PEXT2_VOLUME_PROPERTY2   Property,
    IN ULONG                    Length
);

NTSTATUS
Ex2ProcessUserPerfStat(
    IN PEXT2_IRP_CONTEXT        IrpContext,
    IN PEXT2_QUERY_PERFSTAT     QueryPerf,
    IN ULONG                    Length
);

NTSTATUS
Ex2ProcessMountPoint(
    IN PEXT2_IRP_CONTEXT        IrpContext,
    IN PEXT2_MOUNT_POINT        MountPoint,
    IN ULONG                    Length
);

NTSTATUS
Ext2DeviceControlNormal (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2PrepareToUnload (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2DeviceControl (IN PEXT2_IRP_CONTEXT IrpContext);

//
// Dirctl.c
//

ULONG
Ext2GetInfoLength(IN FILE_INFORMATION_CLASS  FileInformationClass);

NTSTATUS
Ext2ProcessEntry(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Dcb,
    IN FILE_INFORMATION_CLASS  FileInformationClass,
    IN ULONG                in,
    IN PVOID                Buffer,
    IN ULONG                UsedLength,
    IN ULONG                Length,
    IN ULONG                FileIndex,
    IN PUNICODE_STRING      pName,
    OUT PULONG              EntrySize,
    IN BOOLEAN              Single
);

BOOLEAN
Ext2IsWearingCloak(
    IN  PEXT2_VCB       Vcb,
    IN  POEM_STRING     OeName
);

NTSTATUS Ext2QueryDirectory (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2NotifyChangeDirectory (
    IN PEXT2_IRP_CONTEXT IrpContext
);

VOID
Ext2NotifyReportChange (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN PEXT2_MCB         Mcb,
    IN ULONG             Filter,
    IN ULONG             Action
);

NTSTATUS
Ext2DirectoryControl (IN PEXT2_IRP_CONTEXT IrpContext);

BOOLEAN
Ext2IsDirectoryEmpty (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb
);

//
// Dispatch.c
//

VOID NTAPI
Ext2OplockComplete (
    IN PVOID Context,
    IN PIRP Irp
);

VOID NTAPI
Ext2LockIrp (
    IN PVOID Context,
    IN PIRP Irp
);

NTSTATUS
Ext2QueueRequest (IN PEXT2_IRP_CONTEXT IrpContext);

VOID NTAPI
Ext2DeQueueRequest (IN PVOID Context);

NTSTATUS
Ext2DispatchRequest (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS NTAPI
Ext2BuildRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
);

//
// Except.c
//

NTSTATUS
Ext2ExceptionFilter (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
);

NTSTATUS
Ext2ExceptionHandler (IN PEXT2_IRP_CONTEXT IrpContext);


//
// Extents.c
//


NTSTATUS
Ext2MapExtent(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Index,
    IN BOOLEAN              Alloc,
    OUT PULONG              Block,
    OUT PULONG              Number
    );

NTSTATUS
Ext2ExpandExtent(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    ULONG             Start,
    ULONG             End,
    PLARGE_INTEGER    Size
    );

NTSTATUS
Ext2TruncateExtent(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
    );


//
// generic.c
//

static inline ext3_fsblk_t ext3_blocks_count(struct ext3_super_block *es)
{
    return ((ext3_fsblk_t)le32_to_cpu(es->s_blocks_count_hi) << 32) |
           le32_to_cpu(es->s_blocks_count);
}

static inline ext3_fsblk_t ext3_r_blocks_count(struct ext3_super_block *es)
{
    return ((ext3_fsblk_t)le32_to_cpu(es->s_r_blocks_count_hi) << 32) |
           le32_to_cpu(es->s_r_blocks_count);
}

static inline ext3_fsblk_t ext3_free_blocks_count(struct ext3_super_block *es)
{
    return ((ext3_fsblk_t)le32_to_cpu(es->s_free_blocks_count_hi) << 32) |
           le32_to_cpu(es->s_free_blocks_count);
}

static inline void ext3_blocks_count_set(struct ext3_super_block *es,
        ext3_fsblk_t blk)
{
    es->s_blocks_count = cpu_to_le32((u32)blk);
    es->s_blocks_count_hi = cpu_to_le32(blk >> 32);
}

static inline void ext3_free_blocks_count_set(struct ext3_super_block *es,
        ext3_fsblk_t blk)
{
    es->s_free_blocks_count = cpu_to_le32((u32)blk);
    es->s_free_blocks_count_hi = cpu_to_le32(blk >> 32);
}

static inline void ext3_r_blocks_count_set(struct ext3_super_block *es,
        ext3_fsblk_t blk)
{
    es->s_r_blocks_count = cpu_to_le32((u32)blk);
    es->s_r_blocks_count_hi = cpu_to_le32(blk >> 32);
}

blkcnt_t ext3_inode_blocks(struct ext3_inode *raw_inode,
                           struct inode *inode);

int ext3_inode_blocks_set(struct ext3_inode *raw_inode,
                          struct inode * inode);
ext4_fsblk_t ext4_block_bitmap(struct super_block *sb,
                               struct ext4_group_desc *bg);

ext4_fsblk_t ext4_inode_bitmap(struct super_block *sb,
                               struct ext4_group_desc *bg);
ext4_fsblk_t ext4_inode_table(struct super_block *sb,
                              struct ext4_group_desc *bg);
__u32 ext4_free_blks_count(struct super_block *sb,
                           struct ext4_group_desc *bg);
__u32 ext4_free_inodes_count(struct super_block *sb,
                             struct ext4_group_desc *bg);
__u32 ext4_used_dirs_count(struct super_block *sb,
                           struct ext4_group_desc *bg);
__u32 ext4_itable_unused_count(struct super_block *sb,
                               struct ext4_group_desc *bg);
void ext4_block_bitmap_set(struct super_block *sb,
                           struct ext4_group_desc *bg, ext4_fsblk_t blk);
void ext4_inode_bitmap_set(struct super_block *sb,
                           struct ext4_group_desc *bg, ext4_fsblk_t blk);
void ext4_inode_table_set(struct super_block *sb,
                          struct ext4_group_desc *bg, ext4_fsblk_t blk);
void ext4_free_blks_set(struct super_block *sb,
                        struct ext4_group_desc *bg, __u32 count);
void ext4_free_inodes_set(struct super_block *sb,
                          struct ext4_group_desc *bg, __u32 count);
void ext4_used_dirs_set(struct super_block *sb,
                        struct ext4_group_desc *bg, __u32 count);
void ext4_itable_unused_set(struct super_block *sb,
                            struct ext4_group_desc *bg, __u32 count);

int ext3_bg_has_super(struct super_block *sb, ext3_group_t group);
unsigned long ext4_bg_num_gdb(struct super_block *sb, ext4_group_t group);
unsigned ext4_init_inode_bitmap(struct super_block *sb, struct buffer_head *bh,
                                ext4_group_t block_group,
                                struct ext4_group_desc *gdp);
unsigned ext4_init_block_bitmap(struct super_block *sb, struct buffer_head *bh,
                                ext4_group_t block_group, struct ext4_group_desc *gdp);
struct ext4_group_desc * ext4_get_group_desc(struct super_block *sb,
                    ext4_group_t block_group, struct buffer_head **bh);
ext4_fsblk_t ext4_count_free_blocks(struct super_block *sb);
unsigned long ext4_count_free_inodes(struct super_block *sb);
int ext4_check_descriptors(struct super_block *sb);

NTSTATUS
Ext2LoadSuper(
    IN PEXT2_VCB      Vcb,
    IN BOOLEAN        bVerify,
    OUT PEXT2_SUPER_BLOCK * Sb
);


BOOLEAN
Ext2SaveSuper(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
);

BOOLEAN
Ext2RefreshSuper(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
);

BOOLEAN
Ext2LoadGroup(IN PEXT2_VCB Vcb);

VOID
Ext2PutGroup(IN PEXT2_VCB Vcb);

BOOLEAN
Ext2SaveGroup(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Group
);

BOOLEAN
Ext2RefreshGroup(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
);

BOOLEAN
Ext2GetInodeLba (
    IN PEXT2_VCB Vcb,
    IN ULONG inode,
    OUT PLONGLONG offset
);

BOOLEAN
Ext2LoadInode (
    IN PEXT2_VCB Vcb,
    IN struct inode *Inode
);

BOOLEAN
Ext2ClearInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB Vcb,
    IN ULONG inode
);

BOOLEAN
Ext2SaveInode (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN struct inode *Inode
);

BOOLEAN
Ext2LoadBlock (
    IN PEXT2_VCB Vcb,
    IN ULONG     dwBlk,
    IN PVOID     Buffer
);

BOOLEAN
Ext2SaveBlock (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                dwBlk,
    IN PVOID                Buf
);

BOOLEAN
Ext2ZeroBuffer(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN LONGLONG             Offset,
    IN ULONG                Size
);

BOOLEAN
Ext2SaveBuffer(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN LONGLONG             Offset,
    IN ULONG                Size,
    IN PVOID                Buf
);

NTSTATUS
Ext2GetBlock(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Base,
    IN ULONG                Layer,
    IN ULONG                Start,
    IN ULONG                SizeArray,
    IN PULONG               BlockArray,
    IN BOOLEAN              bAlloc,
    IN OUT PULONG           Hint,
    OUT PULONG              Block,
    OUT PULONG              Number
);

NTSTATUS
Ext2BlockMap(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Index,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock,
    OUT PULONG              Number
);

VOID
Ext2UpdateVcbStat(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb
);

NTSTATUS
Ext2NewBlock(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                GroupHint,
    IN ULONG                BlockHint,
    OUT PULONG              Block,
    IN OUT PULONG           Number
);

NTSTATUS
Ext2FreeBlock(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Block,
    IN ULONG                Number
);


NTSTATUS
Ext2NewInode(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                GroupHint,
    IN ULONG                Type,
    OUT PULONG              Inode
);

NTSTATUS
Ext2FreeInode(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN ULONG                Inode,
    IN ULONG                Type
);

NTSTATUS
Ext2AddEntry (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Dcb,
    IN struct inode       *Inode,
    IN PUNICODE_STRING     FileName,
    OUT struct dentry    **dentry
);

NTSTATUS
Ext2RemoveEntry (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Dcb,
    IN PEXT2_MCB            Mcb
);

NTSTATUS
Ext2SetParentEntry (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Dcb,
    IN ULONG               OldParent,
    IN ULONG               NewParent );


NTSTATUS
Ext2TruncateBlock(
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN PEXT2_MCB         Mcb,
    IN ULONG             Base,
    IN ULONG             Start,
    IN ULONG             Layer,
    IN ULONG             SizeArray,
    IN PULONG            BlockArray,
    IN PULONG            Extra
);

struct ext3_dir_entry_2 *ext3_next_entry(struct ext3_dir_entry_2 *p);

int ext3_check_dir_entry (const char * function, struct inode * dir,
                          struct ext3_dir_entry_2 * de,
                          struct buffer_head * bh,
                          unsigned long offset);

loff_t ext3_max_size(int blkbits, int has_huge_files);
loff_t ext3_max_bitmap_size(int bits, int has_huge_files);


__le16 ext4_group_desc_csum(struct ext3_sb_info *sbi, __u32 block_group,
                            struct ext4_group_desc *gdp);
int ext4_group_desc_csum_verify(struct ext3_sb_info *sbi, __u32 block_group,
                                struct ext4_group_desc *gdp);

ext3_fsblk_t descriptor_loc(struct super_block *sb,
                            ext3_fsblk_t logical_sb_block, unsigned int nr);
struct ext4_group_desc * ext4_get_group_desc(struct super_block *sb,
                    ext4_group_t block_group, struct buffer_head **bh);
int ext4_check_descriptors(struct super_block *sb);

//
// Fastio.c
//

FAST_IO_POSSIBLE
Ext2IsFastIoPossible(
    IN PEXT2_FCB Fcb
);

BOOLEAN NTAPI
Ext2FastIoCheckIfPossible (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    IN BOOLEAN              CheckForReadOperation,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
);


BOOLEAN NTAPI
Ext2FastIoRead (IN PFILE_OBJECT FileObject,
                IN PLARGE_INTEGER       FileOffset,
                IN ULONG                Length,
                IN BOOLEAN              Wait,
                IN ULONG                LockKey,
                OUT PVOID               Buffer,
                OUT PIO_STATUS_BLOCK    IoStatus,
                IN PDEVICE_OBJECT       DeviceObject);

BOOLEAN NTAPI
Ext2FastIoWrite (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN ULONG                Length,
    IN BOOLEAN              Wait,
    IN ULONG                LockKey,
    OUT PVOID               Buffer,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject);

BOOLEAN NTAPI
Ext2FastIoQueryBasicInfo (
    IN PFILE_OBJECT             FileObject,
    IN BOOLEAN                  Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK        IoStatus,
    IN PDEVICE_OBJECT           DeviceObject);

BOOLEAN NTAPI
Ext2FastIoQueryStandardInfo (
    IN PFILE_OBJECT                 FileObject,
    IN BOOLEAN                      Wait,
    OUT PFILE_STANDARD_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK            IoStatus,
    IN PDEVICE_OBJECT               DeviceObject);

BOOLEAN NTAPI
Ext2FastIoLock (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              FailImmediately,
    IN BOOLEAN              ExclusiveLock,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
);

BOOLEAN NTAPI
Ext2FastIoUnlockSingle (
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
);

BOOLEAN NTAPI
Ext2FastIoUnlockAll (
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
);

BOOLEAN NTAPI
Ext2FastIoUnlockAllByKey (
    IN PFILE_OBJECT         FileObject,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PDEVICE_OBJECT       DeviceObject
);


BOOLEAN NTAPI
Ext2FastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject );

BOOLEAN NTAPI
Ext2FastIoQueryNetworkOpenInfo (
    IN PFILE_OBJECT                     FileObject,
    IN BOOLEAN                          Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
    OUT PIO_STATUS_BLOCK                IoStatus,
    IN PDEVICE_OBJECT                   DeviceObject);


//
// FileInfo.c
//


NTSTATUS
Ext2QueryFileInformation (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2SetFileInformation (IN PEXT2_IRP_CONTEXT IrpContext);

ULONG
Ext2TotalBlocks(
    PEXT2_VCB         Vcb,
    PLARGE_INTEGER    Size,
    PULONG            pMeta
);

NTSTATUS
Ext2ExpandFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
);

NTSTATUS
Ext2TruncateFile (
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_MCB Mcb,
    PLARGE_INTEGER AllocationSize );

NTSTATUS
Ext2IsFileRemovable(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_FCB            Fcb,
    IN PEXT2_CCB            Ccb
);

NTSTATUS
Ext2SetDispositionInfo(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb,
    PEXT2_CCB Ccb,
    BOOLEAN bDelete
);

NTSTATUS
Ext2SetRenameInfo(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb,
    PEXT2_CCB Ccb
);

ULONG
Ext2InodeType(PEXT2_MCB Mcb);

NTSTATUS
Ext2DeleteFile(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_FCB Fcb,
    PEXT2_MCB Mcb
);


//
// Flush.c
//

NTSTATUS
Ext2FlushFiles(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN BOOLEAN              bShutDown
);

NTSTATUS
Ext2FlushVolume (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN BOOLEAN              bShutDown
);

NTSTATUS
Ext2FlushFile (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_FCB            Fcb,
    IN PEXT2_CCB            Ccb
);

NTSTATUS
Ext2Flush (IN PEXT2_IRP_CONTEXT IrpContext);


//
// Fsctl.c
//

//
// MountPoint process workitem
//

VOID
Ext2SetVpbFlag (IN PVPB     Vpb,
                IN USHORT   Flag );

VOID
Ext2ClearVpbFlag (IN PVPB     Vpb,
                  IN USHORT   Flag );

BOOLEAN
Ext2CheckDismount (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB         Vcb,
    IN BOOLEAN           bForce   );

NTSTATUS
Ext2PurgeVolume (IN PEXT2_VCB Vcb,
                 IN BOOLEAN  FlushBeforePurge);

NTSTATUS
Ext2PurgeFile (IN PEXT2_FCB Fcb,
               IN BOOLEAN  FlushBeforePurge);

BOOLEAN
Ext2IsHandleCountZero(IN PEXT2_VCB Vcb);

NTSTATUS
Ext2LockVcb (IN PEXT2_VCB    Vcb,
             IN PFILE_OBJECT FileObject);

NTSTATUS
Ext2LockVolume (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2UnlockVcb (IN PEXT2_VCB    Vcb,
               IN PFILE_OBJECT FileObject);

NTSTATUS
Ext2UnlockVolume (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2AllowExtendedDasdIo(IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2OplockRequest (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2QueryExtentMappings(
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PEXT2_VCB           Vcb,
    IN PEXT2_FCB           Fcb,
    IN PLARGE_INTEGER      RequestVbn,
    OUT PLARGE_INTEGER *   pMappedRuns
);

NTSTATUS
Ext2QueryRetrievalPointers(IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2GetRetrievalPointers(IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2GetRetrievalPointerBase(IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2UserFsRequest (IN PEXT2_IRP_CONTEXT IrpContext);

BOOLEAN
Ext2IsMediaWriteProtected (
    IN PEXT2_IRP_CONTEXT   IrpContext,
    IN PDEVICE_OBJECT TargetDevice
);

NTSTATUS
Ext2MountVolume (IN PEXT2_IRP_CONTEXT IrpContext);

VOID
Ext2VerifyVcb (IN PEXT2_IRP_CONTEXT IrpContext,
               IN PEXT2_VCB         Vcb );
NTSTATUS
Ext2VerifyVolume (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2IsVolumeMounted (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2DismountVolume (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2FileSystemControl (IN PEXT2_IRP_CONTEXT IrpContext);

//
// HTree.c
//

struct buffer_head *ext3_append(struct ext2_icb *icb, struct inode *inode,
                                            ext3_lblk_t *block, int *err);

void ext3_set_de_type(struct super_block *sb,
                      struct ext3_dir_entry_2 *de,
                      umode_t mode);

__u32 ext3_current_time(struct inode *in);
void ext3_warning (struct super_block * sb, const char * function,
                   char * fmt, ...);
#define ext3_error ext3_warning
#define ext4_error ext3_error

void ext3_update_dx_flag(struct inode *inode);
int ext3_mark_inode_dirty(struct ext2_icb *icb, struct inode *in);

void ext3_inc_count(struct inode *inode);
void ext3_dec_count(struct inode *inode);

struct buffer_head *
            ext3_find_entry (struct ext2_icb *icb, struct dentry *dentry,
                             struct ext3_dir_entry_2 ** res_dir);
struct buffer_head *
            ext3_dx_find_entry(struct ext2_icb *, struct dentry *dentry,
                               struct ext3_dir_entry_2 **res_dir, int *err);

typedef int (*filldir_t)(void *, const char *, int, unsigned long, __u32, unsigned);
int ext3_dx_readdir(struct file *filp, filldir_t filldir, void * context);

struct buffer_head *ext3_bread(struct ext2_icb *icb, struct inode *inode,
                                           unsigned long block, int *err);
int add_dirent_to_buf(struct ext2_icb *icb, struct dentry *dentry,
                      struct inode *inode, struct ext3_dir_entry_2 *de,
                      struct buffer_head *bh);

#if !defined(__REACTOS__) || defined(_MSC_VER)
struct ext3_dir_entry_2 *
            do_split(struct ext2_icb *icb, struct inode *dir,
                     struct buffer_head **bh,struct dx_frame *frame,
                     struct dx_hash_info *hinfo, int *error);
#endif

int ext3_add_entry(struct ext2_icb *icb, struct dentry *dentry, struct inode *inode);

int ext3_delete_entry(struct ext2_icb *icb, struct inode *dir,
                      struct ext3_dir_entry_2 *de_del,
                      struct buffer_head *bh);

int ext3_is_dir_empty(struct ext2_icb *icb, struct inode *inode);

//
// Init.c
//

BOOLEAN
Ext2QueryGlobalParameters (IN PUNICODE_STRING  RegistryPath);

VOID NTAPI
DriverUnload (IN PDRIVER_OBJECT DriverObject);

//
// Indirect.c
//

NTSTATUS
Ext2MapIndirect(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONG                Index,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock,
    OUT PULONG              Number
);

NTSTATUS
Ext2ExpandIndirect(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    ULONG             Start,
    ULONG             End,
    PLARGE_INTEGER    Size
);

NTSTATUS
Ext2TruncateIndirect(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb,
    PEXT2_MCB         Mcb,
    PLARGE_INTEGER    Size
);


//
// linux.c: linux lib implemenation
//

int
ext2_init_linux();

void
ext2_destroy_linux();


//
// Lock.c
//

NTSTATUS
Ext2LockControl (IN PEXT2_IRP_CONTEXT IrpContext);


//
// Memory.c
//

PEXT2_IRP_CONTEXT
Ext2AllocateIrpContext (IN PDEVICE_OBJECT   DeviceObject,
                        IN PIRP             Irp );

VOID
Ext2FreeIrpContext (IN PEXT2_IRP_CONTEXT IrpContext);


PEXT2_FCB
Ext2AllocateFcb (
    IN PEXT2_VCB   Vcb,
    IN PEXT2_MCB   Mcb
);

VOID
Ext2FreeFcb (IN PEXT2_FCB Fcb);

VOID
Ext2InsertFcb(PEXT2_VCB Vcb, PEXT2_FCB Fcb);

VOID
Ext2RemoveFcb(PEXT2_VCB Vcb, PEXT2_FCB Fcb);

PEXT2_CCB
Ext2AllocateCcb (PEXT2_MCB  SymLink);

VOID
Ext2FreeMcb (
    IN PEXT2_VCB        Vcb,
    IN PEXT2_MCB        Mcb
);

VOID
Ext2FreeCcb (IN PEXT2_VCB Vcb, IN PEXT2_CCB Ccb);

PEXT2_INODE
Ext2AllocateInode (PEXT2_VCB  Vcb);

VOID
Ext2DestroyInode (IN PEXT2_VCB Vcb, IN PEXT2_INODE inode);

struct dentry * Ext2AllocateEntry();
VOID Ext2FreeEntry (IN struct dentry *de);
struct dentry *Ext2BuildEntry(PEXT2_VCB Vcb, PEXT2_MCB Dcb, PUNICODE_STRING FileName);

PEXT2_EXTENT
Ext2AllocateExtent();

VOID
Ext2FreeExtent (IN PEXT2_EXTENT Extent);

ULONG
Ext2CountExtents(IN PEXT2_EXTENT Chain);

VOID
Ext2JointExtents(
    IN PEXT2_EXTENT Chain,
    IN PEXT2_EXTENT Extent
);

VOID
Ext2DestroyExtentChain(IN PEXT2_EXTENT Chain);

NTSTATUS
Ext2BuildExtents(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONGLONG            Offset,
    IN ULONG                Size,
    IN BOOLEAN              bAlloc,
    OUT PEXT2_EXTENT *      Chain
);

BOOLEAN
Ext2ListExtents(PLARGE_MCB  Extents);

VOID
Ext2CheckExtent(
    PLARGE_MCB  Zone,
    LONGLONG    Vbn,
    LONGLONG    Lbn,
    LONGLONG    Length,
    BOOLEAN     bAdded
);

VOID
Ext2ClearAllExtents(PLARGE_MCB  Zone);

BOOLEAN
Ext2AddVcbExtent (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Length
);

BOOLEAN
Ext2RemoveVcbExtent (
    IN PEXT2_VCB Vcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Length
);

BOOLEAN
Ext2LookupVcbExtent (
    IN PEXT2_VCB    Vcb,
    IN LONGLONG     Vbn,
    OUT PLONGLONG   Lbn,
    OUT PLONGLONG   Length
);

BOOLEAN
Ext2AddMcbExtent (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Lbn,
    IN LONGLONG  Length
);

BOOLEAN
Ext2RemoveMcbExtent (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN LONGLONG  Vbn,
    IN LONGLONG  Length
);

BOOLEAN
Ext2LookupMcbExtent (
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN LONGLONG     Vbn,
    OUT PLONGLONG   Lbn,
    OUT PLONGLONG   Length
);

BOOLEAN
Ext2AddMcbMetaExts (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN ULONG     Block,
    IN ULONG     Length
);

BOOLEAN
Ext2RemoveMcbMetaExts (
    IN PEXT2_VCB Vcb,
    IN PEXT2_MCB Mcb,
    IN ULONG     Block,
    IN ULONG     Length
);

BOOLEAN
Ext2AddBlockExtent(
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN ULONG        Start,
    IN ULONG        Block,
    IN ULONG        Number
);

BOOLEAN
Ext2LookupBlockExtent(
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN ULONG        Start,
    IN PULONG       Block,
    IN PULONG       Mapped
);

BOOLEAN
Ext2RemoveBlockExtent(
    IN PEXT2_VCB    Vcb,
    IN PEXT2_MCB    Mcb,
    IN ULONG        Start,
    IN ULONG        Number
);

NTSTATUS
Ext2InitializeZone(
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb
);

BOOLEAN
Ext2BuildName(
    IN OUT PUNICODE_STRING  Target,
    IN PUNICODE_STRING      File,
    IN PUNICODE_STRING      Parent
);


PEXT2_MCB
Ext2AllocateMcb (
    IN PEXT2_VCB        Vcb,
    IN PUNICODE_STRING  FileName,
    IN PUNICODE_STRING  Parent,
    IN ULONG            FileAttr
);

PEXT2_MCB
Ext2SearchMcb(
    PEXT2_VCB           Vcb,
    PEXT2_MCB           Parent,
    PUNICODE_STRING     FileName
);

PEXT2_MCB
Ext2SearchMcbWithoutLock(
    PEXT2_MCB           Parent,
    PUNICODE_STRING     FileName
);

VOID
Ext2InsertMcb(
    PEXT2_VCB Vcb,
    PEXT2_MCB Parent,
    PEXT2_MCB Child
);

BOOLEAN
Ext2RemoveMcb(
    PEXT2_VCB Vcb,
    PEXT2_MCB Mcb
);

VOID
Ext2CleanupAllMcbs(
    PEXT2_VCB Vcb
);

BOOLEAN
Ext2CheckSetBlock(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb, LONGLONG Block
);

BOOLEAN
Ext2CheckBitmapConsistency(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb
);

VOID
Ext2InsertVcb(PEXT2_VCB Vcb);

VOID
Ext2RemoveVcb(PEXT2_VCB Vcb);

NTSTATUS
Ext2InitializeLabel(
    IN PEXT2_VCB            Vcb,
    IN PEXT2_SUPER_BLOCK    Sb
);

NTSTATUS
Ext2InitializeVcb(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb,
    PEXT2_SUPER_BLOCK Ext2Sb,
    PDEVICE_OBJECT TargetDevice,
    PDEVICE_OBJECT VolumeDevice,
    PVPB Vpb                   );

VOID
Ext2TearDownStream (IN PEXT2_VCB Vcb);

VOID
Ext2DestroyVcb (IN PEXT2_VCB Vcb);

NTSTATUS
Ext2CompleteIrpContext (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN NTSTATUS Status );

VOID
Ext2SyncUninitializeCacheMap (
    IN PFILE_OBJECT FileObject    );

VOID
Ext2LinkTailMcb(PEXT2_VCB Vcb, PEXT2_MCB Mcb);


VOID
Ext2LinkHeadMcb(PEXT2_VCB Vcb, PEXT2_MCB Mcb);

VOID
Ext2UnlinkMcb(PEXT2_VCB Vcb, PEXT2_MCB Mcb);

PEXT2_MCB
Ext2FirstUnusedMcb(
    PEXT2_VCB   Vcb,
    BOOLEAN     Wait,
    ULONG       Number
);

VOID NTAPI
Ext2ReaperThread(
    PVOID   Context
);

NTSTATUS
Ext2StartReaperThread();

//
// Misc.c
//

ULONG
Ext2Log2(ULONG Value);

LARGE_INTEGER
Ext2NtTime (IN ULONG i_time);

ULONG
Ext2LinuxTime (IN LARGE_INTEGER SysTime);

ULONG
Ext2OEMToUnicodeSize(
    IN PEXT2_VCB        Vcb,
    IN PANSI_STRING     Oem
);

NTSTATUS
Ext2OEMToUnicode(
    IN PEXT2_VCB           Vcb,
    IN OUT PUNICODE_STRING Oem,
    IN POEM_STRING         Unicode
);

ULONG
Ext2UnicodeToOEMSize(
    IN PEXT2_VCB        Vcb,
    IN PUNICODE_STRING  Unicode
);

NTSTATUS
Ext2UnicodeToOEM (
    IN PEXT2_VCB        Vcb,
    IN OUT POEM_STRING  Oem,
    IN PUNICODE_STRING  Unicode
);

VOID
Ext2Sleep(ULONG ms);

int Ext2LinuxError (NTSTATUS Status);
NTSTATUS Ext2WinntError(int rc);

BOOLEAN Ext2IsDot(PUNICODE_STRING name);
BOOLEAN Ext2IsDotDot(PUNICODE_STRING name);
//
// nls/nls_rtl.c
//

int
Ext2LoadAllNls();

VOID
Ext2UnloadAllNls();

//
// Pnp.c
//

NTSTATUS
Ext2Pnp(IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2PnpQueryRemove(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb      );

NTSTATUS
Ext2PnpRemove(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB         Vcb      );

NTSTATUS
Ext2PnpCancelRemove(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb              );

NTSTATUS
Ext2PnpSurpriseRemove(
    PEXT2_IRP_CONTEXT IrpContext,
    PEXT2_VCB Vcb              );


//
// Read.c
//

NTSTATUS
Ext2ReadInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bDirectIo,
    OUT PULONG              dwReturn
);

NTSTATUS
Ext2Read (IN PEXT2_IRP_CONTEXT IrpContext);


//
// ext3\recover.c
//

PEXT2_MCB
Ext2LoadInternalJournal(
    PEXT2_VCB         Vcb,
    ULONG             jNo
);

INT
Ext2CheckJournal(
    PEXT2_VCB          Vcb,
    PULONG             jNo
);

INT
Ext2RecoverJournal(
    PEXT2_IRP_CONTEXT  IrpContext,
    PEXT2_VCB          Vcb
);

//
// Shutdown.c
//


NTSTATUS
Ext2ShutDown (IN PEXT2_IRP_CONTEXT IrpContext);


//
// Volinfo.c
//

NTSTATUS
Ext2QueryVolumeInformation (IN PEXT2_IRP_CONTEXT IrpContext);

NTSTATUS
Ext2SetVolumeInformation (IN PEXT2_IRP_CONTEXT IrpContext);

//
// Write.c
//

typedef struct _EXT2_RW_CONTEXT {
    PIRP                MasterIrp;
    KEVENT              Event;
    ULONG               Blocks;
    ULONG               Length;
    PERESOURCE          Resource;
    ERESOURCE_THREAD    ThreadId;
    PFILE_OBJECT        FileObject;
    ULONG               Flags;
    BOOLEAN             Wait;

} EXT2_RW_CONTEXT, *PEXT2_RW_CONTEXT;

#define EXT2_RW_CONTEXT_WRITE   1

NTSTATUS
Ext2WriteInode (
    IN PEXT2_IRP_CONTEXT    IrpContext,
    IN PEXT2_VCB            Vcb,
    IN PEXT2_MCB            Mcb,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bDirectIo,
    OUT PULONG              dwReturn
);

VOID
Ext2StartFloppyFlushDpc (
    PEXT2_VCB   Vcb,
    PEXT2_FCB   Fcb,
    PFILE_OBJECT FileObject );

BOOLEAN
Ext2ZeroData (
    IN PEXT2_IRP_CONTEXT IrpContext,
    IN PEXT2_VCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER Start,
    IN PLARGE_INTEGER End );

NTSTATUS
Ext2Write (IN PEXT2_IRP_CONTEXT IrpContext);

#endif /* _EXT2_HEADER_ */
