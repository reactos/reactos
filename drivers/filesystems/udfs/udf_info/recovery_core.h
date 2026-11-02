/* Bounded UDF recovery core. Probe and planning are read-only. Apply may
 * perform only the separately validated ordered bitmap/LVID sector edits.
 * Keep this core independent of NT I/O for stopped-image cross-checks. */
#ifndef UDFS_RECOVERY_CORE_H
#define UDFS_RECOVERY_CORE_H

#include <stdint.h>
#include <stddef.h>

#define UDFS_RECOVERY_SECTOR_SIZE 512U
#define UDFS_RECOVERY_MAX_EXTENTS 128U
/* First missing LBAs retained for diagnostics, not a repair limit. */
#define UDFS_RECOVERY_MAX_REPAIR_BLOCKS 4U
/* Bound nonpaged plan memory by bitmap sectors, not file count. */
/* Distinct bitmap sectors one repair may rewrite. A large install cut off
 * by power loss can leave over a gigabyte referenced but free, hundreds of
 * sectors even when contiguous. 4096 sectors cover 16M blocks; the plan
 * stays near 4 MB. */
#define UDFS_RECOVERY_MAX_BITMAP_EDITS 4096U

typedef int (*UdfsRecoveryReadSector)(void *context, uint32_t lba,
                                      uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE]);
typedef void *(*UdfsRecoveryAllocate)(void *context, size_t bytes);
typedef void (*UdfsRecoveryFree)(void *context, void *buffer);
typedef int (*UdfsRecoveryWriteSector)(void *context, uint32_t lba,
                                       const uint8_t sector[UDFS_RECOVERY_SECTOR_SIZE]);
typedef int (*UdfsRecoveryFlush)(void *context);

typedef struct _UDFS_RECOVERY_READER {
    UdfsRecoveryReadSector ReadSector;
    UdfsRecoveryAllocate Allocate;
    UdfsRecoveryFree Free;
    void *Context;
    uint32_t SectorCount;
    UdfsRecoveryWriteSector WriteSector;
    UdfsRecoveryFlush Flush;
} UDFS_RECOVERY_READER;

typedef enum _UDFS_RECOVERY_PROBE_STATUS {
    UdfsRecoveryProbeValidLayout = 0,
    UdfsRecoveryProbeInvalidInput,
    UdfsRecoveryProbeIoError,
    UdfsRecoveryProbeNoMemory,
    UdfsRecoveryProbeCorrupt,
    UdfsRecoveryProbeGraphMismatch,
    UdfsRecoveryProbeLvidMismatch,
    UdfsRecoveryProbeUnsupported
} UDFS_RECOVERY_PROBE_STATUS;

typedef struct _UDFS_RECOVERY_PROBE {
    UDFS_RECOVERY_PROBE_STATUS Status;
    uint32_t FailureLba;
    uint32_t AnchorLba;
    uint32_t VdsStartLba;
    uint32_t MainVdsStartLba;
    uint32_t MainVdsBytes;
    uint32_t ReserveVdsStartLba;
    uint32_t ReserveVdsBytes;
    uint32_t PartitionStart;
    uint32_t PartitionBlocks;
    uint32_t PhysicalBitmapLbn;
    uint32_t MetadataMainFeLbn;
    uint32_t MetadataMirrorFeLbn;
    uint32_t MetadataBitmapFeLbn;
    uint32_t MetadataBlocks;
    uint32_t MetadataBitmapBits;
    uint32_t MainExtents;
    uint32_t MirrorExtents;
    uint32_t PhysicalFreeBlocks;
    uint32_t MetadataFreeBlocks;
    uint32_t AllocatedMirrorBlocksCompared;
    uint32_t ReferencedFreeMirrorBlocksCompared;
    uint32_t PrevailingLvidLba;
    uint32_t PrecedingClosedLvidLba;
    uint32_t JournalOpenLvidLba;
    uint32_t JournalOtherLvidLba;
    uint64_t JournalGeneration;
    uint8_t JournalOtherDamaged;
    uint32_t LvidPhysicalFreeBlocks;
    uint32_t LvidMetadataFreeBlocks;
    uint32_t LvidPhysicalSizeBlocks;
    uint32_t LvidMetadataSizeBlocks;
    uint64_t LvidNextUniqueId;
    uint32_t LvidFileCount;
    uint32_t LvidDirectoryCount;
    uint32_t OwnedPhysicalBlocks;
    uint32_t OwnedMetadataBlocks;
    uint32_t ReferencedFreePhysicalBlocks;
    uint32_t ReferencedFreeMetadataBlocks;
    uint32_t ReferencedFreeInlineFileFeBlocks;
    uint32_t UnownedPhysicalBlocks;
    uint32_t UnownedMetadataBlocks;
    uint32_t FirstReferencedFreePhysicalLbn;
    uint32_t FirstReferencedFreeMetadataLbn;
    uint32_t ReferencedFreeMetadataLbn[UDFS_RECOVERY_MAX_REPAIR_BLOCKS];
    uint32_t FirstUnownedPhysicalLbn;
    uint32_t FirstUnownedMetadataLbn;
    uint32_t OverlapBlocks;
    uint32_t FilesVisited;
    uint32_t DirectoriesVisited;
    uint32_t FidsChecked;
    uint64_t MaxUniqueId;
    uint32_t FailureObjectLbn;
    uint16_t FailureObjectRef;
    /* recovery_core.c line of the first failed namespace check, 0 if none. */
    uint32_t FailureLine;
    /* One bounded empty-directory pre-FID deletion transaction. */
    uint32_t DirectoryLinkFixParentLbn;
    uint32_t DirectoryLinkFixChildLbn;
    uint16_t DirectoryLinkFixParentRef;
    uint16_t DirectoryLinkFixStored;
    uint16_t DirectoryLinkFixExpected;
    uint8_t DirectoryLinkFixCount;
    uint8_t DirectoryLinkFixChildVerified;
    uint32_t MirrorDirectoryLbn;
    uint16_t MirrorDirectoryMainLinks;
    uint16_t MirrorDirectoryMirrorLinks;
    uint8_t MirrorDirectorySplitCount;
    uint32_t FidSplitLbn;
    uint32_t FidSplitMainLba;
    uint32_t FidSplitMirrorLba;
    uint32_t FidSplitChildLbn;
    uint32_t FidSplitRecordOffset;
    uint32_t FidSplitParentLbn;
    uint8_t FidSplitCount;
    uint8_t FidSplitLiveOnMirror;
    uint8_t FidSplitSeen;
    uint8_t MetadataDuplicated;
    uint8_t BitmapContentsScanned;
    uint8_t LvidIntegrityType;
    uint8_t LvidTagRepairMode; /* 0 none, 1 checksum, 2 linked header, 3 journal */
    uint8_t LvidCountersMatch;
    uint8_t OwnershipGraphScanned;
    /* Valid FE/EFE candidates in bitmap-free metadata blocks were checked. */
    uint8_t FreeMetadataFeScanComplete;
    uint8_t CompleteOwnershipGraph;
    uint8_t WritableMountAuthorized;
} UDFS_RECOVERY_PROBE;

typedef struct _UDFS_RECOVERY_SECTOR_EDIT {
    uint32_t Lba;
    uint8_t Before[UDFS_RECOVERY_SECTOR_SIZE];
    uint8_t After[UDFS_RECOVERY_SECTOR_SIZE];
} UDFS_RECOVERY_SECTOR_EDIT;

/* Planning is read-only. These bytes must never be written until a separate
 * caller establishes exclusive access, durable ordering and retry semantics. */
typedef struct _UDFS_RECOVERY_REPAIR_PLAN {
    uint32_t BitmapEditCount;
    uint32_t LastBitmapEdit;    /* lookup hint: blocks arrive in order */
    UDFS_RECOVERY_SECTOR_EDIT BitmapEdits[UDFS_RECOVERY_MAX_BITMAP_EDITS];
    uint32_t FeEditCount;
    UDFS_RECOVERY_SECTOR_EDIT FeEdits[2];
    uint32_t FidEditCount;
    uint32_t FidEditLogicalLbn;
    uint32_t FidLiveLba;
    UDFS_RECOVERY_SECTOR_EDIT FidEdit;
    UDFS_RECOVERY_SECTOR_EDIT LvidEdit;
    uint32_t PrecedingClosedLvidLba;
    uint8_t PrecedingClosedLvid[UDFS_RECOVERY_SECTOR_SIZE];
    uint32_t JournalOpenLvidLba;
    uint8_t JournalOpenLvid[UDFS_RECOVERY_SECTOR_SIZE];
    UDFS_RECOVERY_SECTOR_EDIT JournalNextLvidEdit;
    uint8_t LvidTagRepairMode;
    /* A clean, unlinked legacy extent has no durable sidecar yet. During
     * its first recovery, write this prepared sidecar before CLOSED LVID. */
    uint8_t LegacyJournalBootstrap;
    /* A valid journaled OPEN checkpoint needs adjusted counters. Publish
     * the exact new OPEN state in the alternate slot before CLOSED LVID. */
    uint8_t JournalRepairRefresh;
    uint8_t JournalOtherDamaged;
    uint8_t Ready;
} UDFS_RECOVERY_REPAIR_PLAN;

UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryProbeLayout(const UDFS_RECOVERY_READER *reader,
                        UDFS_RECOVERY_PROBE *result);

/* Cheap read-only discriminator for the supported duplicated-metadata class.
 * ReadSector, Allocate and Free are required; the driver supplies nonpaged
 * storage for the working sectors instead of using the mount thread stack.
 * ValidLayout returns an integrity type of 0 (OPEN) or 1 (CLOSED). All other
 * results require the existing mount policy and authorize no repair. */
UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryPeekIntegrity(const UDFS_RECOVERY_READER *reader,
                          uint8_t *integrityType);

UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryPrepareRepair(const UDFS_RECOVERY_READER *reader,
                          const UDFS_RECOVERY_PROBE *probe,
                          const uint8_t recordingTime[12],
                          UDFS_RECOVERY_REPAIR_PLAN *plan);

UDFS_RECOVERY_PROBE_STATUS
UdfsRecoveryApplyRepair(const UDFS_RECOVERY_READER *reader,
                        const UDFS_RECOVERY_REPAIR_PLAN *plan,
                        UDFS_RECOVERY_PROBE *after);

#endif
