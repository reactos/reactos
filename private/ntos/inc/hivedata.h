/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    hivedata.h

Abstract:

    This module contains data structures used by the 
    direct memory loaded hive manager.

Author:

    Dragos C. Sambotin (dragoss) 13-Jan-99

Revision History:



--*/

#ifndef __HIVE_DATA__
#define __HIVE_DATA__

//
// ===== Arbitrary Limits Imposed For Sanity =====
//
#define HSANE_CELL_MAX      (1024*1024)     // 1 megabyte max size for
                                            // a single cell


//
// ===== Tuning =====

#define HBIN_THRESHOLD      (HBLOCK_SIZE-512)   // If less than threshold
                                                // bytes would be left in
                                                // bin, add another page

#define HLOG_GROW           HBLOCK_SIZE         // Minimum size to grow log
                                                // by.  Can set this up
                                                // if we think it thrashes.

#define HCELL_BIG_ROUND     (HBLOCK_SIZE*3)     //
                                                // If someone tries to
                                                // allocate a very large
                                                // cell, round it up to
                                                // HBLOCK_SIZE.  This is
                                                // the rather arbitrary
                                                // define for "very large"
                                                //
//
// Never shrink the log files smaller than this, this prevents people
// from sucking up all the disk space and then being unable to do
// critical registry operations (like logging on to delete some files)
//
#define HLOG_MINSIZE(Hive)  \
    ((Hive)->Cluster * HSECTOR_SIZE * 2)

//
// ===== Basic Structures and Definitions =====
//
// These are same whether on disk or in memory.
//

//
// NOTE:    Volatile == storage goes away at reboot
//          Stable == Persistent == Not Volatile
//
typedef enum {
    Stable = 0,
    Volatile = 1
} HSTORAGE_TYPE;

#define HTYPE_COUNT 2

//
// --- HCELL_INDEX ---
//
//
// Handle to a cell -> effectively the "virtual" address of the cell,
// HvMapCell converts this to a "real" address, that is, a memory
// address.  Mapping scheme is very much like that standard two level
// page table.  No mappings stored in file, they are built up when
// the file is read in.  (The INDEX in HCELL_INDEX is historical)
//
//  Bit     31  30-21   20-12   11-0
//        +----------------------------+
//        | T | Table | Block | Offset |
//        +----------------------------+
//
//  T = Type(1)= 0 for stable ("normal") storage
//               1 for volatile storage
//
//      Table(10) = Index into directory of mapping tables, selects a table.
//                  Each mapping table is an array of HMAP_ENTRY structures.
//
//      Block(9) = Index into Table, selects an HMAP_ENTRY.  HMAP_ENTRY
//                 contains address of area in memory that this HCELL_INDEX
//                 maps to.  (Base of memory copy of Block)
//
//      Offset(12) = Offset within page, of the Cell header for the cell
//                   of interest.
//
typedef ULONG HCELL_INDEX;
typedef HCELL_INDEX *PHCELL_INDEX;

#define HCELL_NIL   ((HCELL_INDEX)(-1))

#define HCELL_TYPE_MASK         0x80000000
#define HCELL_TYPE_SHIFT        31

#define HCELL_TABLE_MASK        0x7fe00000
#define HCELL_TABLE_SHIFT       21

#define HCELL_BLOCK_MASK        0x001ff000
#define HCELL_BLOCK_SHIFT       12

#define HCELL_OFFSET_MASK       0x00000fff

#define HBLOCK_SIZE             0x1000      // LOGICAL block size
                                            // This is the size of one of
                                            // the registry's logical/virtual
                                            // pages.  It has no particular
                                            // relationship to page size
                                            // of the machine.

#define HSECTOR_SIZE            0x200       // LOGICAL sector size
#define HSECTOR_COUNT           8           // LOGICAL sectors / LOGICAL Block

#define HTABLE_SLOTS        512         // 9 bits of address
#define HDIRECTORY_SLOTS    1024        // 10 bits of address

#define HvGetCellType(Cell) ((ULONG)((Cell & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT))

//
// --- HCELL --- an object within the hive  (A bin is filled with HCELLs)
//
// Any given item of user data must fit within a single HCELL.
// HCELLs cannot span Bins.
//
#define HCELL_PAD(Hive)         ((Hive->Version>=2) ? 8 : 16)
                                // All cells must be at least this large,
                                // All allocations on this boundary

#define HCELL_ALLOCATE_FILL 0xb2    // bz -> buzz buzz (yeah, it's a stretch)
                                    // must fill all newly allocated
                                    // cells for security reasons

#define HCELL_FREE_FILL     0xfc    // fc = HvFreeCell...

//
// Currently we support two cell formats, one with a Last backpointer (old version),
// and one without (new version)
//
// All cells in a hive must be of the same type.  Version 1 hives use the old version,
// Version 2 or greater use the new version.
//

#define USE_OLD_CELL(Hive) (Hive->Version==1)

typedef struct _HCELL {
    LONG    Size;
    union {
        struct {
            ULONG Last;
            union {
                ULONG UserData;
                HCELL_INDEX Next;   // offset of next element in freelist (not a FLink)
            } u;
        } OldCell;

        struct {
            union {
                ULONG UserData;
                HCELL_INDEX Next;    // offset of next element in freelist (not a FLink)
            } u;
        } NewCell;
    } u;
} HCELL, *PHCELL;


//
// --- HBIN ---  is a contiguous set of HBLOCKs, filled with HCELLs.
//
#define HBIN_SIGNATURE          0x6e696268      // "hbin"
#define HBIN_NIL                (-1)

#pragma pack(4)
typedef struct  _HBIN {
    ULONG       Signature;
    ULONG       FileOffset;     // Own file offset (used in checking)
    ULONG       Size;           // Size of bin in bytes, all inclusive
    ULONG       Reserved1[2];   // Old FreeSpace and FreeList (from 1.0)
    LARGE_INTEGER   TimeStamp;  // Old Link (from 1.0).  Usually trash, but
                                // first bin has valid value used for .log
                                // correspondence testing, only meaningful
                                // on disk.
    ULONG       MemAlloc;       // if non 0, this is first bin of allocation
                                // and this is it's size.  else, middle
                                // bin of an allocation.  MEMORY ONLY,
                                // field on disk has no meaning.

    //
    // Cell data goes here
    //

} HBIN, *PHBIN;
#pragma pack()

//
// ===== On Disk Structures =====
//

//
// NOTE:    Hive storage is always allocated in units of 4K.  This size
//          must be used on all systems, regardless of page size, since
//          the file format needs to be transportable amoung systems.
//
// NOTE:    The integrity code depends on certain blocks (e.g., the
//          BASE block) being at least as large as the size of a physical
//          sector.  (Otherwise data that should be left alone will
//          be written because the FS has to block/deblock.)  This means
//          that the current code will not work with sectors > 4K.
//
// NOTE:    A hive on disk always contains at least two blocks of storage.
//          1 block for the base block, and 1 for the minimum 1 bin.
//
// NOTE:    Only modified parts of the hive get written to disk.
//          This is not just for efficiency, but also to avoid risk
//          of destruction of unlogged data.  Dirty bits keep track
//          of what has been modified, they reside in a simple
//          bit map attached to the hive.  One bit for each logical
//          sector of 512 bytes.
//
//          If the physical sector size of the machine is less than 512,
//          no matter, we'll always write in clumps of 512.  If the
//          physical sector size is greater than 512, we'll always clump
//          data together so that we log and write data
//          in chunks of that size.  Physical sector sizes > 4K will
//          not work correctly (logging will not work right, so system
//          crashes may lose data that would not otherwise be lost.)
//


//
// An on disk image of a hive looks like this:
//
//      +---------------------------------------+
//      | HBASE_BLOCK                           | 1 Hive Block == 4K
//      |                                       |
//      +---------------------------------------+ <- HBLOCK_SIZE boundary
//      | Bin - 1 to N 4K blocks                |
//      | Each contains a signature, size, and  |
//      | a boundary tag heap internal to       |
//      | itself.  Once allocated lives forever |
//      | and always at same file offset.       |
//      +---------------------------------------+ <- HBLOCK_SIZE boundary
//      | Bin ...                               |
//      +---------------------------------------+ <- HBLOCK_SIZE boundary
//              ...
//      +---------------------------------------+ <- HBLOCK_SIZE boundary
//      | Last allocated Bin, new bins are put  |
//      | immediately after this one.           |
//      +---------------------------------------+ <- HBLOCK_SIZE boundary
//
//  Hive files must allocate on HBLOCK_SIZE boundaries because they
//  might be written on many different systems, and must therefore be
//  set up for the largest cluster size we will support.
//

//
//  The log file format is:
//
//          +-------------------------------+
//          | HBASE_BLOCK copy              |
//          +-------------------------------+ <- cluster (usually 512) bound
//          | DirtyVector                   |
//          | (length computed from length  |
//          |  in the base block            |
//          | (with "DIRT" on front as a    |
//          |  signature)                   |
//          +-------------------------------+ <- cluster (usually 512) bound
//          | Dirty Data                    |
//          +-------------------------------+ <- cluster (usually 512) bound
//          | Dirty Data                    |
//          +-------------------------------+ <- cluster (usually 512) bound
//          | ...                           |
//          +-------------------------------+
//
//  Recovery consists of reading the file in, computing which clusters
//  of data are present from the dirtyvector, and where they belong in
//  the hive address space.  Position in file is by sequential count.
//
//  Logs can allocate on cluster boundaries (physical sector size of
//  host machine) because they will never be written on any machine other
//  than the one that created them.
//
//  For log to be valid:
//
//      Signature, format, major.minor must match expected values.
//      Sequence1 and Sequence2 must match.
//      CheckSum must be correct.
//      Signture on DirtyVector must be correct
//
//  For log to be applicable:
//
//      Sequence in log must match sequence in hive.
//      TimeStamp in log must match TimeStamp in hive.
//      Hive must be in mid-update state, or have bogus header.
//

//
// --- HBASE_BLOCK --- on disk description of the hive
//

//
// NOTE:    HBASE_BLOCK must be >= the size of physical sector,
//          or integrity assumptions will be violated, and crash
//          recovery may not work.
//
#define HBASE_BLOCK_SIGNATURE   0x66676572  // "regf"

#define HSYS_MAJOR          1               // Must match to read at all
#define HSYS_MINOR          3               // Must be <= to write, always
                                            // set up to writer's version.

#define HBASE_FORMAT_MEMORY 1               // Direct memory load case

#define HBASE_NAME_ALLOC    64              // 32 unicode chars

#pragma pack(4)
typedef struct _HBASE_BLOCK {
    ULONG           Signature;
    ULONG           Sequence1;
    ULONG           Sequence2;
    LARGE_INTEGER   TimeStamp;
    ULONG           Major;
    ULONG           Minor;
    ULONG           Type;                   // HFILE_TYPE_[PRIMARY|LOG]
    ULONG           Format;
    HCELL_INDEX     RootCell;
    ULONG           Length;                 // Includes all but header
    ULONG           Cluster;                // for logs only
    UCHAR           FileName[HBASE_NAME_ALLOC];  // filename tail
    ULONG           Reserved1[99];
    ULONG           CheckSum;
    ULONG           Reserved2[128*7];
} HBASE_BLOCK, *PHBASE_BLOCK;
#pragma pack()

#define HLOG_HEADER_SIZE  (FIELD_OFFSET(HBASE_BLOCK, Reserved2))
#define HLOG_DV_SIGNATURE   0x54524944      // "DIRT"

//
// ===== In Memory Structures =====
//

//
// In memory image of a Hive looks just like the on-disk image,
// EXCEPT that the HBIN structures can be spread throughout memory
// rather than packed together.
//
// To find an HCELL in memory, a mechanism that takes an HCELL_INDEX and
// derives a memory address from it is used.  That mechanism is very
// similar to a two level hardware paging table.
//
// A bit map is used to remember which parts of the hive are dirty.
//
// An HBLOCK can be in three different states
//  1. Present in memory.  BlockAddress and BinAddress are valid pointers.
//     This is the normal state of an HBLOCK.
//
//  2. Discardable.  The HBIN containing this HBLOCK is completely free, but
//     the bin is dirty and needs to be written to the hive file before it
//     can be free.  This is the state we will be in if somebody frees a
//     cell, causing the entire HBIN to become free.  HvpEnlistFreeCell will
//     transition all the HBLOCKs in the free HBIN to this state, but will
//     not free their memory.  After the dirty HBLOCKs are flushed to the
//     file, the memory will be freed.
//
//     Note that if we need to allocate more storage from an HBIN in this
//     state, HvAllocateCell will simply change its state back to State 1
//     and it will be usable.
//
//     An HBLOCK in this state has a valid BlockAddress and BinAddress, but
//     the HMAP_DISCARDABLE bit will be set.
//
//  3. Discarded.  The HBIN containing this HBLOCK is completely free, and
//     is not dirty (i.e. it is marked as free in the hive file as well).
//     There is no memory allocated to contain this HBIN.  After HvSyncHive
//     writes out an HBIN that is in State 2, it frees its pool and the
//     HBIN moves into this state.
//
//     In order to use this HBIN, memory must be allocated to back it, and
//     the HBIN and initial HCELL must be recreated.  (we could re-read it
//     from the hive file, but there's not much point in that since we know
//     that it is entirely free, so we might as well just recreate it and
//     save the disk i/o)
//
//     An HBLOCK in this state has a NULL BlockAddress in the map.
//     The BinAddress will contain the next HCELL in the free list, so
//     we can reconstruct this when we need it.
//     The HMAP_NEWALLOC bit will be set for the first HBLOCK in the HBIN.
//

//
// --- HMAP_ENTRY --- Holds memory location of HCELL
//
#define HMAP_FLAGS      (3)
#define HMAP_BASE       (~3)
#define HMAP_NEWALLOC   1
#define HMAP_DISCARDABLE 2

typedef struct _HMAP_ENTRY {
    ULONG_PTR    BlockAddress;       // Low 2 bits always 0.  High bits
                                    // are memory address of HBLOCK that
                                    // HCELL starts in, add Offset to this.
                                    // (An HCELL can span several HBLOCKs)
                                    //

    ULONG_PTR    BinAddress;         // Low bit set TRUE to mark beginning
                                    // of a new allocation.
                                    // High bits are memory address of
                                    // first HBLOCK in same bin.
                                    // (A given HCELL is always contained
                                    //  in a single bin.)
} HMAP_ENTRY, *PHMAP_ENTRY;


//
// --- HMAP_TABLE --- Array of MAP_ENTRYs that point to memory HBLOCKs
//
// Each HBLOCK worth of space in the Hive image has an entry in
// an HMAP_TABLE.
//
typedef struct _HMAP_TABLE {
    HMAP_ENTRY  Table[ HTABLE_SLOTS ];
} HMAP_TABLE, *PHMAP_TABLE;


//
// --- HMAP_DIRECTORY --- Array of pointers to HMAP_TABLEs
//
typedef struct _HMAP_DIRECTORY {
    PHMAP_TABLE Directory[  HDIRECTORY_SLOTS ];
} HMAP_DIRECTORY, *PHMAP_DIRECTORY;


//
// ===== Hive Routines typedefs =====
//
struct _HHIVE; // forward

typedef
PVOID
(*PALLOCATE_ROUTINE) (
    ULONG       Length,             // Size of new block wanted
    BOOLEAN     UseForIo            // TRUE if yes, FALSE if no
    );

typedef
VOID
(*PFREE_ROUTINE) (
    PVOID       MemoryBlock,
    ULONG       GlobalQuotaSize
    );

typedef
BOOLEAN
(*PFILE_SET_SIZE_ROUTINE) (
    struct _HHIVE  *Hive,
    ULONG          FileType,
    ULONG          FileSize
    );

typedef struct {
    ULONG  FileOffset;
    PVOID  DataBuffer;
    ULONG  DataLength;
} CMP_OFFSET_ARRAY, * PCMP_OFFSET_ARRAY;

typedef
BOOLEAN
(*PFILE_WRITE_ROUTINE) (
    struct _HHIVE  *Hive,
    ULONG       FileType,
    PCMP_OFFSET_ARRAY offsetArray,
    ULONG offsetArrayCount,
    PULONG FileOffset
    );

typedef
BOOLEAN
(*PFILE_READ_ROUTINE) (
    struct _HHIVE  *Hive,
    ULONG       FileType,
    PULONG      FileOffset,
    PVOID       DataBuffer,
    ULONG       DataLength
    );

typedef
BOOLEAN
(*PFILE_FLUSH_ROUTINE) (
    struct _HHIVE  *Hive,
    ULONG           FileType
    );

typedef
struct _CELL_DATA *
(*PGET_CELL_ROUTINE)(
    struct _HHIVE   *Hive,
    HCELL_INDEX Cell
    );

//
// --- HHIVE --- In memory descriptor for a hive.
//

//
// HHIVE contains pointers to service procedures, and pointers to
// map structure.
//
// NOTE:    Optimization - If the size of a hive is less than what can
//          be mapped with a single HMAP_TABLE (HTABLE_SLOTS * HBLOCK_SIZE,
//          or 2 megabytes) there is no real HMAP_DIRECTORY.  Instead,
//          a single DWORD in the HHIVE acts as the 0th entry of the
//          directory.
//
// NOTE:    Free Storage Management - When a hive is loaded, we build up
//          a display (vector) of lists of free cells.  The first part
//          of this vector contains lists that only hold one size cell.
//          The size of cell on the list is HCELL_PAD * (ListIndex+1)
//          There are 15 of these lists, so all free cells between 8 and
//          120 bytes are on these lists.
//
//          The second part of this vector contains lists that hold more
//          than one size cell.  Each size bucket is twice the previous
//          size.  There are 8 of these lists, so all free cells between 136 and
//          32768 bytes are on these lists.
//
//          The last list in this vector contains all cells too large to
//          fit in any previous list.
//
//          Example:    All free cells of size 1 HCELL_PAD (8 bytes)
//                      are on the list at offset 0 in FreeDisplay.
//
//                      All free cells of size 15 HCELL_PAD (120 bytes)
//                      are on the list at offset 0xe.
//
//                      All free cells of size 16-31 HCELL_PAD (128-248 bytes)
//                      are on the list at offset 0xf
//
//                      All free cells of size 32-63 HCELL_PAD (256-506 bytes)
//                      are on the list at offset 0x10.
//
//                      All free cells of size 2048 HCELL_PAD (16384 bytes)
//                      OR greater, are on the list at offset 0x17.
//
//          FreeSummary is a bit vector, with a bit set to true for each
//          entry in FreeDisplay that is not empty.
//

#define HHIVE_SIGNATURE 0xBEE0BEE0

#define HFILE_TYPE_PRIMARY      0   // Base hive file
#define HFILE_TYPE_ALTERNATE    1   // Alternate (e.g. system.alt)
#define HFILE_TYPE_LOG          2   // Log (security.log)
#define HFILE_TYPE_EXTERNAL     3   // Target of savekey, etc.
#define HFILE_TYPE_MAX          4

#define HHIVE_LINEAR_INDEX      16  // All computed linear indices < HHIVE_LINEAR_INDEX are valid
#define HHIVE_EXPONENTIAL_INDEX 23  // All computed exponential indices < HHIVE_EXPONENTIAL_INDEX
                                    // and >= HHIVE_LINEAR_INDEX are valid.
#define HHIVE_FREE_DISPLAY_SIZE 24

#define HHIVE_FREE_DISPLAY_SHIFT 3  // This must be log2 of HCELL_PAD!
#define HHIVE_FREE_DISPLAY_BIAS  7  // Add to first set bit left of cell size to get exponential index


#define FREE_HBIN_DISCARDABLE 1

typedef struct _FREE_HBIN {
    LIST_ENTRY  ListEntry;
    ULONG       Size;
    ULONG       FileOffset;
    ULONG       Flags;
} FREE_HBIN, *PFREE_HBIN;

typedef struct _HHIVE {
    ULONG                   Signature;

    PGET_CELL_ROUTINE       GetCellRoutine;

    PALLOCATE_ROUTINE       Allocate;
    PFREE_ROUTINE           Free;

    PFILE_SET_SIZE_ROUTINE  FileSetSize;
    PFILE_WRITE_ROUTINE     FileWrite;
    PFILE_READ_ROUTINE      FileRead;
    PFILE_FLUSH_ROUTINE     FileFlush;

    struct _HBASE_BLOCK     *BaseBlock;

    RTL_BITMAP              DirtyVector;    // only for Stable bins
    ULONG                   DirtyCount;
    ULONG                   DirtyAlloc;     // allocated bytges for dirty vect
    ULONG                   Cluster;        // Usually 1 512 byte sector.
                                            // Set up force writes to be
                                            // done in larger units on
                                            // machines with larger sectors.
                                            // Is number of logical 512 sectors.

    BOOLEAN                 Flat;               // TRUE if FLAT
    BOOLEAN                 ReadOnly;           // TRUE if READONLY

    BOOLEAN                 Log;
    BOOLEAN                 Alternate;

    ULONG                   HiveFlags;

    ULONG                   LogSize;

    ULONG                   RefreshCount;       // debugging aid


    ULONG                   StorageTypeCount;   // 1 > Number of largest valid
                                                // type. (1 for Stable only,
                                                // 2 for stable & volatile)

    ULONG                   Version;            // hive version, to allow supporting multiple
                                                // formats simultaneously.

    struct _DUAL {
        ULONG               Length;
        PHMAP_DIRECTORY     Map;
        PHMAP_TABLE         SmallDir;
        ULONG               Guard;          // Always == -1

        HCELL_INDEX         FreeDisplay[HHIVE_FREE_DISPLAY_SIZE];
        ULONG               FreeSummary;
        LIST_ENTRY          FreeBins;           // list of freed HBINs (FREE_HBIN)

    }                       Storage[ HTYPE_COUNT ];

    //
    // Caller defined data goes here
    //

} HHIVE, *PHHIVE;


#endif // __HIVE_DATA__


