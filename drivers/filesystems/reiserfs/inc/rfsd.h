/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             rfsd.h
 * PURPOSE:          Header file: rfsd structures.
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

#ifndef _RFSD_HEADER_
#define _RFSD_HEADER_

#ifdef _MSC_VER
#ifndef _PREFAST_
#pragma warning(disable:4068)
#define __drv_mustHoldCriticalRegion
#endif // !_PREFAST_
#endif

/* INCLUDES *************************************************************/

#include <linux/module.h>
//#include <linux/reiserfs_fs.h>	// Full ReiserFS header
#include "reiserfs.h"				// Simplified ReiserFS header
#ifdef __REACTOS__
#include <ndk/rtlfuncs.h>
#include <pseh/pseh2.h>
typedef IO_STACK_LOCATION EXTENDED_IO_STACK_LOCATION, *PEXTENDED_IO_STACK_LOCATION;
#endif

typedef struct reiserfs_super_block_v1		RFSD_SUPER_BLOCK, *PRFSD_SUPER_BLOCK;
typedef struct stat_data					RFSD_INODE, *PRFSD_INODE;

#define RFSD_CALLBACK(name)	NTSTATUS(* name )(ULONG BlockNumber, PVOID pContext)


typedef struct block_head					RFSD_BLOCK_HEAD, *PRFSD_BLOCK_HEAD;		// [mark]
typedef struct reiserfs_de_head				RFSD_DENTRY_HEAD, *PRFSD_DENTRY_HEAD;	// [mark]
typedef struct item_head					RFSD_ITEM_HEAD, *PRFSD_ITEM_HEAD;		// [mark]
typedef struct reiserfs_key					RFSD_KEY_ON_DISK, *PRFSD_KEY_ON_DISK;
typedef struct reiserfs_cpu_key				RFSD_KEY_IN_MEMORY, *PRFSD_KEY_IN_MEMORY;		
typedef struct disk_child					RFSD_DISK_NODE_REF, *PRFSD_DISK_NODE_REF; 

#define RFSD_NAME_LEN				255			/// Default length of buffers for filenames (although filenames may be longer)

#define SUPER_BLOCK_OFFSET              REISERFS_DISK_OFFSET_IN_BYTES
#define SUPER_BLOCK_SIZE                sizeof(RFSD_SUPER_BLOCK)

#define RFSD_ROOT_PARENT_ID			1			/// Part of the key for the root node
#define RFSD_ROOT_OBJECT_ID			2			/// Part of the key for the root node
#define RFSD_IS_ROOT_KEY(x)			(x.k_dir_id  == RFSD_ROOT_PARENT_ID && x.k_objectid  == RFSD_ROOT_OBJECT_ID)
#define RFSD_IS_PTR_TO_ROOT_KEY(x)	(x->k_dir_id == RFSD_ROOT_PARENT_ID && x->k_objectid == RFSD_ROOT_OBJECT_ID)

typedef short RFSD_KEY_COMPARISON;
typedef __u16 RFSD_KEY_VERSION;

#define RFSD_KEY_VERSION_1			0
#define RFSD_KEY_VERSION_2			1
#define RFSD_KEY_VERSION_UNKNOWN	7

// Results of a key comparison (as returned by CompareKeys)
#define RFSD_KEYS_MATCH			0
#define RFSD_KEY_SMALLER		-1
#define RFSD_KEY_LARGER			1


#define	RFSD_LEAF_BLOCK_LEVEL	1

#include <ntdddisk.h>

#pragma pack(1)

/* DEBUG ****************************************************************/
#if DBG
    #define DbgBreak()  DbgPrint("rfsd: breakpoint requested.\n");DbgBreakPoint()
#else
    #define DbgBreak()  DbgPrint("rfsd: breakpoint ignored.\n")
#endif

/* STRUCTS & CONSTS******************************************************/

#define RFSD_VERSION  "0.26"

//
// Rfsd build options
//

// To build read-only driver

#define RFSD_READ_ONLY  TRUE

// To support driver dynamics unload

#define RFSD_UNLOAD     TRUE

// The Pool Tag

#define RFSD_POOL_TAG 'dsfR'

//
// Constants
//

#define RFSD_BLOCK_TYPES                (0x04)

#define MAXIMUM_RECORD_LENGTH           (0x10000)

#define SECTOR_BITS                     (Vcb->SectorBits)
#define SECTOR_SIZE                     (Vcb->DiskGeometry.BytesPerSector)
#define DEFAULT_SECTOR_SIZE             (0x200)

#define READ_AHEAD_GRANULARITY          (0x10000)

#define SUPER_BLOCK                     (Vcb->SuperBlock)

#define BLOCK_SIZE                      (Vcb->BlockSize)
#define BLOCK_BITS                      (SUPER_BLOCK->s_log_block_size + 10)

#define INODES_COUNT                    (Vcb->SuperBlock->s_inodes_count)

#define INODES_PER_GROUP                (SUPER_BLOCK->s_inodes_per_group)
#define BLOCKS_PER_GROUP                (SUPER_BLOCK->s_blocks_per_group)
#define TOTAL_BLOCKS                    (SUPER_BLOCK->s_blocks_count)

#define RFSD_FIRST_DATA_BLOCK           (SUPER_BLOCK->s_first_data_block)



#define CEILING_ALIGNED(A, B) (((A) + (B) - 1) & (~((B) - 1)))


// The __SLINE__ macro evaluates to a string with the line of the program from which it is called.
// (Note that this requires two levels of macro indirection...)
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __SLINE__ __STR1__(__LINE__)


/* File System Releated *************************************************/

#define DRIVER_NAME      "Rfsd"
#define DEVICE_NAME     L"\\Rfsd"

// Registry

#define PARAMETERS_KEY      L"\\Parameters"

#define WRITING_SUPPORT     L"WritingSupport"
#define CHECKING_BITMAP     L"CheckingBitmap"
#define EXT3_FORCEWRITING   L"Ext3ForceWriting"
#define EXT3_CODEPAGE       L"CodePage"

// To support rfsd unload routine
#if RFSD_UNLOAD

#define DOS_DEVICE_NAME L"\\DosDevices\\Rfsd"

//
// Private IOCTL to make the driver ready to unload
//
#define IOCTL_PREPARE_TO_UNLOAD \
CTL_CODE(FILE_DEVICE_UNKNOWN, 2048, METHOD_NEITHER, FILE_WRITE_ACCESS)

#endif // RFSD_UNLOAD

#ifndef SetFlag
#define SetFlag(x,f)    ((x) |= (f))
#endif

#ifndef ClearFlag
#define ClearFlag(x,f)  ((x) &= ~(f))
#endif

#define IsFlagOn(a,b) ((BOOLEAN)(FlagOn(a,b) == b))

#define RfsdRaiseStatus(IRPCONTEXT,STATUS) {  \
    (IRPCONTEXT)->ExceptionCode = (STATUS); \
    ExRaiseStatus( (STATUS) );                \
}

#define RfsdNormalizeAndRaiseStatus(IRPCONTEXT,STATUS) {                        \
    /* (IRPCONTEXT)->ExceptionStatus = (STATUS);  */                            \
    if ((STATUS) == STATUS_VERIFY_REQUIRED) { ExRaiseStatus((STATUS)); }        \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STATUS),STATUS_UNEXPECTED_IO_ERROR)); \
}

//
// Define IsEndofFile for read and write operations
//

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff
#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe

#define IsEndOfFile(Pos) ((Pos.LowPart == FILE_WRITE_TO_END_OF_FILE) && \
                          (Pos.HighPart == FILE_USE_FILE_POINTER_POSITION ))

#define IsDirectory(Fcb) IsFlagOn(Fcb->RfsdMcb->FileAttr, FILE_ATTRIBUTE_DIRECTORY)

//
// Bug Check Codes Definitions
//

#define RFSD_FILE_SYSTEM   (FILE_SYSTEM)

#define RFSD_BUGCHK_BLOCK               (0x00010000)
#define RFSD_BUGCHK_CLEANUP             (0x00020000)
#define RFSD_BUGCHK_CLOSE               (0x00030000)
#define RFSD_BUGCHK_CMCB                (0x00040000)
#define RFSD_BUGCHK_CREATE              (0x00050000)
#define RFSD_BUGCHK_DEBUG               (0x00060000)
#define RFSD_BUGCHK_DEVCTL              (0x00070000)
#define RFSD_BUGCHK_DIRCTL              (0x00080000)
#define RFSD_BUGCHK_DISPATCH            (0x00090000)
#define RFSD_BUGCHK_EXCEPT              (0x000A0000)
#define RFSD_BUGCHK_RFSD                (0x000B0000)
#define RFSD_BUGCHK_FASTIO              (0x000C0000)
#define RFSD_BUGCHK_FILEINFO            (0x000D0000)
#define RFSD_BUGCHK_FLUSH               (0x000E0000)
#define RFSD_BUGCHK_FSCTL               (0x000F0000)
#define RFSD_BUGCHK_INIT                (0x00100000)
#define RFSD_BUGCHK_LOCK                (0x0011000)
#define RFSD_BUGCHK_MEMORY              (0x0012000)
#define RFSD_BUGCHK_MISC                (0x0013000)
#define RFSD_BUGCHK_READ                (0x00140000)
#define RFSD_BUGCHK_SHUTDOWN            (0x00150000)
#define RFSD_BUGCHK_VOLINFO             (0x00160000)
#define RFSD_BUGCHK_WRITE               (0x00170000)

#define RFSD_BUGCHK_LAST                (0x00170000)

#define RfsdBugCheck(A,B,C,D) { KeBugCheckEx(RFSD_FILE_SYSTEM, A | __LINE__, B, C, D ); }


/* Rfsd file system definions *******************************************/

#define RFSD_MIN_BLOCK      1024
#define RFSD_MIN_FRAG       1024

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
#define S_IRUSR  0x100              /* 0 0400 */
#define S_IWUSR  0x080              /* 0 0200 */
#define S_IXUSR  0x040              /* 0 0100 */

#define S_IRWXG  0x038              /* 0 0070 */
#define S_IRGRP  0x020              /* 0 0040 */
#define S_IWGRP  0x010              /* 0 0020 */
#define S_IXGRP  0x008              /* 0 0010 */

#define S_IRWXO  0x007              /* 0 0007 */
#define S_IROTH  0x004              /* 0 0004 */
#define S_IWOTH  0x002              /* 0 0002 */
#define S_IXOTH  0x001              /* 0 0001 */

#define S_IRWXUGO   (S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO   (S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO     (S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO     (S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO     (S_IXUSR|S_IXGRP|S_IXOTH)

#define S_ISREADABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IRUSR | S_IRGRP | S_IROTH))
#define S_ISWRITABLE(m)    (((m) & S_IPERMISSION_MASK) == (S_IWUSR | S_IWGRP | S_IWOTH))

#define RfsdSetReadable(m) (m) = ((m) | (S_IRUSR | S_IRGRP | S_IROTH))
#define RfsdSetWritable(m) (m) = ((m) | (S_IWUSR | S_IWGRP | S_IWOTH))

#define RfsdSetReadOnly(m) (m) = ((m) & (~(S_IWUSR | S_IWGRP | S_IWOTH)))
#define RfsdIsReadOnly(m)  (!((m) & (S_IWUSR | S_IWGRP | S_IWOTH)))

//
//  Inode state bits
//

#define I_DIRTY_SYNC		1 /* Not dirty enough for O_DATASYNC */
#define I_DIRTY_DATASYNC	2 /* Data-related inode changes pending */
#define I_DIRTY_PAGES		4 /* Data-related inode changes pending */
#define I_LOCK			8
#define I_FREEING		16
#define I_CLEAR			32

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)


//
// Rfsd Driver Definitions
//

//
// RFSD_IDENTIFIER_TYPE
//
// Identifiers used to mark the structures
//

typedef enum _RFSD_IDENTIFIER_TYPE {
    RFSDFGD  = ':DGF',
    RFSDVCB  = ':BCV',
    RFSDFCB  = ':BCF',
    RFSDCCB  = ':BCC',
    RFSDICX  = ':XCI',
    RFSDMCB  = ':BCM'
} RFSD_IDENTIFIER_TYPE;

//
// RFSD_IDENTIFIER
//
// Header used to mark the structures
//
typedef struct _RFSD_IDENTIFIER {
    RFSD_IDENTIFIER_TYPE     Type;
    ULONG                    Size;
} RFSD_IDENTIFIER, *PRFSD_IDENTIFIER;


#define NodeType(Ptr) (*((RFSD_IDENTIFIER_TYPE *)(Ptr)))

typedef struct _RFSD_MCB  RFSD_MCB, *PRFSD_MCB;


typedef PVOID   PBCB;

//
// REPINNED_BCBS List
//

#define RFSD_REPINNED_BCBS_ARRAY_SIZE         (8)

typedef struct _RFSD_REPINNED_BCBS {

    //
    //  A pointer to the next structure contains additional repinned bcbs
    //

    struct _RFSD_REPINNED_BCBS *Next;

    //
    //  A fixed size array of pinned bcbs.  Whenever a new bcb is added to
    //  the repinned bcb structure it is added to this array.  If the
    //  array is already full then another repinned bcb structure is allocated
    //  and pointed to with Next.
    //

    PBCB Bcb[ RFSD_REPINNED_BCBS_ARRAY_SIZE ];

} RFSD_REPINNED_BCBS, *PRFSD_REPINNED_BCBS;


#define CODEPAGE_MAXLEN     0x20

//
// RFSD_GLOBAL_DATA
//
// Data that is not specific to a mounted volume
//

typedef struct _RFSD_GLOBAL {
    
    // Identifier for this structure
    RFSD_IDENTIFIER             Identifier;
    
    // Syncronization primitive for this structure
    ERESOURCE                   Resource;

    // Syncronization primitive for Counting
    ERESOURCE                   CountResource;

    // Syncronization primitive for LookAside Lists
    ERESOURCE                   LAResource;
    
    // Table of pointers to the fast I/O entry points
    FAST_IO_DISPATCH            FastIoDispatch;
    
    // Table of pointers to the Cache Manager callbacks
    CACHE_MANAGER_CALLBACKS     CacheManagerCallbacks;
    CACHE_MANAGER_CALLBACKS     CacheManagerNoOpCallbacks;
    
    // Pointer to the driver object
    PDRIVER_OBJECT              DriverObject;
    
    // Pointer to the main device object
    PDEVICE_OBJECT              DeviceObject;
    
    // List of mounted volumes
    LIST_ENTRY                  VcbList;

    // Look Aside table of IRP_CONTEXT, FCB, MCB, CCB
    USHORT                      MaxDepth;
    NPAGED_LOOKASIDE_LIST       RfsdIrpContextLookasideList;
    NPAGED_LOOKASIDE_LIST       RfsdFcbLookasideList;
    NPAGED_LOOKASIDE_LIST       RfsdCcbLookasideList;
    PAGED_LOOKASIDE_LIST        RfsdMcbLookasideList;

    // Mcb Count ...
    USHORT                      McbAllocated;

#if DBG
    // Fcb Count
    USHORT                      FcbAllocated;

    // IRP_MJ_CLOSE : FCB
    USHORT                      IRPCloseCount;
#endif
    
    // Global flags for the driver
    ULONG                       Flags;

    // User specified codepage name
    struct {
        WCHAR                   UniName[CODEPAGE_MAXLEN];
        UCHAR                   AnsiName[CODEPAGE_MAXLEN];
        struct nls_table *      PageTable;
    } CodePage;
    
} RFSD_GLOBAL, *PRFSD_GLOBAL;

#define PAGE_TABLE RfsdGlobal->CodePage.PageTable

//
// Flags for RFSD_GLOBAL_DATA
//
#define RFSD_UNLOAD_PENDING     0x00000001
#define RFSD_SUPPORT_WRITING    0x00000002
#define EXT3_FORCE_WRITING      0x00000004
#define RFSD_CHECKING_BITMAP    0x00000008

//
// Driver Extension define
//
typedef struct {
    RFSD_GLOBAL RfsdGlobal;
} RFSDFS_EXT, *PRFSDFS_EXT;


typedef struct _RFSD_FCBVCB {
    
    // FCB header required by NT
    FSRTL_COMMON_FCB_HEADER         CommonFCBHeader;
    SECTION_OBJECT_POINTERS         SectionObject;
    ERESOURCE                       MainResource;
    ERESOURCE                       PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    RFSD_IDENTIFIER                 Identifier;
} RFSD_FCBVCB, *PRFSD_FCBVCB;

//
// RFSD_VCB Volume Control Block
//
// Data that represents a mounted logical volume
// It is allocated as the device extension of the volume device object
//
typedef struct _RFSD_VCB {
    
    // FCB header required by NT
    // The VCB is also used as an FCB for file objects
    // that represents the volume itself
    FSRTL_COMMON_FCB_HEADER     Header;
    SECTION_OBJECT_POINTERS     SectionObject;
    ERESOURCE                   MainResource;
    ERESOURCE                   PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    RFSD_IDENTIFIER             Identifier;
    
    LIST_ENTRY                  Next;
    
    // Share Access for the file object
    SHARE_ACCESS                ShareAccess;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    // for files on this volume.
    ULONG                       OpenFileHandleCount;
    
    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    // for both files on this volume and open instances of the
    // volume itself.
    ULONG                       ReferenceCount;
    ULONG                       OpenHandleCount;

    //
    // Disk change count
    //

    ULONG                       ChangeCount;
    
    // Pointer to the VPB in the target device object
    PVPB                        Vpb;

    // The FileObject of Volume used to lock the volume
    PFILE_OBJECT                LockFile;

    // List of FCBs for open files on this volume
    LIST_ENTRY                  FcbList;

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
    
    PRFSD_SUPER_BLOCK           SuperBlock;
    PVOID						GroupDesc;		// (NOTE: unused in ReiserFS, but preserved in order to minimize changes to existing code)
//  PVOID                       GroupDescBcb;

    // Number of Group Decsciptions
    ULONG                       NumOfGroups;
/*
    // Bitmap Block per group
    PRTL_BITMAP                 BlockBitMaps;
    PRTL_BITMAP                 InodeBitMaps;
*/
    // Block / Cluster size
    ULONG                       BlockSize;

    // Sector size in bits  (NOTE: unused in ReiserFS)
    //ULONG                       SectorBits;
    
    ULONG                       dwData[RFSD_BLOCK_TYPES];
    ULONG                       dwMeta[RFSD_BLOCK_TYPES];

    // Flags for the volume
    ULONG                       Flags;

    // Streaming File Object
    PFILE_OBJECT                StreamObj;

    // Resource Lock for Mcb
    ERESOURCE                   McbResource;

    // Dirty Mcbs of modifications for volume stream
    LARGE_MCB                   DirtyMcbs;

    // Entry of Mcb Tree (Root Node)
    PRFSD_MCB                   McbTree;
    LIST_ENTRY                  McbList;
    
} RFSD_VCB, *PRFSD_VCB;

//
// Flags for RFSD_VCB
//
#define VCB_INITIALIZED         0x00000001
#define VCB_VOLUME_LOCKED       0x00000002
#define VCB_MOUNTED             0x00000004
#define VCB_DISMOUNT_PENDING    0x00000008
#define VCB_READ_ONLY           0x00000010

#define VCB_WRITE_PROTECTED     0x10000000
#define VCB_FLOPPY_DISK         0x20000000
#define VCB_REMOVAL_PREVENTED   0x40000000
#define VCB_REMOVABLE_MEDIA     0x80000000


#define IsMounted(Vcb)    (IsFlagOn(Vcb->Flags, VCB_MOUNTED))

//
// RFSD_FCB File Control Block
//
// Data that represents an open file
// There is a single instance of the FCB for every open file
//
typedef struct _RFSD_FCB {
    
    // FCB header required by NT
    FSRTL_COMMON_FCB_HEADER         Header;
    SECTION_OBJECT_POINTERS         SectionObject;
    ERESOURCE                       MainResource;
    ERESOURCE                       PagingIoResource;
    // end FCB header required by NT
    
    // Identifier for this structure
    RFSD_IDENTIFIER                 Identifier;
    
    // List of FCBs for this volume
    LIST_ENTRY                      Next;
    
    // Share Access for the file object
    SHARE_ACCESS                    ShareAccess;

    // List of byte-range locks for this file
    FILE_LOCK                       FileLockAnchor;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    ULONG                           OpenHandleCount;
    
    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLOSE
    ULONG                           ReferenceCount;

    // Incremented on IRP_MJ_CREATE, decremented on IRP_MJ_CLEANUP
    // But only for Files with FO_NO_INTERMEDIATE_BUFFERING flag
    ULONG                           NonCachedOpenCount;

    // Flags for the FCB
    ULONG                           Flags;
    
    // Pointer to the inode  / stat data structure
    PRFSD_INODE                     Inode;

    // Hint block for next allocation
    ULONG                           BlkHint;
    
    // Vcb

    PRFSD_VCB                       Vcb;

    // Mcb Node ...
    PRFSD_MCB                       RfsdMcb;

    // Full Path Name
    UNICODE_STRING                  LongName;

#if DBG
    // The Ansi Filename for debugging
    OEM_STRING                      AnsiFileName;   
#endif


} RFSD_FCB, *PRFSD_FCB;


//
// Flags for RFSD_FCB
//
#define FCB_FROM_POOL               0x00000001
#define FCB_PAGE_FILE               0x00000002
#define FCB_DELETE_ON_CLOSE         0x00000004
#define FCB_DELETE_PENDING          0x00000008
#define FCB_FILE_DELETED            0x00000010
#define FCB_FILE_MODIFIED           0x00000020

// Mcb Node

struct _RFSD_MCB {

    // Identifier for this structure
    RFSD_IDENTIFIER                 Identifier;

    // Flags
    ULONG                           Flags;

    // Link List Info

    PRFSD_MCB                       Parent; // Parent
    PRFSD_MCB                       Child;  // Children
    PRFSD_MCB                       Next;   // Brothers

    // Mcb Node Info

    // -> Fcb
    PRFSD_FCB                       RfsdFcb;

    // Short name
    UNICODE_STRING                  ShortName;

    // Inode number (ReiserFS uses 128-bit keys instead of inode numbers)
	RFSD_KEY_IN_MEMORY				Key;

    // Dir entry offset in parent (relative to the start of the directory listing)
    ULONG                           DeOffset;

    // File attribute
    ULONG                           FileAttr;

    // List Link to Vcb->McbList
    LIST_ENTRY                      Link;
};

//
// Flags for MCB
//
#define MCB_FROM_POOL               0x00000001
#define MCB_IN_TREE                 0x00000002
#define MCB_IN_USE                  0x00000004

#define IsMcbUsed(Mcb) IsFlagOn(Mcb->Flags, MCB_IN_USE)

//
// RFSD_CCB Context Control Block
//
// Data that represents one instance of an open file
// There is one instance of the CCB for every instance of an open file
//
typedef struct _RFSD_CCB {
    
    // Identifier for this structure
    RFSD_IDENTIFIER  Identifier;

    // Flags
    ULONG             Flags;
    
    // State that may need to be maintained
    ULONG             CurrentByteOffset;
    USHORT            deh_location;
    UNICODE_STRING    DirectorySearchPattern;
    
} RFSD_CCB, *PRFSD_CCB;

//
// Flags for CCB
//

#define CCB_FROM_POOL               0x00000001

#define CCB_ALLOW_EXTENDED_DASD_IO  0x80000000

//
// RFSD_IRP_CONTEXT
//
// Used to pass information about a request between the drivers functions
//
typedef struct _RFSD_IRP_CONTEXT {
    
    // Identifier for this structure
    RFSD_IDENTIFIER     Identifier;
    
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

    PRFSD_FCB           Fcb;
    PRFSD_CCB           Ccb;
    
    // If the request is synchronous (we are allowed to block)
    BOOLEAN             IsSynchronous;
    
    // If the request is top level
    BOOLEAN             IsTopLevel;
    
    // Used if the request needs to be queued for later processing
    WORK_QUEUE_ITEM     WorkQueueItem;
    
    // If an exception is currently in progress
    BOOLEAN             ExceptionInProgress;
    
    // The exception code when an exception is in progress
    NTSTATUS            ExceptionCode;

    // Repinned BCBs List
    RFSD_REPINNED_BCBS  Repinned;
    
} RFSD_IRP_CONTEXT, *PRFSD_IRP_CONTEXT;


#define IRP_CONTEXT_FLAG_FROM_POOL       (0x00000001)
#define IRP_CONTEXT_FLAG_WAIT            (0x00000002)
#define IRP_CONTEXT_FLAG_WRITE_THROUGH   (0x00000004)
#define IRP_CONTEXT_FLAG_FLOPPY          (0x00000008)
#define IRP_CONTEXT_FLAG_RECURSIVE_CALL  (0x00000010)
#define IRP_CONTEXT_FLAG_DISABLE_POPUPS  (0x00000020)
#define IRP_CONTEXT_FLAG_DEFERRED        (0x00000040)
#define IRP_CONTEXT_FLAG_VERIFY_READ     (0x00000080)
#define IRP_CONTEXT_STACK_IO_CONTEXT     (0x00000100)
#define IRP_CONTEXT_FLAG_REQUEUED        (0x00000200)
#define IRP_CONTEXT_FLAG_USER_IO         (0x00000400)
#define IRP_CONTEXT_FLAG_DELAY_CLOSE     (0x00000800)

//
// RFSD_ALLOC_HEADER
//
// In the checked version of the driver this header is put in the beginning of
// every memory allocation
//
typedef struct _RFSD_ALLOC_HEADER {
    RFSD_IDENTIFIER Identifier;
} RFSD_ALLOC_HEADER, *PRFSD_ALLOC_HEADER;

typedef struct _FCB_LIST_ENTRY {
    PRFSD_FCB    Fcb;
    LIST_ENTRY   Next;
} FCB_LIST_ENTRY, *PFCB_LIST_ENTRY;


// Block Description List
typedef struct _RFSD_BDL {
    ULONGLONG    Lba;
    ULONGLONG    Offset;
    ULONG        Length;
    PIRP         Irp;
} RFSD_BDL, *PRFSD_BDL;

#pragma pack()


/* FUNCTIONS DECLARATION *****************************************************/

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanRfsdWait(IRP) IoIsOperationSynchronous(Irp)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//
// RfsdBlock.c
//

// TODO move allocate and load block here

NTSTATUS
RfsdFindItemHeaderInBlock(
	 IN PRFSD_VCB			Vcb,
	 IN PRFSD_KEY_IN_MEMORY	pKey,						// The key to match against
	 IN PUCHAR				pBlockBuffer,				// A filled disk block, provided by the caller
	 OUT PRFSD_ITEM_HEAD*	ppTargetItemHeader,			// A pointer to a PRFSD_ITEM_HEAD.  The PRFSD_ITEM_HEAD will point to the item head matching Key, or NULL if there was no such item head in the given block.
	 IN	RFSD_KEY_COMPARISON (*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY)
	 );

NTSTATUS
RfsdLoadItem(
	IN	  PRFSD_VCB				Vcb,
	IN   PRFSD_KEY_IN_MEMORY	pItemKey,					// The key of the item to find
	OUT  PRFSD_ITEM_HEAD*		ppMatchingItemHeader,
	OUT  PUCHAR*				ppItemBuffer,				
	OUT  PUCHAR*				ppBlockBuffer,				// Block buffer, which backs the other output data structures.  The caller must free this (even in the case of an error)!
	OUT	 PULONG					pBlockNumber,				// The ordinal disk block number at which the item was found
	IN	RFSD_KEY_COMPARISON (*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY)
	);
//
// Block.c
//

NTSTATUS
RfsdLockUserBuffer (
        IN PIRP             Irp,
        IN ULONG            Length,
        IN LOCK_OPERATION   Operation);
PVOID
RfsdGetUserBuffer (IN PIRP Irp);


NTSTATUS
RfsdReadWriteBlocks(
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB        Vcb,
        IN PRFSD_BDL        RfsdBDL,
        IN ULONG            Length,
        IN ULONG            Count,
        IN BOOLEAN          bVerify );

PUCHAR
RfsdAllocateAndLoadBlock(
	IN	PRFSD_VCB			Vcb,
	IN	ULONG				BlockIndex );

NTSTATUS
RfsdReadSync(
        IN PRFSD_VCB        Vcb,
        IN ULONGLONG        Offset,
        IN ULONG            Length,
        OUT PVOID           Buffer,
        IN BOOLEAN          bVerify );

NTSTATUS
RfsdReadDisk(
         IN PRFSD_VCB       Vcb,
         IN ULONGLONG       Offset,
         IN ULONG           Size,
         IN PVOID           Buffer,
         IN BOOLEAN         bVerify  );

NTSTATUS 
RfsdDiskIoControl (
        IN PDEVICE_OBJECT   DeviceOjbect,
        IN ULONG            IoctlCode,
        IN PVOID            InputBuffer,
        IN ULONG            InputBufferSize,
        IN OUT PVOID        OutputBuffer,
        IN OUT PULONG       OutputBufferSize );

VOID
RfsdMediaEjectControl (
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb,
        IN BOOLEAN bPrevent );

NTSTATUS
RfsdDiskShutDown(PRFSD_VCB Vcb);


//
// Cleanup.c
//
NTSTATUS
RfsdCleanup (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Close.c
//
NTSTATUS
RfsdClose (IN PRFSD_IRP_CONTEXT IrpContext);

VOID
RfsdQueueCloseRequest (IN PRFSD_IRP_CONTEXT IrpContext);

#ifdef _PREFAST_
IO_WORKITEM_ROUTINE RfsdDeQueueCloseRequest;
#endif // _PREFAST_

VOID NTAPI
RfsdDeQueueCloseRequest (IN PVOID Context);

//
// Cmcb.c
//

BOOLEAN NTAPI
RfsdAcquireForLazyWrite (
        IN PVOID    Context,
        IN BOOLEAN  Wait );
VOID NTAPI
RfsdReleaseFromLazyWrite (IN PVOID Context);

BOOLEAN NTAPI
RfsdAcquireForReadAhead (
        IN PVOID    Context,
        IN BOOLEAN  Wait );

BOOLEAN NTAPI
RfsdNoOpAcquire (
        IN PVOID Fcb,
        IN BOOLEAN Wait );

VOID NTAPI
RfsdNoOpRelease (IN PVOID Fcb    );

VOID NTAPI
RfsdReleaseFromReadAhead (IN PVOID Context);

//
// Create.c
//

PRFSD_FCB
RfsdSearchFcbList(
        IN PRFSD_VCB    Vcb,
        IN ULONG        inode);

NTSTATUS
RfsdScanDir (IN PRFSD_VCB       Vcb,
         IN PRFSD_MCB           ParentMcb,				// Mcb of the directory to be scanned
         IN PUNICODE_STRING     FileName,				// Short file name (not necisarilly null-terminated!)
         IN OUT PULONG          Index,					// Offset (in bytes) of the dentry relative to the start of the directory listing
         IN OUT PRFSD_DENTRY_HEAD rfsd_dir);			// Directory entry of the found item

NTSTATUS
RfsdLookupFileName (
        IN PRFSD_VCB            Vcb,
        IN PUNICODE_STRING      FullFileName,
        IN PRFSD_MCB            ParentMcb,
        OUT PRFSD_MCB *         RfsdMcb,
        IN OUT PRFSD_INODE      Inode);

NTSTATUS
RfsdCreateFile(
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb );

NTSTATUS
RfsdCreateVolume(
        IN PRFSD_IRP_CONTEXT IrpContext, 
        IN PRFSD_VCB Vcb );

NTSTATUS
RfsdCreate (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdCreateInode(
        IN PRFSD_IRP_CONTEXT   IrpContext,
        IN PRFSD_VCB           Vcb,
        IN PRFSD_FCB           pParentFcb,
        IN ULONG               Type,
        IN ULONG               FileAttr,
        IN PUNICODE_STRING     FileName);

NTSTATUS
RfsdSupersedeOrOverWriteFile(
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb,
        IN PRFSD_FCB Fcb,
        IN ULONG     Disposition);

//
// Debug.c
//

#define DBG_VITAL 0
#define DBG_ERROR 1
#define DBG_USER  2
#define DBG_TRACE 3
#define DBG_INFO  4
#define DBG_FUNC  5

#if DBG
#define RfsdPrint(arg)          RfsdPrintf   arg
#define RfsdPrintNoIndent(arg)  RfsdNIPrintf arg

#define RfsdCompleteRequest(Irp, bPrint, PriorityBoost) \
        RfsdDbgPrintComplete(Irp, bPrint); \
        IoCompleteRequest(Irp, PriorityBoost)

#else

#define RfsdPrint(arg)
#define RfsdPrintNoIndent(arg)

#define RfsdCompleteRequest(Irp, bPrint, PriorityBoost) \
        IoCompleteRequest(Irp, PriorityBoost)

#endif // DBG

VOID
__cdecl
RfsdPrintf(
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

VOID
__cdecl
RfsdNIPrintf(
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

extern ULONG ProcessNameOffset;

#define RfsdGetCurrentProcessName() ( \
    (PUCHAR) PsGetCurrentProcess() + ProcessNameOffset \
)

ULONG 
RfsdGetProcessNameOffset (VOID);

VOID
RfsdDbgPrintCall (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp );

VOID
RfsdDbgPrintComplete (
        IN PIRP Irp,
        IN BOOLEAN bPrint
        );

PUCHAR
RfsdNtStatusToString (IN NTSTATUS Status );

//
// Devctl.c
//

NTSTATUS
RfsdDeviceControlNormal (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdPrepareToUnload (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdDeviceControl (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Dirctl.c
//

ULONG
RfsdGetInfoLength(IN FILE_INFORMATION_CLASS  FileInformationClass);

ULONG
RfsdProcessDirEntry(
			IN PRFSD_VCB         Vcb,
            IN FILE_INFORMATION_CLASS  FileInformationClass,
            IN __u32		 Key_ParentID,
			IN __u32		 Key_ObjectID,
            IN PVOID         Buffer,
            IN ULONG         UsedLength,
            IN ULONG         Length,
            IN ULONG         FileIndex,
            IN PUNICODE_STRING   pName,
            IN BOOLEAN       Single,
			IN PVOID		 pPreviousEntry	);

NTSTATUS
RfsdQueryDirectory (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdNotifyChangeDirectory (
        IN PRFSD_IRP_CONTEXT IrpContext
        );

VOID
RfsdNotifyReportChange (
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB         Vcb,
        IN PRFSD_FCB         Fcb,
        IN ULONG             Filter,
        IN ULONG             Action
        );

NTSTATUS
RfsdDirectoryControl (IN PRFSD_IRP_CONTEXT IrpContext);

BOOLEAN
RfsdIsDirectoryEmpty (
        IN PRFSD_VCB Vcb,
        IN PRFSD_FCB Fcb
        );

//
// Dispatch.c
//

NTSTATUS
RfsdQueueRequest (IN PRFSD_IRP_CONTEXT IrpContext);

#ifdef _PREFAST_
IO_WORKITEM_ROUTINE RfsdDeQueueRequest;
#endif // _PREFAST_

VOID NTAPI
RfsdDeQueueRequest (IN PVOID Context);

NTSTATUS
RfsdDispatchRequest (IN PRFSD_IRP_CONTEXT IrpContext);

#ifdef _PREFAST_
__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
__drv_dispatchType(IRP_MJ_READ)
__drv_dispatchType(IRP_MJ_WRITE)
__drv_dispatchType(IRP_MJ_FLUSH_BUFFERS)
__drv_dispatchType(IRP_MJ_QUERY_INFORMATION)
__drv_dispatchType(IRP_MJ_SET_INFORMATION)
__drv_dispatchType(IRP_MJ_QUERY_VOLUME_INFORMATION)
__drv_dispatchType(IRP_MJ_SET_VOLUME_INFORMATION)
__drv_dispatchType(IRP_MJ_DIRECTORY_CONTROL)
__drv_dispatchType(IRP_MJ_FILE_SYSTEM_CONTROL)
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
__drv_dispatchType(IRP_MJ_LOCK_CONTROL)
__drv_dispatchType(IRP_MJ_CLEANUP)
__drv_dispatchType(IRP_MJ_PNP)
__drv_dispatchType(IRP_MJ_SHUTDOWN)
DRIVER_DISPATCH RfsdBuildRequest;
#endif // _PREFAST_

NTSTATUS NTAPI
RfsdBuildRequest (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
        );

//
// Except.c
//

NTSTATUS
RfsdExceptionFilter (
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PEXCEPTION_POINTERS ExceptionPointer
        );

NTSTATUS
RfsdExceptionHandler (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Rfsd.c
//

PRFSD_SUPER_BLOCK
RfsdLoadSuper(
        IN PRFSD_VCB Vcb,
        IN BOOLEAN   bVerify
        );

BOOLEAN
RfsdSaveSuper(
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb
        );

BOOLEAN
RfsdLoadGroup(IN PRFSD_VCB Vcb);

BOOLEAN
RfsdSaveGroup(
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb,
        IN ULONG                Group
        );

BOOLEAN
RfsdGetInodeLba (IN PRFSD_VCB   Vcb,
         IN __u32 DirectoryID,
		 IN __u32 ParentID,
         OUT PLONGLONG offset);

BOOLEAN
RfsdLoadInode (IN PRFSD_VCB Vcb,
			   IN PRFSD_KEY_IN_MEMORY pKey,
			   IN OUT PRFSD_INODE Inode);

BOOLEAN
RfsdLoadInode2 (IN PRFSD_VCB Vcb,
			   IN __u32 a,
			   IN __u32 b,
			   IN OUT PRFSD_INODE Inode);
BOOLEAN
RfsdSaveInode (
        IN PRFSD_IRP_CONTEXT IrpContext,
        IN PRFSD_VCB Vcb,
        IN ULONG inode,
        IN PRFSD_INODE Inode
        );

BOOLEAN
RfsdLoadBlock (
        IN PRFSD_VCB Vcb,
        IN ULONG     dwBlk,
        IN PVOID     Buffer
        );

BOOLEAN
RfsdSaveBlock (
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb,
        IN ULONG                dwBlk,
        IN PVOID                Buf
        );

BOOLEAN
RfsdSaveBuffer(
        IN PRFSD_IRP_CONTEXT    IrpContext,
        IN PRFSD_VCB            Vcb,
        IN LONGLONG             Offset,
        IN ULONG                Size,
        IN PVOID                Buf
        );

NTSTATUS
RfsdGetBlock(
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                dwContent,
    IN ULONG                Index,
    IN ULONG                Layer,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock
    );

NTSTATUS
RfsdBlockMap(
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONG                Index,
    IN BOOLEAN              bAlloc,
    OUT PULONG              pBlock
    );

NTSTATUS
RfsdBuildBDL2(	
	IN  PRFSD_VCB				Vcb,
	IN  PRFSD_KEY_IN_MEMORY		pKey,
	IN	PRFSD_INODE				pInode,
	OUT	PULONG					out_Count,
	OUT PRFSD_BDL*				out_ppBdl  );

NTSTATUS
RfsdBuildBDL( 
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN PRFSD_KEY_IN_MEMORY  InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONGLONG            Offset, 
    IN ULONG                Size, 
    IN BOOLEAN              bAlloc,
    OUT PRFSD_BDL *         Bdls,
    OUT PULONG              Count
    );

NTSTATUS
RfsdNewBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    ULONG     GroupHint,
    ULONG     BlockHint,  
    PULONG    dwRet );

NTSTATUS
RfsdFreeBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    ULONG     Block );

NTSTATUS
RfsdExpandBlock(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB   Fcb,
    ULONG dwContent,
    ULONG Index,
    ULONG layer,
    BOOLEAN bNew,
    ULONG *dwRet );


NTSTATUS
RfsdExpandInode(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB Fcb,
    ULONG *dwRet );

NTSTATUS
RfsdNewInode(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            ULONG GroupHint,
            ULONG mode,
            PULONG Inode );

BOOLEAN
RfsdFreeInode(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            ULONG Inode,
            ULONG Type );

NTSTATUS
RfsdAddEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               FileType,
         IN ULONG               Inode,
         IN PUNICODE_STRING     FileName );

NTSTATUS
RfsdRemoveEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               FileType,
         IN ULONG               Inode );

NTSTATUS
RfsdSetParentEntry (
         IN PRFSD_IRP_CONTEXT   IrpContext,
         IN PRFSD_VCB           Vcb,
         IN PRFSD_FCB           Dcb,
         IN ULONG               OldParent,
         IN ULONG               NewParent );


NTSTATUS
RfsdTruncateBlock(
         IN PRFSD_IRP_CONTEXT IrpContext,
         IN PRFSD_VCB Vcb,
         IN PRFSD_FCB Fcb,
         IN ULONG   dwContent,
         IN ULONG   Index,
         IN ULONG   layer,
         OUT BOOLEAN *bFreed );

NTSTATUS
RfsdTruncateInode(
         IN PRFSD_IRP_CONTEXT IrpContext,
         IN PRFSD_VCB   Vcb,
         IN PRFSD_FCB   Fcb );

BOOLEAN
RfsdAddMcbEntry (
    IN PRFSD_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length );

VOID
RfsdRemoveMcbEntry (
    IN PRFSD_VCB Vcb,
    IN LONGLONG  Lba,
    IN LONGLONG  Length );

BOOLEAN
RfsdLookupMcbEntry (
    IN PRFSD_VCB    Vcb,
    IN LONGLONG     Offset,
    OUT PLONGLONG   Lba OPTIONAL,
    OUT PLONGLONG   Length OPTIONAL,
    OUT PLONGLONG   RunStart OPTIONAL,
    OUT PLONGLONG   RunLength OPTIONAL,
    OUT PULONG      Index OPTIONAL );

BOOLEAN
SuperblockContainsMagicKey(PRFSD_SUPER_BLOCK sb);

__u32
ConvertKeyTypeUniqueness(__u32 k_uniqueness);

void
FillInMemoryKey(
	IN		PRFSD_KEY_ON_DISK		pKeyOnDisk, 
	IN		RFSD_KEY_VERSION		KeyVersion, 
	IN OUT	PRFSD_KEY_IN_MEMORY		pKeyInMemory );

RFSD_KEY_VERSION DetermineOnDiskKeyFormat(const PRFSD_KEY_ON_DISK key);

RFSD_KEY_COMPARISON
CompareShortKeys(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b		);

RFSD_KEY_COMPARISON
CompareKeysWithoutOffset(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b		);

RFSD_KEY_COMPARISON
CompareKeys(
	IN		PRFSD_KEY_IN_MEMORY		a,
	IN		PRFSD_KEY_IN_MEMORY		b	);

NTSTATUS
NavigateToLeafNode(
	IN	PRFSD_VCB					Vcb,
	IN	PRFSD_KEY_IN_MEMORY			Key,				
	IN	ULONG						StartingBlockNumber,	
	OUT	PULONG						out_NextBlockNumber );

NTSTATUS
RfsdParseFilesystemTree(
			IN	PRFSD_VCB					Vcb,
			IN	PRFSD_KEY_IN_MEMORY			Key,						// Key to search for.
			IN	ULONG						StartingBlockNumber,		// Block number of an internal or leaf node, to start the search from			
			IN	RFSD_CALLBACK(fpDirectoryCallback),					// A function ptr to trigger on hitting a matching leaf block
			IN  PVOID						Context
			);


NTSTATUS
_NavigateToLeafNode(
	IN	PRFSD_VCB					Vcb,
	IN	PRFSD_KEY_IN_MEMORY			Key,				
	IN	ULONG						StartingBlockNumber,	
	OUT	PULONG						out_NextBlockNumber,
	IN	BOOLEAN						ReturnOnFirstMatch,
	IN	RFSD_KEY_COMPARISON			(*fpComparisonFunction)(PRFSD_KEY_IN_MEMORY, PRFSD_KEY_IN_MEMORY),
	RFSD_CALLBACK(fpDirectoryCallback),
	IN	PVOID						pContext					
	);


//
// Fastio.c
//

#ifdef _PREFAST_
FAST_IO_CHECK_IF_POSSIBLE RfsdFastIoCheckIfPossible;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoCheckIfPossible (
              IN PFILE_OBJECT         FileObject,
              IN PLARGE_INTEGER       FileOffset,
              IN ULONG                Length,
              IN BOOLEAN              Wait,
              IN ULONG                LockKey,
              IN BOOLEAN              CheckForReadOperation,
              OUT PIO_STATUS_BLOCK    IoStatus,
              IN PDEVICE_OBJECT       DeviceObject );

#ifdef _PREFAST_
FAST_IO_READ RfsdFastIoRead;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoRead (IN PFILE_OBJECT FileObject,
        IN PLARGE_INTEGER       FileOffset,
        IN ULONG                Length,
        IN BOOLEAN              Wait,
        IN ULONG                LockKey,
        OUT PVOID               Buffer,
        OUT PIO_STATUS_BLOCK    IoStatus,
        IN PDEVICE_OBJECT       DeviceObject);

#ifdef _PREFAST_
FAST_IO_WRITE RfsdFastIoWrite;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoWrite (
        IN PFILE_OBJECT         FileObject,
        IN PLARGE_INTEGER       FileOffset,
        IN ULONG                Length,
        IN BOOLEAN              Wait,
        IN ULONG                LockKey,
        OUT PVOID               Buffer,
        OUT PIO_STATUS_BLOCK    IoStatus,
        IN PDEVICE_OBJECT       DeviceObject);

#ifdef _PREFAST_
FAST_IO_QUERY_BASIC_INFO RfsdFastIoQueryBasicInfo;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoQueryBasicInfo (
              IN PFILE_OBJECT             FileObject,
              IN BOOLEAN                  Wait,
              OUT PFILE_BASIC_INFORMATION Buffer,
              OUT PIO_STATUS_BLOCK        IoStatus,
              IN PDEVICE_OBJECT           DeviceObject);

#ifdef _PREFAST_
FAST_IO_QUERY_STANDARD_INFO RfsdFastIoQueryStandardInfo;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoQueryStandardInfo (
                 IN PFILE_OBJECT                 FileObject,
                 IN BOOLEAN                      Wait,
                 OUT PFILE_STANDARD_INFORMATION  Buffer,
                 OUT PIO_STATUS_BLOCK            IoStatus,
                 IN PDEVICE_OBJECT               DeviceObject);

#ifdef _PREFAST_
FAST_IO_LOCK RfsdFastIoLock;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoLock (
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

#ifdef _PREFAST_
FAST_IO_UNLOCK_SINGLE RfsdFastIoUnlockSingle;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoUnlockSingle (
               IN PFILE_OBJECT         FileObject,
               IN PLARGE_INTEGER       FileOffset,
               IN PLARGE_INTEGER       Length,
               IN PEPROCESS            Process,
               IN ULONG                Key,
               OUT PIO_STATUS_BLOCK    IoStatus,
               IN PDEVICE_OBJECT       DeviceObject
               );

#ifdef _PREFAST_
FAST_IO_UNLOCK_ALL RfsdFastIoUnlockAll;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoUnlockAll (
            IN PFILE_OBJECT         FileObject,
            IN PEPROCESS            Process,
            OUT PIO_STATUS_BLOCK    IoStatus,
            IN PDEVICE_OBJECT       DeviceObject
            );

#ifdef _PREFAST_
FAST_IO_UNLOCK_ALL_BY_KEY RfsdFastIoUnlockAllByKey;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoUnlockAllByKey (
             IN PFILE_OBJECT         FileObject,
#ifdef __REACTOS__
             IN PVOID                Process,
#else
             IN PEPROCESS            Process,
#endif
             IN ULONG                Key,
             OUT PIO_STATUS_BLOCK    IoStatus,
             IN PDEVICE_OBJECT       DeviceObject
             );

#ifdef _PREFAST_
FAST_IO_QUERY_NETWORK_OPEN_INFO RfsdFastIoQueryNetworkOpenInfo;
#endif // _PREFAST_

BOOLEAN NTAPI
RfsdFastIoQueryNetworkOpenInfo (
     IN PFILE_OBJECT                     FileObject,
     IN BOOLEAN                          Wait,
     OUT PFILE_NETWORK_OPEN_INFORMATION  Buffer,
     OUT PIO_STATUS_BLOCK                IoStatus,
     IN PDEVICE_OBJECT                   DeviceObject );


//
// FileInfo.c
//


NTSTATUS
RfsdQueryInformation (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdSetInformation (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdExpandFile (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB Fcb,
    PLARGE_INTEGER AllocationSize );

NTSTATUS
RfsdTruncateFile (
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb,
    PRFSD_FCB Fcb,
    PLARGE_INTEGER AllocationSize );

NTSTATUS
RfsdSetDispositionInfo(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            PRFSD_FCB Fcb,
            BOOLEAN bDelete);

NTSTATUS
RfsdSetRenameInfo(
            PRFSD_IRP_CONTEXT IrpContext,
            PRFSD_VCB Vcb,
            PRFSD_FCB Fcb );

NTSTATUS
RfsdDeleteFile(
        PRFSD_IRP_CONTEXT IrpContext,
        PRFSD_VCB Vcb,
        PRFSD_FCB Fcb );


//
// Flush.c
//

NTSTATUS
RfsdFlushFiles (IN PRFSD_VCB Vcb, BOOLEAN bShutDown);

NTSTATUS
RfsdFlushVolume (IN PRFSD_VCB Vcb, BOOLEAN bShutDown);

NTSTATUS
RfsdFlushFile (IN PRFSD_FCB Fcb);

NTSTATUS
RfsdFlush (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Fsctl.c
//


VOID
RfsdSetVpbFlag (IN PVPB     Vpb,
        IN USHORT   Flag );

VOID
RfsdClearVpbFlag (IN PVPB     Vpb,
          IN USHORT   Flag );

BOOLEAN
RfsdCheckDismount (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB         Vcb,
    IN BOOLEAN           bForce   );

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPurgeVolume (IN PRFSD_VCB Vcb,
         IN BOOLEAN  FlushBeforePurge);

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPurgeFile (IN PRFSD_FCB Fcb,
           IN BOOLEAN  FlushBeforePurge);

BOOLEAN
RfsdIsHandleCountZero(IN PRFSD_VCB Vcb);

NTSTATUS
RfsdLockVcb (IN PRFSD_VCB    Vcb,
             IN PFILE_OBJECT FileObject);

NTSTATUS
RfsdLockVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdUnlockVcb (IN PRFSD_VCB    Vcb,
               IN PFILE_OBJECT FileObject);

NTSTATUS
RfsdUnlockVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdAllowExtendedDasdIo(IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdUserFsRequest (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdMountVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdVerifyVolume (IN PRFSD_IRP_CONTEXT IrpContext);

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdIsVolumeMounted (IN PRFSD_IRP_CONTEXT IrpContext);

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdDismountVolume (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdFileSystemControl (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Init.c
//

BOOLEAN
RfsdQueryParameters (IN PUNICODE_STRING  RegistryPath);

#ifdef _PREFAST_
DRIVER_INITIALIZE DriverEntry;
#endif // _PREFAST_

#ifdef _PREFAST_
DRIVER_UNLOAD DriverUnload;
#endif // _PREFAST_

VOID NTAPI
DriverUnload (IN PDRIVER_OBJECT DriverObject);


//
// Lock.c
//

NTSTATUS
RfsdLockControl (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Memory.c
//

PRFSD_IRP_CONTEXT
RfsdAllocateIrpContext (IN PDEVICE_OBJECT   DeviceObject,
            IN PIRP             Irp );

VOID
RfsdFreeIrpContext (IN PRFSD_IRP_CONTEXT IrpContext);


PRFSD_FCB
RfsdAllocateFcb (IN PRFSD_VCB   Vcb,
         IN PRFSD_MCB           RfsdMcb,
         IN PRFSD_INODE         Inode );

VOID
RfsdFreeFcb (IN PRFSD_FCB Fcb);

PRFSD_CCB
RfsdAllocateCcb (VOID);

VOID
RfsdFreeMcb (IN PRFSD_MCB Mcb);

PRFSD_FCB
RfsdCreateFcbFromMcb(PRFSD_VCB Vcb, PRFSD_MCB Mcb);

VOID
RfsdFreeCcb (IN PRFSD_CCB Ccb);

PRFSD_MCB
RfsdAllocateMcb ( PRFSD_VCB,
                  PUNICODE_STRING FileName,
                  ULONG FileAttr);

PRFSD_MCB
RfsdSearchMcbTree(  PRFSD_VCB Vcb,
                    PRFSD_MCB RfsdMcb,
                    PRFSD_KEY_IN_MEMORY Key);

PRFSD_MCB
RfsdSearchMcb(  PRFSD_VCB Vcb, PRFSD_MCB Parent,
                PUNICODE_STRING FileName);

BOOLEAN
RfsdGetFullFileName( PRFSD_MCB Mcb, 
                     PUNICODE_STRING FileName);

VOID
RfsdRefreshMcb(PRFSD_VCB Vcb, PRFSD_MCB Mcb);

VOID
RfsdAddMcbNode( PRFSD_VCB Vcb,
                PRFSD_MCB Parent,
                PRFSD_MCB Child );

BOOLEAN
RfsdDeleteMcbNode(
                PRFSD_VCB Vcb, 
                PRFSD_MCB McbTree,
                PRFSD_MCB RfsdMcb);

VOID
RfsdFreeMcbTree(PRFSD_MCB McbTree);

BOOLEAN
RfsdCheckSetBlock( PRFSD_IRP_CONTEXT IrpContext,
                   PRFSD_VCB Vcb, ULONG Block);

BOOLEAN
RfsdCheckBitmapConsistency( PRFSD_IRP_CONTEXT IrpContext,
                            PRFSD_VCB Vcb);

VOID
RfsdInsertVcb(PRFSD_VCB Vcb);

VOID
RfsdRemoveVcb(PRFSD_VCB Vcb);

NTSTATUS
RfsdInitializeVcb(
            PRFSD_IRP_CONTEXT IrpContext, 
            PRFSD_VCB Vcb, 
            PRFSD_SUPER_BLOCK RfsdSb,
            PDEVICE_OBJECT TargetDevice,
            PDEVICE_OBJECT VolumeDevice,
            PVPB Vpb                   );

VOID
RfsdFreeVcb (IN PRFSD_VCB Vcb );


VOID
RfsdRepinBcb (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PBCB Bcb );

VOID
RfsdUnpinRepinnedBcbs (
    IN PRFSD_IRP_CONTEXT IrpContext);


NTSTATUS
RfsdCompleteIrpContext (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN NTSTATUS Status );

VOID
RfsdSyncUninitializeCacheMap (
    IN PFILE_OBJECT FileObject    );

//
// Misc.c
//

/** Returns the length of a string (not including a terminating null), or MaximumLength if no terminator is found within MaximumLength characters. */
static inline USHORT RfsdStringLength(PUCHAR buffer, USHORT MaximumLength)
{
	USHORT i = 0;
	while ((i < MaximumLength) && (buffer[i] != '\0'))  { i++; }
	return i;
}

ULONG
RfsdLog2(ULONG Value);

LARGE_INTEGER
RfsdSysTime (IN ULONG i_time);

ULONG
RfsdInodeTime (IN LARGE_INTEGER SysTime);

ULONG
RfsdOEMToUnicodeSize(
        IN PANSI_STRING Oem
        );

NTSTATUS
RfsdOEMToUnicode(
        IN OUT PUNICODE_STRING Oem,
        IN POEM_STRING         Unicode
        );

ULONG
RfsdUnicodeToOEMSize(
        IN PUNICODE_STRING Unicode
        );

NTSTATUS
RfsdUnicodeToOEM (
        IN OUT POEM_STRING Oem,
        IN PUNICODE_STRING Unicode
        );

//
// nls/nls_rtl.c
//

int
RfsdLoadAllNls();

VOID
RfsdUnloadAllNls();

//
// Pnp.c
//

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnp(IN PRFSD_IRP_CONTEXT IrpContext);

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpQueryRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb      );

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB         Vcb      );

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpCancelRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb              );

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdPnpSurpriseRemove(
    PRFSD_IRP_CONTEXT IrpContext,
    PRFSD_VCB Vcb              );


//
// Read.c
//

BOOLEAN 
RfsdCopyRead(
    IN PFILE_OBJECT  FileObject,
    IN PLARGE_INTEGER  FileOffset,
    IN ULONG  Length,
    IN BOOLEAN  Wait,
    OUT PVOID  Buffer,
    OUT PIO_STATUS_BLOCK  IoStatus   );


NTSTATUS
RfsdReadInode (
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN PRFSD_KEY_IN_MEMORY  Key,
    IN PRFSD_INODE          Inode,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    OUT PULONG              dwReturn
    );

NTSTATUS
RfsdRead (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Shutdown.c
//

NTSTATUS
RfsdShutDown (IN PRFSD_IRP_CONTEXT IrpContext);

//
// Volinfo.c
//

NTSTATUS
RfsdQueryVolumeInformation (IN PRFSD_IRP_CONTEXT IrpContext);

NTSTATUS
RfsdSetVolumeInformation (IN PRFSD_IRP_CONTEXT IrpContext);


//
// Write.c
//

NTSTATUS
RfsdWriteInode (
    IN PRFSD_IRP_CONTEXT    IrpContext,
    IN PRFSD_VCB            Vcb,
    IN ULONG                InodeNo,
    IN PRFSD_INODE          Inode,
    IN ULONGLONG            Offset,
    IN PVOID                Buffer,
    IN ULONG                Size,
    IN BOOLEAN              bWriteToDisk,
    OUT PULONG              dwReturn
    );

VOID
RfsdStartFloppyFlushDpc (
    PRFSD_VCB   Vcb,
    PRFSD_FCB   Fcb,
    PFILE_OBJECT FileObject );

BOOLEAN
RfsdZeroHoles (
    IN PRFSD_IRP_CONTEXT IrpContext,
    IN PRFSD_VCB Vcb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG Offset,
    IN LONGLONG Count );

NTSTATUS
RfsdWrite (IN PRFSD_IRP_CONTEXT IrpContext);

#endif /* _RFSD_HEADER_ */
