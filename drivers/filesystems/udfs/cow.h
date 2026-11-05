/*
 * PROJECT:     ReactOS UDF File System Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Copy-on-write checkpoints for the UDF 2.50 metadata partition
 */

#ifndef _UDF_COW_H_
#define _UDF_COW_H_

/*
 * The metadata partition is reached through the Metadata File, whose
 * allocation descriptors map metadata blocks to physical blocks. Copy-on-
 * write keeps two physical copies of that area: region 0 is the one the
 * driver was mounted with (its "home" addresses, which every in-memory
 * structure keeps using) and region 1 is a second allocation of the same
 * shape. Metadata writes are held in memory per unit. A checkpoint writes
 * the changed units into the region the disk does not point to, then
 * rewrites the Metadata File FE (one sector) to point to that region; the
 * mirror follows the same way. A power loss therefore always leaves the
 * metadata as of the last checkpoint. Blocks freed in between stay
 * allocated until a checkpoint no longer refers to them.
 *
 * Only the start of the metadata area, up to Limit, gets a second region:
 * the blocks in use plus some headroom. Metadata allocations stay below
 * Limit while copy-on-write is on; when that space runs out the second
 * region is rebuilt larger (UDFCowWiden).
 *
 * The background checkpoint waits for the volume resource, so it never
 * records a create, delete or rename half done.
 *
 * File data: an overwrite of recorded file data is written to newly
 * allocated blocks and the file's mapping is switched to them, so a crash
 * leaves the old data in place until the file entry that points to the new
 * blocks is committed. The old blocks are released only after a file entry
 * built from the new mapping has been written, and then wait two
 * checkpoints like any other free.
 */

#define UDF_COW_UNIT_BLOCKS     8       // 4 KB units with 512-byte blocks
#define UDF_COW_INTERVAL_SEC    5       // checkpoint period
#define UDF_COW_DIRTY_KICK      2560    // ~10 MB changed: checkpoint early
#define UDF_COW_MAX_RUNS        64      // Metadata File extents handled
#define UDF_COW_HEADROOM        131072  // 64 MB of free metadata blocks covered
#define UDF_COW_QUIESCE_TRIES   150     // x 20 ms waiting for the volume resource

typedef struct _UDF_COW_RUN {
    uint32 Lbn;         // first metadata block of the extent
    uint32 Blocks;
    uint32 Lba[2];      // physical start in region 0 (home) and region 1
} UDF_COW_RUN, *PUDF_COW_RUN;

typedef struct _UDF_COW_FREE {
    struct _UDF_COW_FREE* Next;
    LONG Gen;           // file data: mapping generation that dropped them
    EXTENT_MAP Map[1];  // terminated copy of a released mapping
} UDF_COW_FREE, *PUDF_COW_FREE;

typedef struct _UDF_COW {
    uint32 UnitBlocks;
    uint32 UnitBytes;
    uint32 Units;
    uint32 MetadataBlocks;
    uint32 Limit;           // metadata blocks [0, Limit) have a second region
    uint32 MainRuns;
    uint32 MirrorRuns;
    UDF_COW_RUN Main[UDF_COW_MAX_RUNS];
    UDF_COW_RUN Mirror[UDF_COW_MAX_RUNS];
    PEXTENT_MAP AltMaps[2 * UDF_COW_MAX_RUNS];  // region 1 allocations
    uint32 AltMapCount;
    BOOLEAN Duplicated;
    uint8 Region;           // region the on-disk Metadata File points to
    int8** Buffer;          // per unit: contents changed since last checkpoint
    uint8* Stale;           // per unit bit: the other region is out of date
    uint32 DirtyUnits;
    // Frees wait two checkpoints: Pending collects them, Waiting holds the
    // ones detached at the previous checkpoint. Guarded by BitMapResource1.
    PUDF_COW_FREE Pending;
    PUDF_COW_FREE Waiting;
    PETHREAD ApplyThread;
    BOOLEAN TrimUnsupported;
    // checkpoint thread
    KEVENT Wake;
    volatile LONG Stop;
    PVOID Thread;
    ULONG Checkpoints;
    ULONG DataBlocksMoved;  // file data written to new blocks
} UDF_COW, *PUDF_COW;

NTSTATUS
UDFCowStart(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
UDFCowWiden(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 Blocks
    );

uint32
UDFCowMetadataLimit(
    IN PVCB Vcb
    );

NTSTATUS
UDFCowSync(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
UDFCowStop(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
UDFCowTeardown(
    IN PVCB Vcb
    );

NTSTATUS
UDFCowCheckpoint(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN ApplyAllFrees
    );

BOOLEAN
UDFCowIo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN PVOID Buffer,
    IN BOOLEAN IsWrite,
    OUT NTSTATUS* Status
    );

BOOLEAN
UDFCowDeferFree(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map
    );

PUDF_COW_FREE
UDFCowRelocateData(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN struct FCB* Fcb,
    IN LONGLONG Offset,
    IN ULONG Length
    );

VOID
UDFCowRelocateDone(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc,
    IN PUDF_COW_FREE Old
    );

VOID
UDFCowDataFeWritten(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    );

VOID
UDFCowDataDlocGone(
    IN PVCB Vcb,
    IN PUDF_DATALOC_INFO Dloc
    );

#endif // _UDF_COW_H_
