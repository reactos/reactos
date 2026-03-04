/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Implements routines to support booting from a RAM Disk.
 * COPYRIGHT:   Copyright 2008 ReactOS Portable Systems Group
 *              Copyright 2009 Hervé Poussineau
 *              Copyright 2019 Hermes Belusca-Maito
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <debug.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <ntstrsafe.h>
#include <fs/iso.h>
#include <fs/fat.h>
#include <disk.h>
#include <arch/archwsup.h>

#if defined(__GNUC__)
extern VOID
AddReactOSArcDiskInfo(
    IN PSTR ArcName,
    IN ULONG Signature,
    IN ULONG Checksum,
    IN BOOLEAN ValidPartitionTable) __attribute__((weak));
#elif defined(_MSC_VER)
VOID
AddReactOSArcDiskInfoStub(
    IN PSTR ArcName,
    IN ULONG Signature,
    IN ULONG Checksum,
    IN BOOLEAN ValidPartitionTable)
{
    UNREFERENCED_PARAMETER(ArcName);
    UNREFERENCED_PARAMETER(Signature);
    UNREFERENCED_PARAMETER(Checksum);
    UNREFERENCED_PARAMETER(ValidPartitionTable);
}

#if defined(_M_IX86)
#pragma comment(linker, "/alternatename:_AddReactOSArcDiskInfo=_AddReactOSArcDiskInfoStub")
#else
#pragma comment(linker, "/alternatename:AddReactOSArcDiskInfo=AddReactOSArcDiskInfoStub")
#endif
#endif

#if defined(__GNUC__)
VOID FatFlushCache(VOID) __attribute__((weak));
#else
VOID FatFlushCache(VOID);
#if defined(_MSC_VER)
VOID FatFlushCacheStub(VOID)
{
    /* Optional legacy FAT cache flush is unavailable; nothing to do. */
}
#if defined(_M_IX86)
#pragma comment(linker, "/alternatename:_FatFlushCache=_FatFlushCacheStub")
#else
#pragma comment(linker, "/alternatename:FatFlushCache=FatFlushCacheStub")
#endif
#endif
#endif
#include <ramdisk_fatwrite.h>
#include <ramdisk_signature.h>
#include "../ntldr/ntldropts.h"

DBG_DEFAULT_CHANNEL(DISK);

#if DBG
static
VOID
RamDiskTraceSample(IN PCSTR Label,
                   IN const VOID *Address)
{
    const UCHAR *Bytes;

    if (!Label || !Address)
    {
        return;
    }

    Bytes = (const UCHAR *)Address;
    TRACE("%s [%p]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
          Label,
          Address,
          Bytes[0], Bytes[1], Bytes[2], Bytes[3],
          Bytes[4], Bytes[5], Bytes[6], Bytes[7]);
}

static
PFN_NUMBER
RamDiskPointerToPfn(IN const VOID *Address)
{
    return Address ? (PFN_NUMBER)(((ULONG_PTR)Address) >> MM_PAGE_SHIFT) : 0;
}
#endif

#define RAMDISK_ALLOCATION_ALIGNMENT 0x1000ULL
#define ALIGN_UP_BY_ULL(Value, Alignment) \
    (((Value) + ((Alignment) - 1ULL)) & ~((Alignment) - 1ULL))

/* GLOBALS ********************************************************************/

PVOID gInitRamDiskBase = NULL;
ULONG gInitRamDiskSize = 0;

static BOOLEAN   RamDiskDeviceRegistered = FALSE;
static PVOID     RamDiskBase;
static ULONGLONG RamDiskFileSize;    // FIXME: RAM disks currently limited to 4GB.
static ULONGLONG RamDiskImageLength; // Total bytes populated in the backing store from the start of the allocation
static ULONG     RamDiskImageOffset; // Starting offset from the Ramdisk base.
static ULONGLONG RamDiskVolumeOffset; // Offset where the FAT volume starts (typically after the MBR)
static ULONGLONG RamDiskVolumeLength; // Length of the exposed FAT volume
static ULONGLONG RamDiskOffset;      // Current position in the Ramdisk.
static ULONGLONG RamDiskRequestedSize = 0;
static PVOID     RamDiskWritableBase = NULL;
static ULONGLONG RamDiskWritableSize = 0;
static BOOLEAN   RamDiskErrorShown = FALSE;

#if defined(_M_AMD64) || defined(__x86_64__) || defined(_M_ARM64) || defined(__aarch64__)
#ifndef MM_MAX_PAGE_LOADER_MAPPED
#define MM_MAX_PAGE_LOADER_MAPPED MM_MAX_PAGE_LOADER
#endif
#define RAMDISK_MAX_LOW_BYTES     ((ULONGLONG)MM_MAX_PAGE_LOADER_MAPPED << MM_PAGE_SHIFT)
#else
#define RAMDISK_MAX_LOW_BYTES     (0x40000000ULL) /* 1 GiB on 32-bit */
#endif

#define RAMDISK_LOW_ALLOC_MAX        RAMDISK_MAX_LOW_BYTES
#define RAMDISK_MINIMUM_EXTRA_SPACE  (64ULL * 1024ULL * 1024ULL)
#define RAMDISK_SAFETY_SLACK         RAMDISK_MINIMUM_EXTRA_SPACE

static
BOOLEAN
RamDiskComputeMbrMetadata(IN PVOID BaseAddress,
                          IN ULONGLONG DiskSize,
                          OUT PULONG Signature,
                          OUT PULONG Checksum,
                          OUT PBOOLEAN ValidPartition)
{
    PMASTER_BOOT_RECORD MasterBootRecord;
    ULONG Sum = 0;
    ULONG WordCount;
    PULONG Words;

    if (!BaseAddress || DiskSize < sizeof(MASTER_BOOT_RECORD))
        return FALSE;

    MasterBootRecord = (PMASTER_BOOT_RECORD)BaseAddress;
    if (MasterBootRecord->MasterBootRecordMagic != 0xAA55)
        return FALSE;

    WordCount = (ULONG)(sizeof(MASTER_BOOT_RECORD) / sizeof(ULONG));
    Words = (PULONG)MasterBootRecord;
    for (ULONG Index = 0; Index < WordCount; ++Index)
    {
        Sum += Words[Index];
    }

    if (MasterBootRecord->Signature == 0 || MasterBootRecord->Signature == 0xFFFFFFFFu)
    {
        MasterBootRecord->Signature = RamDiskDeriveDiskSignature(BaseAddress, DiskSize);

        /* Recalculate the checksum after updating the signature. */
        Sum = 0;
        for (ULONG Index = 0; Index < WordCount; ++Index)
        {
            Sum += Words[Index];
        }
    }

    if (Signature)
        *Signature = MasterBootRecord->Signature;

    if (Checksum)
        *Checksum = (~Sum) + 1;

    if (ValidPartition)
    {
        BOOLEAN Found = FALSE;

        for (ULONG EntryIndex = 0; EntryIndex < RTL_NUMBER_OF(MasterBootRecord->PartitionTable); ++EntryIndex)
        {
            const PARTITION_TABLE_ENTRY *Entry = &MasterBootRecord->PartitionTable[EntryIndex];

            if (Entry->SystemIndicator != PARTITION_ENTRY_UNUSED &&
                Entry->PartitionSectorCount != 0)
            {
                Found = TRUE;
                break;
            }
        }

        *ValidPartition = Found;
    }

    return TRUE;
}

static
VOID
RamDiskRegisterArcDevice(VOID)
{
    static BOOLEAN ArcRegistered = FALSE;
    ULONG Signature;
    ULONG Checksum;
    BOOLEAN ValidPartition;

    if (ArcRegistered)
        return;

    if (!RamDiskBase || RamDiskFileSize < sizeof(MASTER_BOOT_RECORD))
        return;

    if (!RamDiskComputeMbrMetadata(RamDiskBase,
                                   RamDiskFileSize,
                                   &Signature,
                                   &Checksum,
                                   &ValidPartition))
    {
        return;
    }

    if (AddReactOSArcDiskInfo)
    {
        AddReactOSArcDiskInfo("ramdisk(0)", Signature, Checksum, ValidPartition);
    }
    ArcRegistered = TRUE;
}

static BOOLEAN RamDiskReserveWritableBuffer(ULONGLONG RequestedSize, BOOLEAN OptionalRamDisk);

static VOID
RamDiskSetVisibleRegion(IN ULONGLONG Offset,
                        IN ULONGLONG Length)
{
    RamDiskVolumeOffset = Offset;
    RamDiskVolumeLength = Length;
    /* Note: Caller must call RamDiskInvalidateFatCache() after changing visible LBA window,
       as the FAT mount state is invalidated when the underlying disk region changes */
}

static VOID
RamDiskResetVisibleRegion(VOID)
{
    ULONGLONG VisibleLength = 0;

    if (RamDiskImageLength > RamDiskImageOffset)
        VisibleLength = RamDiskImageLength - RamDiskImageOffset;

    RamDiskSetVisibleRegion(RamDiskImageOffset, VisibleLength);
}

static VOID
RamDiskInvalidateFatCache(VOID)
{
#if defined(__GNUC__)
    if (FatFlushCache)
        FatFlushCache();
#else
    FatFlushCache();
#endif
}

static
ULONGLONG
RamDiskWritableAllocationLimit(VOID)
{
    if (RAMDISK_LOW_ALLOC_MAX > RAMDISK_SAFETY_SLACK)
        return RAMDISK_LOW_ALLOC_MAX - RAMDISK_SAFETY_SLACK;

    return RAMDISK_LOW_ALLOC_MAX;
}

#define ISO_SECTOR_SIZE 2048
#define ISO_DIRECTORY_MAX_SIZE    (32 * 1024 * 1024)
#define ISO_NAME_BUFFER_SIZE      256

typedef struct _ISO_SOURCE
{
    const UCHAR *MemoryBase;
    ULONGLONG Size;
    ULONG ArcFileId;
    ULONGLONG ArcOffset;
    ULONGLONG ArcPosition;
} ISO_SOURCE, *PISO_SOURCE;

typedef struct _ISO_COPY_CONTEXT
{
    PISO_SOURCE Source;
    FAT32_WRITER Writer;
    PUCHAR ScratchBuffer;
    ULONG ScratchBufferSize;
    ULONG ScratchPreferred;
    PUCHAR DirectoryBuffer;
    ULONG DirectoryBufferSize;
    BOOLEAN DirectoryBufferBusy;
    BOOLEAN ProgressActive;
    ULONGLONG TotalBytes;
    ULONGLONG BytesCopied;
    ULONG LastPercentShown;
    CHAR ProgressMessage[128];
    ULONG ProgressStartSeconds;
    ULONG LastDisplaySeconds;
    ULONG LastSpeedKBs;
    ULONG LastSecsLeft;
} ISO_COPY_CONTEXT, *PISO_COPY_CONTEXT;

#define ISO_SCRATCH_MIN_SIZE     (1024 * 1024)
#define ISO_SCRATCH_MAX_SIZE     (8 * 1024 * 1024)
#define ISO_SCRATCH_FLOOR_SIZE   (128 * 1024)
#define ISO_STREAM_FALLBACK_CHUNK (1024 * 1024)
#define TAG_ISO_BUFFER 'BosI'

static
ULONG
RamDiskGetRelativeTime(VOID)
{
    return ArcGetRelativeTime();
}

static
ULONG
IsoGetPreferredChunkSize(VOID)
{
    ULONGLONG Preferred = ISO_STREAM_FALLBACK_CHUNK;

    if (DiskReadBufferSize != 0)
    {
        ULONGLONG Candidate = (ULONGLONG)DiskReadBufferSize;
        if (Candidate > ISO_SCRATCH_MAX_SIZE)
            Candidate = ISO_SCRATCH_MAX_SIZE;
        Preferred = Candidate;
    }

    if (Preferred < ISO_SCRATCH_MIN_SIZE)
        Preferred = ISO_SCRATCH_MIN_SIZE;

    if (Preferred > ISO_SCRATCH_MAX_SIZE)
        Preferred = ISO_SCRATCH_MAX_SIZE;

    Preferred = ALIGN_UP_BY_ULL(Preferred, ISO_SECTOR_SIZE);
    Preferred = ALIGN_UP_BY_ULL(Preferred, MM_PAGE_SIZE);

    if (Preferred > ISO_SCRATCH_MAX_SIZE)
        Preferred = ISO_SCRATCH_MAX_SIZE;

    return (ULONG)Preferred;
}

static
ULONG
IsoAlignScratchSize(
    _In_ ULONGLONG Value)
{
    ULONGLONG Result;

    if (Value < ISO_SECTOR_SIZE)
        Value = ISO_SECTOR_SIZE;

    Result = ALIGN_UP_BY_ULL(Value, ISO_SECTOR_SIZE);
    Result = ALIGN_UP_BY_ULL(Result, MM_PAGE_SIZE);

    if (Result > ISO_SCRATCH_MAX_SIZE)
        Result = ISO_SCRATCH_MAX_SIZE;

    return (ULONG)Result;
}

static
BOOLEAN
IsoEnsureScratchBuffer(
    _Inout_ PISO_COPY_CONTEXT Context,
    _In_ ULONGLONG RequiredSize)
{
    ULONG Preferred;
    ULONG Floor;
    ULONG Target;
    ULONG Attempt;
    ULONG Previous;
    PVOID NewBuffer = NULL;

    if (!Context)
        return FALSE;

    Preferred = IsoGetPreferredChunkSize();
    Floor = IsoAlignScratchSize(ISO_SCRATCH_FLOOR_SIZE);

    if (Context->ScratchPreferred != 0 && Preferred > Context->ScratchPreferred)
        Preferred = Context->ScratchPreferred;

    if (Context->ScratchBuffer && Context->ScratchBufferSize > Preferred)
        Preferred = Context->ScratchBufferSize;

    if (Preferred < Floor)
        Preferred = Floor;

    if (RequiredSize == 0)
        RequiredSize = ISO_SECTOR_SIZE;

    if (RequiredSize > ISO_SCRATCH_MAX_SIZE)
        RequiredSize = ISO_SCRATCH_MAX_SIZE;

    Target = IsoAlignScratchSize(RequiredSize);

    if (Target < Floor)
        Target = Floor;

    if (Target > Preferred)
        Target = Preferred;

    if (Context->ScratchBuffer && Context->ScratchBufferSize >= Target)
    {
        Context->ScratchPreferred = Context->ScratchBufferSize;
        return TRUE;
    }

    Attempt = Target;
    Previous = 0;

    while (Attempt >= Floor && Attempt != Previous)
    {
        if (Context->ScratchBuffer && Context->ScratchBufferSize >= Attempt)
            return TRUE;

        NewBuffer = FrLdrTempAlloc(Attempt, TAG_ISO_BUFFER);
        if (NewBuffer)
            break;

        Previous = Attempt;

        if (Attempt == Floor)
            break;

        Attempt /= 2;
        if (Attempt < Floor)
            Attempt = Floor;

        Attempt = IsoAlignScratchSize(Attempt);
    }

    if (!NewBuffer)
    {
        if (Context->ScratchBuffer && Context->ScratchBufferSize >= Floor)
        {
            Context->ScratchPreferred = Context->ScratchBufferSize;
            return TRUE;
        }

        return FALSE;
    }

    if (Context->ScratchBuffer)
    {
        FrLdrTempFree(Context->ScratchBuffer, TAG_ISO_BUFFER);
    }

    Context->ScratchBuffer = NewBuffer;
    Context->ScratchBufferSize = Attempt;
    Context->ScratchPreferred = Attempt;
    return TRUE;
}

static
VOID
IsoProgressDisplay(
    _Inout_ PISO_COPY_CONTEXT Context,
    _In_ ULONG Percent)
{
    if (!Context)
        return;

    UiUpdateProgressBar(Percent, Context->ProgressMessage);
}

static
VOID
IsoProgressInitialize(
    _Inout_ PISO_COPY_CONTEXT Context)
{
    if (!Context || !Context->Source || Context->Source->Size == 0)
        return;

    Context->TotalBytes = Context->Source->Size;
    Context->BytesCopied = 0;
    Context->LastPercentShown = 0;
    Context->ProgressActive = TRUE;
    Context->ProgressStartSeconds = RamDiskGetRelativeTime();
    Context->LastDisplaySeconds = Context->ProgressStartSeconds;
    Context->LastSpeedKBs = 0;
    Context->LastSecsLeft = 0;

    UiDrawProgressBarCenter("Copying files...");

    RtlStringCbPrintfA(Context->ProgressMessage,
                       sizeof(Context->ProgressMessage),
                       "Copying files...");
    IsoProgressDisplay(Context, 0);
}

static
VOID
IsoProgressAdvance(
    _Inout_ PISO_COPY_CONTEXT Context,
    _In_ ULONGLONG Bytes)
{
    ULONG Percent;
    ULONG NowSeconds;

    if (!Context || !Context->ProgressActive || Context->TotalBytes == 0)
        return;

    Context->BytesCopied += Bytes;
    if (Context->BytesCopied > Context->TotalBytes)
        Context->BytesCopied = Context->TotalBytes;

    Percent = (ULONG)((Context->BytesCopied * 100ULL) / Context->TotalBytes);
    if (Percent > 100)
        Percent = 100;

    NowSeconds = RamDiskGetRelativeTime();

    /*
     * Recompute speed and ETA only when the seconds counter changes.
     * ArcGetRelativeTime has 1-second resolution; recomputing between
     * ticks would cause the displayed rate to jump wildly because
     * elapsed stays frozen while BytesCopied grows.
     */
    if (NowSeconds != Context->LastDisplaySeconds)
    {
        ULONG ElapsedSeconds = NowSeconds - Context->ProgressStartSeconds;

        if (ElapsedSeconds > 0)
        {
            ULONGLONG CopiedKB = Context->BytesCopied / 1024ULL;

            Context->LastSpeedKBs = (ULONG)(CopiedKB / (ULONGLONG)ElapsedSeconds);
            if (Context->LastSpeedKBs == 0)
                Context->LastSpeedKBs = 1;

            if (Context->BytesCopied < Context->TotalBytes)
            {
                ULONGLONG Remaining = Context->TotalBytes - Context->BytesCopied;
                /* seconds = remaining_bytes / (total_bytes_done / elapsed_s) */
                Context->LastSecsLeft = (ULONG)((Remaining * (ULONGLONG)ElapsedSeconds) /
                                                 Context->BytesCopied);
            }
            else
            {
                Context->LastSecsLeft = 0;
            }
        }

        Context->LastDisplaySeconds = NowSeconds;
    }

    /* Update the bar and message on every percent change */
    if (Percent != Context->LastPercentShown)
    {
        ULONGLONG CopiedKB = Context->BytesCopied / 1024ULL;
        ULONGLONG TotalKB = (Context->TotalBytes + 1023ULL) / 1024ULL;

        if (Percent >= 100)
        {
            RtlStringCbPrintfA(Context->ProgressMessage,
                               sizeof(Context->ProgressMessage),
                               "Copy complete");
        }
        else
        {
            RtlStringCbPrintfA(Context->ProgressMessage,
                               sizeof(Context->ProgressMessage),
                               "Ramdisk %u%% (%llu/%llu KB, %u KB/s, %us left)",
                               Percent,
                               CopiedKB,
                               TotalKB,
                               Context->LastSpeedKBs,
                               Context->LastSecsLeft);
        }

        IsoProgressDisplay(Context, Percent);
        TRACE("IsoProgress: %s\n", Context->ProgressMessage);
        Context->LastPercentShown = Percent;
    }
}

static
VOID
IsoProgressComplete(
    _Inout_ PISO_COPY_CONTEXT Context)
{
    if (!Context || !Context->ProgressActive)
        return;

    Context->BytesCopied = Context->TotalBytes;
    Context->ProgressActive = FALSE;
    Context->LastPercentShown = 100;

    RtlStringCbPrintfA(Context->ProgressMessage,
                       sizeof(Context->ProgressMessage),
                       "Copy complete");
    IsoProgressDisplay(Context, 100);
}

static
BOOLEAN
IsoSourceRead(
    _Inout_ PISO_SOURCE Source,
    _In_ ULONGLONG Offset,
    _Out_writes_(Size) PVOID Buffer,
    _In_ ULONG Size)
{
    ULONGLONG AbsoluteStart;
    const ULONG SectorMask = ISO_SECTOR_SIZE - 1;
    ULONG MaxChunkBytesTmp;
    ULONG MaxChunkBytes;
    PUCHAR Out;
    ULONG Remaining;
    ARC_STATUS Status;
    ULONG BytesRead;
    LARGE_INTEGER Position;
    UCHAR Bounce[ISO_SECTOR_SIZE];
    ULONGLONG CurrentPos;

    if (Size == 0)
        return TRUE;

    if (!Source || !Buffer)
        return FALSE;

    if (Source->MemoryBase)
    {
        if (Offset + Size > Source->Size)
            return FALSE;

        RtlCopyMemory(Buffer, Source->MemoryBase + Offset, Size);
        return TRUE;
    }

    if (Source->ArcFileId == INVALID_FILE_ID)
        return FALSE;

    if (Offset > Source->Size || Offset + Size > Source->Size)
        return FALSE;

    AbsoluteStart = Source->ArcOffset + Offset;
    MaxChunkBytesTmp = IsoGetPreferredChunkSize();

    if (Size > MaxChunkBytesTmp)
    {
        ULONGLONG Candidate = ALIGN_UP_BY_ULL(Size, ISO_SECTOR_SIZE);

        if (Candidate > ISO_SCRATCH_MAX_SIZE)
            Candidate = ISO_SCRATCH_MAX_SIZE;

        if (Candidate > ULONG_MAX)
            Candidate = ULONG_MAX & ~((ULONGLONG)ISO_SECTOR_SIZE - 1ULL);

        if (Candidate > MaxChunkBytesTmp)
            MaxChunkBytesTmp = (ULONG)Candidate;
    }

    MaxChunkBytesTmp &= ~SectorMask;
    MaxChunkBytes = (MaxChunkBytesTmp == 0) ? ISO_SECTOR_SIZE : MaxChunkBytesTmp;

    Out = (PUCHAR)Buffer;
    Remaining = Size;
    CurrentPos = Source->ArcPosition;

    /* Handle head misalignment */
    if ((AbsoluteStart & SectorMask) != 0)
    {
        ULONG CopyLength;
        ULONGLONG SectorStart;

        CopyLength = ISO_SECTOR_SIZE - (ULONG)(AbsoluteStart & SectorMask);
        if (CopyLength > Remaining)
            CopyLength = Remaining;

        SectorStart = AbsoluteStart - (AbsoluteStart & SectorMask);
        if (CurrentPos != SectorStart)
        {
            Position.QuadPart = SectorStart;
            Status = ArcSeek(Source->ArcFileId, &Position, SeekAbsolute);
            if (Status != ESUCCESS)
            {
                WARN("IsoSourceRead: ArcSeek failed (status %lu) at offset %I64u\n",
                     Status,
                     Position.QuadPart);
                return FALSE;
            }
            CurrentPos = SectorStart;
        }

        Status = ArcRead(Source->ArcFileId, Bounce, ISO_SECTOR_SIZE, &BytesRead);
        if (Status != ESUCCESS || BytesRead != ISO_SECTOR_SIZE)
        {
            WARN("IsoSourceRead: ArcRead failed (status %lu) at offset %I64u (head)\n",
                 Status,
                 SectorStart);
            return FALSE;
        }

        CurrentPos += ISO_SECTOR_SIZE;
        RtlCopyMemory(Out, Bounce + (AbsoluteStart & SectorMask), CopyLength);
        Out += CopyLength;
        Remaining -= CopyLength;
    }

    /* Handle middle aligned region */
    if (Remaining >= ISO_SECTOR_SIZE)
    {
        ULONGLONG Consumed;
        ULONGLONG AlignedOffset;
        ULONG AlignedBytes;

        Consumed = Size - Remaining;
        AlignedOffset = (AbsoluteStart + Consumed) & ~((ULONGLONG)SectorMask);
        AlignedBytes = Remaining & ~SectorMask;

        if (AlignedBytes)
        {
            if (CurrentPos != AlignedOffset)
            {
                Position.QuadPart = AlignedOffset;
                Status = ArcSeek(Source->ArcFileId, &Position, SeekAbsolute);
                if (Status != ESUCCESS)
                {
                    WARN("IsoSourceRead: ArcSeek failed (status %lu) at offset %I64u\n",
                         Status,
                         Position.QuadPart);
                    return FALSE;
                }
                CurrentPos = AlignedOffset;
            }

            while (AlignedBytes > 0)
            {
                ULONG Chunk = (AlignedBytes > MaxChunkBytes) ? MaxChunkBytes : AlignedBytes;

                Status = ArcRead(Source->ArcFileId, Out, Chunk, &BytesRead);
                if (Status != ESUCCESS || BytesRead != Chunk)
                {
                    WARN("IsoSourceRead: ArcRead failed (status %lu) at offset %I64u (aligned chunk %lu)\n",
                         Status,
                         CurrentPos,
                         Chunk);
                    return FALSE;
                }

                Out += Chunk;
                AlignedBytes -= Chunk;
                CurrentPos += Chunk;
            }

            Remaining &= SectorMask;
        }
    }

    /* Handle tail */
    if (Remaining > 0)
    {
        ULONGLONG TailStart;
        ULONGLONG SectorStart;
        ULONG OffsetInSector;

        TailStart = AbsoluteStart + Size - Remaining;
        SectorStart = TailStart & ~((ULONGLONG)SectorMask);
        OffsetInSector = (ULONG)(TailStart & SectorMask);

        if (CurrentPos != SectorStart)
        {
            Position.QuadPart = SectorStart;
            Status = ArcSeek(Source->ArcFileId, &Position, SeekAbsolute);
            if (Status != ESUCCESS)
            {
                WARN("IsoSourceRead: ArcSeek failed (status %lu) at offset %I64u (tail)\n",
                     Status,
                     Position.QuadPart);
                return FALSE;
            }
            CurrentPos = SectorStart;
        }

        Status = ArcRead(Source->ArcFileId, Bounce, ISO_SECTOR_SIZE, &BytesRead);
        if (Status != ESUCCESS || BytesRead != ISO_SECTOR_SIZE)
        {
            WARN("IsoSourceRead: ArcRead failed (status %lu) at offset %I64u (tail)\n",
                 Status,
                 SectorStart);
            return FALSE;
        }

        CurrentPos += ISO_SECTOR_SIZE;
        RtlCopyMemory(Out, Bounce + OffsetInSector, Remaining);
    }

    Source->ArcPosition = CurrentPos;
    return TRUE;
}

static
ARC_STATUS
RamDiskOpenIsoSource(
    _In_ PCSTR FileName,
    _In_opt_ PCSTR DefaultPath,
    _In_ ULONGLONG ImageOffset,
    _In_ ULONGLONG ImageLength,
    _Out_ PISO_SOURCE Source)
{
    ARC_STATUS Status;
    ULONG FileId;
    FILEINFORMATION Information;
    ULONGLONG FileSize;
    ULONGLONG EffectiveLength;
    UCHAR Descriptor[ISO_SECTOR_SIZE];
    LARGE_INTEGER Position;
    ULONG BytesRead;
    BOOLEAN OpenedRawDevice = FALSE;

    if (!Source)
        return EINVAL;

    RtlZeroMemory(Source, sizeof(*Source));
    Source->ArcFileId = INVALID_FILE_ID;

    Status = FsOpenFile((PCHAR)FileName, DefaultPath, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        /* Fall back to opening the ARC device directly (e.g. CD/DVD handle) */
        if (FileName && strchr(FileName, ')'))
        {
            Status = ArcOpen((PCHAR)FileName, OpenReadOnly, &FileId);
            if (Status == ESUCCESS)
            {
                OpenedRawDevice = TRUE;
            }
        }

        if (Status != ESUCCESS)
            return Status;
    }

    Status = ArcGetFileInformation(FileId, &Information);
    if (Status != ESUCCESS)
    {
        ArcClose(FileId);
        return Status;
    }

    FileSize = Information.EndingAddress.QuadPart;

    /*
     * Some firmware/device paths report the raw device capacity here,
     * which for 2KiB-block optical media can be 4x the ISO size when
     * combined with 512-byte sector-based partition metadata. Read the
     * ISO9660 Primary Volume Descriptor to obtain an authoritative size
     * and prefer it when available.
     */
    {
        ULONGLONG PvdOffset = ImageOffset + (ULONGLONG)16 * ISO_SECTOR_SIZE;

        Position.QuadPart = PvdOffset;
        if (ArcSeek(FileId, &Position, SeekAbsolute) == ESUCCESS)
        {
            if (ArcRead(FileId, Descriptor, ISO_SECTOR_SIZE, &BytesRead) == ESUCCESS &&
                BytesRead == ISO_SECTOR_SIZE)
            {
                PPVD Pvd = (PPVD)Descriptor;
                if (Pvd->VdType == 1 &&
                    RtlEqualMemory(Pvd->StandardId, "CD001", 5) &&
                    Pvd->VdVersion == 1)
                {
                    USHORT LogicalBlockSize = Pvd->LogicalBlockSizeL;
                    if (LogicalBlockSize == 0)
                        LogicalBlockSize = ISO_SECTOR_SIZE; /* Fallback to default */

                    /* Compute the ISO byte length from the PVD */
                    ULONGLONG IsoLength = (ULONGLONG)Pvd->VolumeSpaceSizeL * LogicalBlockSize;

                    /* Prefer the PVD-declared size when sensible (never larger than reported file size if nonzero). */
                    if (IsoLength != 0 && (FileSize == 0 || IsoLength <= FileSize))
                        FileSize = IsoLength;
                }
            }
        }

        /* Restore the caller's requested starting position */
        Position.QuadPart = ImageOffset;
        ArcSeek(FileId, &Position, SeekAbsolute);
    }

    if (FileSize == 0 && ImageLength != 0)
    {
        /* Some firmware return 0 for raw devices; use the supplied length as a hint */
        FileSize = ImageOffset + ImageLength;
    }

    if (FileSize != 0 && ImageOffset >= FileSize)
    {
        ArcClose(FileId);
        return EINVAL;
    }

    EffectiveLength = (FileSize != 0) ? (FileSize - ImageOffset) : ImageLength;
    if (FileSize != 0 && ImageLength != 0 && ImageLength < EffectiveLength)
        EffectiveLength = ImageLength;
    if (EffectiveLength == 0)
    {
        ArcClose(FileId);
        return EINVAL;
    }

    Source->MemoryBase = NULL;
    Source->ArcFileId = FileId;
    Source->ArcOffset = ImageOffset;
    Source->Size = EffectiveLength;
    Source->ArcPosition = OpenedRawDevice ? ImageOffset : 0;

    return ESUCCESS;
}

static
VOID
RamDiskCloseIsoSource(
    _Inout_ PISO_SOURCE Source)
{
    if (!Source)
        return;

    if (Source->ArcFileId != INVALID_FILE_ID)
    {
        ArcClose(Source->ArcFileId);
        Source->ArcFileId = INVALID_FILE_ID;
    }
}


static
BOOLEAN
IsoExtractName(
    _In_ PDIR_RECORD Record,
    _Out_writes_(NameBufferSize) PCHAR NameBuffer,
    _In_ SIZE_T NameBufferSize)
{
    SIZE_T Index;

    if (!Record || !NameBuffer || NameBufferSize == 0)
        return FALSE;

    /* Skip '.' and '..' entries early */
    if (Record->FileIdLength == 1 && (Record->FileId[0] == 0 || Record->FileId[0] == 1))
        return FALSE;

    for (Index = 0; Index < Record->FileIdLength && Index < NameBufferSize - 1; ++Index)
    {
        CHAR Character = Record->FileId[Index];

        if (Character == ';')
            break;

        NameBuffer[Index] = Character;
    }

    NameBuffer[Index] = '\0';

    /* Remove trailing dot, if any (appears on directory records) */
    while (Index > 0 && NameBuffer[Index - 1] == '.')
    {
        NameBuffer[Index - 1] = '\0';
        --Index;
    }

    if (Index == 0)
        return FALSE;

    return TRUE;
}

static
BOOLEAN
FatEnsureDirectoryExists(
    _In_ PFAT32_WRITER Writer,
    _In_ PCSTR Path)
{
    if (!Writer || !Path)
        return FALSE;

    return Fat32CreateDirectory(Writer, Path);
}

static
BOOLEAN
FatCopyFileFromIso(
    _In_ PISO_COPY_CONTEXT Context,
    _In_ PDIR_RECORD Record,
    _In_ PCSTR DestinationPath)
{
    ULONGLONG Remaining;
    ULONGLONG FileOffset;
    PUCHAR DataDest;

    if (!Context || !Context->Source || !Record || !DestinationPath)
        return FALSE;

    if (Record->FileFlags & 0x02)
        return FALSE;

    FileOffset = (ULONGLONG)Record->ExtentLocationL * ISO_SECTOR_SIZE;
    Remaining = Record->DataLengthL;

    if (FileOffset + Remaining > Context->Source->Size)
        return FALSE;

    if (Remaining > ULONG_MAX)
    {
        WARN("FatCopyFileFromIso: file '%s' too large (%llu bytes)\n",
             DestinationPath, Remaining);
        return FALSE;
    }

    if (!IsoEnsureScratchBuffer(Context, Remaining))
    {
        WARN("IsoEnsureScratchBuffer failed for '%s' size %lu\n",
             DestinationPath,
             Record->DataLengthL);
        return FALSE;
    }

    /* Allocate clusters and create directory entry in one call */
    if (!Fat32CreateFileEx(&Context->Writer, DestinationPath,
                           (ULONG)Remaining, &DataDest))
    {
        WARN("Fat32CreateFileEx failed for '%s'\n", DestinationPath);

        if (strstr(DestinationPath, "/efi/boot/BCD"))
        {
            WARN("FatCopyFileFromIso: skipping '%s' on legacy BIOS path\n",
                 DestinationPath);
            return TRUE;
        }

        return FALSE;
    }

    /* Copy file data in chunks from ISO source directly into cluster memory */
    while (Remaining > 0)
    {
        ULONG Chunk = (Remaining > Context->ScratchBufferSize)
                        ? Context->ScratchBufferSize
                        : (ULONG)Remaining;

        if (!IsoSourceRead(Context->Source, FileOffset, DataDest, Chunk))
        {
            WARN("IsoSourceRead failed while copying '%s' at offset %I64u len %lu\n",
                 DestinationPath,
                 FileOffset,
                 Chunk);
            return FALSE;
        }

        DataDest += Chunk;
        FileOffset += Chunk;
        Remaining -= Chunk;

        IsoProgressAdvance(Context, Chunk);
    }

    return TRUE;
}

static
BOOLEAN
IsoCopyDirectoryRecursive(
    _In_ PISO_COPY_CONTEXT Context,
    _In_ ULONG StartSector,
    _In_ ULONG DirectoryLength,
    _In_ PCSTR DestinationPath)
{
    ULONGLONG DirectoryOffset;
    ULONG Offset = 0;
    PUCHAR DirectoryBuffer = NULL;
    PCHAR NameBuffer = NULL;
    PCHAR ChildPath = NULL;
    SIZE_T ChildPathCapacity = 0;
    BOOLEAN UseSharedBuffer = FALSE;
    BOOLEAN AllocatedBuffer = FALSE;
    BOOLEAN Result = FALSE;

    if (!Context || !Context->Source || !DestinationPath)
        return FALSE;

    DirectoryOffset = (ULONGLONG)StartSector * ISO_SECTOR_SIZE;

    if (DirectoryOffset >= Context->Source->Size ||
        DirectoryOffset + DirectoryLength > Context->Source->Size)
        return FALSE;

    if (DirectoryLength > ISO_DIRECTORY_MAX_SIZE)
    {
        WARN("IsoCopyDirectoryRecursive: directory length %lu exceeds safety cap\n",
             DirectoryLength);
        return FALSE;
    }

    if (!Context->DirectoryBufferBusy)
    {
        if (!Context->DirectoryBuffer ||
            Context->DirectoryBufferSize < DirectoryLength)
        {
            if (Context->DirectoryBuffer)
            {
                FrLdrTempFree(Context->DirectoryBuffer, TAG_ISO_BUFFER);
                Context->DirectoryBuffer = NULL;
                Context->DirectoryBufferSize = 0;
            }

            Context->DirectoryBuffer = FrLdrTempAlloc(DirectoryLength, TAG_ISO_BUFFER);
            if (!Context->DirectoryBuffer)
                return FALSE;

            Context->DirectoryBufferSize = DirectoryLength;
        }

        DirectoryBuffer = Context->DirectoryBuffer;
        Context->DirectoryBufferBusy = TRUE;
        UseSharedBuffer = TRUE;
    }
    else
    {
        DirectoryBuffer = FrLdrTempAlloc(DirectoryLength, TAG_ISO_BUFFER);
        if (!DirectoryBuffer)
            return FALSE;

        AllocatedBuffer = TRUE;
    }

    NameBuffer = FrLdrTempAlloc(ISO_NAME_BUFFER_SIZE, TAG_ISO_BUFFER);
    if (!NameBuffer)
        goto Cleanup;

    if (!IsoSourceRead(Context->Source, DirectoryOffset, DirectoryBuffer, DirectoryLength))
    {
        WARN("IsoSourceRead failed reading directory at sector %lu length %lu\n",
             StartSector,
             DirectoryLength);
        goto Cleanup;
    }

    TRACE("IsoCopyDirectoryRecursive: sector %lu length %lu -> %s\n",
          StartSector,
          DirectoryLength,
          DestinationPath);

    while (Offset < DirectoryLength)
    {
        PDIR_RECORD Record;
        ULONG NextOffset;
        BOOLEAN IsDirectory;

        /* Ensure at least the fixed portion of a directory record fits */
        if (Offset + FIELD_OFFSET(DIR_RECORD, FileId) > DirectoryLength)
            break;

        Record = (PDIR_RECORD)(DirectoryBuffer + Offset);

        if (Record->RecordLength == 0)
        {
            Offset = ROUND_UP(Offset, ISO_SECTOR_SIZE);
            continue;
        }

        /* Validate that the full record (including FileId) fits in the buffer */
        if (Record->RecordLength < FIELD_OFFSET(DIR_RECORD, FileId) ||
            Offset + Record->RecordLength > DirectoryLength)
        {
            WARN("IsoCopyDirectoryRecursive: invalid RecordLength %u at offset %lu\n",
                 Record->RecordLength, Offset);
            break;
        }

        NextOffset = Offset + Record->RecordLength;

        IsDirectory = !!(Record->FileFlags & 0x02);
        if (!IsoExtractName(Record, NameBuffer, ISO_NAME_BUFFER_SIZE))
        {
            Offset = NextOffset;
            continue;
        }

        {
            size_t Need = strlen(DestinationPath) + 1 + strlen(NameBuffer) + 1;

            if (Need > ChildPathCapacity)
            {
                PCHAR NewChildPath = FrLdrTempAlloc(Need, TAG_ISO_BUFFER);
                if (!NewChildPath)
                {
                    WARN("IsoCopyDirectoryRecursive: unable to allocate %zu bytes for '%s/%s'\n",
                         Need,
                         DestinationPath,
                         NameBuffer);
                    goto Cleanup;
                }

                if (ChildPath)
                    FrLdrTempFree(ChildPath, TAG_ISO_BUFFER);

                ChildPath = NewChildPath;
                ChildPathCapacity = Need;
            }

            if (!NT_SUCCESS(RtlStringCbPrintfA(ChildPath,
                                               ChildPathCapacity,
                                               "%s/%s",
                                               DestinationPath,
                                               NameBuffer)))
            {
                WARN("RtlStringCbPrintfA failed while building path '%s/%s'\n",
                     DestinationPath,
                     NameBuffer);
                goto Cleanup;
            }
        }

#ifndef UEFIBOOT
        {
            /* Skip EFI boot paths on legacy BIOS */
            PCSTR p = ChildPath;
            while (*p == '/') p++;
            if (strlen(p) >= 3 &&
                (p[0] == 'e' || p[0] == 'E') &&
                (p[1] == 'f' || p[1] == 'F') &&
                (p[2] == 'i' || p[2] == 'I') &&
                (p[3] == '/' || p[3] == '\0'))
            {
                TRACE("IsoCopyDirectoryRecursive: skipping legacy-only path %s\n", ChildPath);
                Offset = NextOffset;
                continue;
            }
        }
#endif

        if (IsDirectory)
        {
            if (!FatEnsureDirectoryExists(&Context->Writer, ChildPath))
            {
                goto Cleanup;
            }

            if (!IsoCopyDirectoryRecursive(Context,
                                           Record->ExtentLocationL,
                                           Record->DataLengthL,
                                           ChildPath))
            {
                WARN("IsoCopyDirectoryRecursive failed for '%s' (sector %lu length %lu)\n",
                     ChildPath,
                     Record->ExtentLocationL,
                     Record->DataLengthL);
                goto Cleanup;
            }
        }
        else
        {
            // TRACE("IsoCopyDirectoryRecursive: copying file %s (%lu bytes, %lu KB)\n",
            //       ChildPath,
            //       Record->DataLengthL,
            //       (Record->DataLengthL + 1023UL) / 1024UL);
            if (!FatCopyFileFromIso(Context, Record, ChildPath))
            {
                WARN("FatCopyFileFromIso failed for '%s'\n", ChildPath);
                goto Cleanup;
            }

        }

        Offset = NextOffset;
    }

    Result = TRUE;

Cleanup:
    if (ChildPath)
        FrLdrTempFree(ChildPath, TAG_ISO_BUFFER);

    if (NameBuffer)
        FrLdrTempFree(NameBuffer, TAG_ISO_BUFFER);

    if (UseSharedBuffer)
    {
        Context->DirectoryBufferBusy = FALSE;
    }
    else if (AllocatedBuffer && DirectoryBuffer)
    {
        FrLdrTempFree(DirectoryBuffer, TAG_ISO_BUFFER);
    }

    return Result;
}

static
BOOLEAN
RamDiskPopulateFatFromIso(
    _In_ PVOID FatBase,
    _In_ ULONGLONG FatSize,
    _Inout_ PISO_SOURCE Source,
    _In_ PRAMDISK_FAT32_LAYOUT Layout)
{
    ISO_COPY_CONTEXT Context;
    PPVD PrimaryVolumeDescriptor;
    UCHAR DescriptorBuffer[ISO_SECTOR_SIZE];

    if (!FatBase || !Source || Source->Size < (16 * ISO_SECTOR_SIZE) || !Layout)
        return FALSE;

    if (!IsoSourceRead(Source,
                       (ULONGLONG)16 * ISO_SECTOR_SIZE,
                       DescriptorBuffer,
                       sizeof(DescriptorBuffer)))
        return FALSE;

    PrimaryVolumeDescriptor = (PPVD)DescriptorBuffer;

    if (PrimaryVolumeDescriptor->VdType != 1 ||
        !RtlEqualMemory(PrimaryVolumeDescriptor->StandardId, "CD001", 5) ||
        PrimaryVolumeDescriptor->VdVersion != 1)
    {
        return FALSE;
    }

    TRACE("RamDiskPopulateFatFromIso: FAT size %llu bytes, ISO size %llu bytes\n",
          FatSize,
          Source->Size);

    RtlZeroMemory(&Context, sizeof(Context));
    Context.Source = Source;

    if (!Fat32WriterInit(&Context.Writer, FatBase, FatSize, Layout))
    {
        WARN("RamDiskPopulateFatFromIso: Fat32WriterInit failed\n");
        return FALSE;
    }

    IsoProgressInitialize(&Context);

    TRACE("RamDiskPopulateFatFromIso: traversing ISO root (sector %lu size %lu)\n",
          PrimaryVolumeDescriptor->RootDirRecord.ExtentLocationL,
          PrimaryVolumeDescriptor->RootDirRecord.DataLengthL);

    if (!IsoCopyDirectoryRecursive(&Context,
                                   PrimaryVolumeDescriptor->RootDirRecord.ExtentLocationL,
                                   PrimaryVolumeDescriptor->RootDirRecord.DataLengthL,
                                   ""))
    {
        if (Context.ScratchBuffer)
            FrLdrTempFree(Context.ScratchBuffer, TAG_ISO_BUFFER);
        if (Context.DirectoryBuffer)
            FrLdrTempFree(Context.DirectoryBuffer, TAG_ISO_BUFFER);
        return FALSE;
    }

    IsoProgressComplete(&Context);

    if (Context.ScratchBuffer)
        FrLdrTempFree(Context.ScratchBuffer, TAG_ISO_BUFFER);
    if (Context.DirectoryBuffer)
        FrLdrTempFree(Context.DirectoryBuffer, TAG_ISO_BUFFER);

    TRACE("RamDiskPopulateFatFromIso: copy complete\n");
    return TRUE;
}

BOOLEAN
RamDiskBuildWritableImage(
    IN PISO_SOURCE Source,
    IN ULONGLONG RequestedSize,
    OUT PVOID *NewBase,
    OUT PULONGLONG NewSize,
    IN BOOLEAN OptionalRamDisk)
{
    PVOID WritableBase = NULL;
    ULONGLONG WritableSize = 0;
    ULONGLONG RequiredSize;
    ULONGLONG IsoSize;
    ULONGLONG ResidentIsoBytes = 0;
    RAMDISK_FAT32_LAYOUT Layout;

    if (!Source || !NewBase || !NewSize || Source->Size == 0)
        return FALSE;

    IsoSize = Source->Size;

    /* Leave some slack to account for ISO9660 metadata and future writes */
    RequiredSize = RequestedSize;
    if (RequiredSize < IsoSize + RAMDISK_MINIMUM_EXTRA_SPACE)
        RequiredSize = IsoSize + RAMDISK_MINIMUM_EXTRA_SPACE;

    if (RequiredSize + RAMDISK_SAFETY_SLACK > RAMDISK_LOW_ALLOC_MAX)
    {
        WARN("RamDiskBuildWritableImage: requested size %llu exceeds low-memory limit %llu\n",
             RequiredSize,
             RAMDISK_LOW_ALLOC_MAX);
        if (!RamDiskErrorShown && !OptionalRamDisk)
        {
            UiMessageBox("Requested writable RAM disk size exceeds available low memory.");
            RamDiskErrorShown = TRUE;
        }
        return FALSE;
    }

    if (Source->MemoryBase)
        ResidentIsoBytes = ALIGN_UP_BY_ULL(IsoSize, RAMDISK_ALLOCATION_ALIGNMENT);

    if ((RequiredSize + ResidentIsoBytes + RAMDISK_SAFETY_SLACK) > RAMDISK_LOW_ALLOC_MAX)
    {
        WARN("RamDiskBuildWritableImage: %llu-byte ISO plus %llu-byte writable buffer exceed low-memory budget %llu\n",
             IsoSize,
             RequiredSize,
             RAMDISK_LOW_ALLOC_MAX);
        if (!RamDiskErrorShown && !OptionalRamDisk)
        {
            UiMessageBox("Writable RAM disk request uses too much low memory to keep the ISO resident.");
            RamDiskErrorShown = TRUE;
        }
        return FALSE;
    }

    RequiredSize = ALIGN_UP_BY_ULL(RequiredSize, RAMDISK_ALLOCATION_ALIGNMENT);
    if (RequiredSize == 0 || RequiredSize > MAXULONG)
        return FALSE;

    TRACE("RamDiskBuildWritableImage: ISO=%llu requested=%llu align=%llu\n",
          IsoSize,
          RequestedSize,
          RequiredSize);

    if (!RamDiskReserveWritableBuffer(RequiredSize, OptionalRamDisk))
        return FALSE;

    if (!RamDiskGetReservedBuffer(RequiredSize, &WritableBase, &WritableSize))
        return FALSE;

    if (WritableSize > MAXULONG)
        WritableSize = MAXULONG;

    TRACE("RamDiskBuildWritableImage: formatting FAT32 (%llu bytes)\n", WritableSize);

    if (!RamDiskFormatFat32(WritableBase, WritableSize, &Layout))
    {
        MmFreeMemory(WritableBase);
        return FALSE;
    }

    TRACE("RamDiskBuildWritableImage: populating ramdisk from ISO\n");

    {
        ULONGLONG VolumeOffset;
        PVOID VolumeBase;
        ULONGLONG VolumeSize;

        VolumeOffset = (ULONGLONG)Layout.HiddenSectors * Layout.BytesPerSector;
        if (VolumeOffset >= WritableSize)
        {
            MmFreeMemory(WritableBase);
            return FALSE;
        }

        VolumeBase = (PUCHAR)WritableBase + VolumeOffset;
        VolumeSize = WritableSize - VolumeOffset;

        if (!RamDiskPopulateFatFromIso(VolumeBase,
                                       VolumeSize,
                                       Source,
                                       &Layout))
        {
            MmFreeMemory(WritableBase);
            return FALSE;
        }

        RamDiskSetVisibleRegion(VolumeOffset, VolumeSize);

#if DBG
        {
            PFN_NUMBER BasePfn = RamDiskPointerToPfn(WritableBase);
            PFN_NUMBER EndPfn = RamDiskPointerToPfn((const VOID *)((ULONG_PTR)WritableBase + (ULONG_PTR)(WritableSize - 1)));
            PVOID VolumePointer = (PUCHAR)WritableBase + VolumeOffset;
            TRACE("RamDiskBuildWritableImage: PFN span %lx-%lx hidden=%lu reserved=%lu fat=%lu data=%lu\n",
                  BasePfn,
                  EndPfn,
                  Layout.HiddenSectors,
                  Layout.ReservedSectors,
                  Layout.FatSizeSectors,
                  Layout.FirstDataSector);
            RamDiskTraceSample("  writable[boot]", WritableBase);
            RamDiskTraceSample("  writable[volume]", VolumePointer);

            if (Layout.ReservedSectors < WritableSize / Layout.BytesPerSector)
            {
                ULONGLONG FatOffset = VolumeOffset + ((ULONGLONG)Layout.ReservedSectors * Layout.BytesPerSector);
                if (FatOffset + (16 * sizeof(ULONG)) <= WritableSize)
                {
                    const ULONG *FatWords = (const ULONG *)((PUCHAR)WritableBase + FatOffset);
                    TRACE("  writable[FAT0..7]=%08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX\n",
                          FatWords[0], FatWords[1], FatWords[2], FatWords[3],
                          FatWords[4], FatWords[5], FatWords[6], FatWords[7]);
                }
            }

            if (Source->MemoryBase)
            {
                PFN_NUMBER SourceBasePfn = RamDiskPointerToPfn(Source->MemoryBase);
                TRACE("  source: base=%p size=%llu basePFN=%lx\n",
                      Source->MemoryBase,
                      Source->Size,
                      SourceBasePfn);
                RamDiskTraceSample("  source[0]", Source->MemoryBase);
            }
        }
#endif
    }

    *NewBase = WritableBase;
    *NewSize = WritableSize;
    TRACE("RamDiskBuildWritableImage: writable ramdisk ready at %p (%llu bytes)\n",
          WritableBase,
          WritableSize);
    return TRUE;
}

static
ULONGLONG
RamDiskParseSizeString(
    PCSTR ValueString,
    ULONG ValueLength)
{
    ULONGLONG Value = 0;
    ULONGLONG Multiplier = 1;
    BOOLEAN SawDigit = FALSE;
    ULONG Index = 0;

    if (!ValueString || ValueLength == 0)
        return 0;

    /* Skip leading whitespace */
    while (Index < ValueLength && isspace((unsigned char)ValueString[Index]))
        ++Index;

    /* Parse the numeric component */
    while (Index < ValueLength && isdigit((unsigned char)ValueString[Index]))
    {
        int Digit = ValueString[Index] - '0';

        if (Value > (ULLONG_MAX - Digit) / 10ULL)
            return 0;

        Value = Value * 10ULL + (ULONGLONG)Digit;
        SawDigit = TRUE;
        ++Index;
    }

    if (!SawDigit)
        return 0;

    /* Skip any whitespace between the number and the optional suffix */
    while (Index < ValueLength && isspace((unsigned char)ValueString[Index]))
        ++Index;

    if (Index < ValueLength)
    {
        char Suffix = (char)toupper((unsigned char)ValueString[Index]);

        switch (Suffix)
        {
            case 'B':
                Multiplier = 1ULL;
                ++Index;
                break;

            case 'K':
                Multiplier = 1024ULL;
                ++Index;
                break;

            case 'M':
                Multiplier = 1024ULL * 1024ULL;
                ++Index;
                break;

            case 'G':
                Multiplier = 1024ULL * 1024ULL * 1024ULL;
                ++Index;
                break;

            case 'T':
                Multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
                ++Index;
                break;

            default:
                return 0;
        }

        /* Optional trailing 'B' (e.g. "MB", "GiB") */
        if (Index < ValueLength)
        {
            char SecondSuffix = (char)toupper((unsigned char)ValueString[Index]);

            if (SecondSuffix == 'I')
            {
                /* Accept IEC-style suffixes like MiB/GiB */
                ++Index;
                if (Index < ValueLength)
                {
                    SecondSuffix = (char)toupper((unsigned char)ValueString[Index]);
                }
                else
                {
                    SecondSuffix = '\0';
                }
            }

            if (SecondSuffix == 'B')
            {
                ++Index;
            }
        }

        while (Index < ValueLength && isspace((unsigned char)ValueString[Index]))
            ++Index;

        /* Reject unknown trailing characters */
        if (Index < ValueLength)
            return 0;

        if (Multiplier != 1ULL && Value > ULLONG_MAX / Multiplier)
            return 0;

        Value *= Multiplier;
    }

    return Value;
}

ULONGLONG
RamDiskGetRequestedSize(VOID)
{
    return RamDiskRequestedSize;
}

ULONGLONG
RamDiskGetImageLength(VOID)
{
    return RamDiskImageLength;
}

ULONG
RamDiskGetImageOffset(VOID)
{
    return RamDiskImageOffset;
}

ULONGLONG
RamDiskGetVolumeOffset(VOID)
{
    return RamDiskVolumeOffset;
}

#if defined(__GNUC__)
__attribute__((unused))
#endif
static
BOOLEAN
RamDiskReserveWritableBuffer(ULONGLONG RequestedSize, BOOLEAN OptionalRamDisk)
{
    ULONGLONG AllocationSize;
    PVOID Base;
    ULONGLONG AllocationLimit;

    if (RequestedSize == 0)
        return FALSE;

    AllocationSize = ALIGN_UP_BY_ULL(RequestedSize, RAMDISK_ALLOCATION_ALIGNMENT);
    if (AllocationSize == 0 || AllocationSize < RequestedSize)
        return FALSE;

    AllocationLimit = RamDiskWritableAllocationLimit();

    if (AllocationSize > AllocationLimit)
    {
        WARN("Requested ramdisk buffer %llu bytes exceeds low-memory limit %llu bytes\n",
             AllocationSize,
             AllocationLimit);
        if (!RamDiskErrorShown && !OptionalRamDisk)
        {
            UiMessageBox("Requested writable RAM disk size exceeds available low memory.");
            RamDiskErrorShown = TRUE;
        }
        return FALSE;
    }

    if (RamDiskWritableBase &&
        RamDiskWritableSize >= AllocationSize &&
        ((ULONGLONG)(ULONG_PTR)RamDiskWritableBase + AllocationSize) <= AllocationLimit)
        return TRUE;

    if (RamDiskWritableBase)
    {
        MmFreeMemory(RamDiskWritableBase);
        RamDiskWritableBase = NULL;
        RamDiskWritableSize = 0;
    }

    if ((ULONGLONG)(SIZE_T)AllocationSize != AllocationSize)
    {
        WARN("Requested ramdisk size (%llu) exceeds allocator limits\n", AllocationSize);
        return FALSE;
    }

    /*
     * Allocate ramdisk memory as LoaderXIPRom.
     *
     * Using LoaderXIPRom ensures the ramdisk descriptor is uniquely identifiable
     * by the kernel's IopStartRamdisk function. Other allocations use LoaderMemoryData,
     * which can cause descriptor coalescing and misidentification when adjacent
     * LoaderMemoryData regions exist.
     *
     * The kernel's memory manager will mark LoaderXIPRom pages as ROM, preventing
     * reclamation.
     */
    Base = MmAllocateHighestMemoryBelowAddress((SIZE_T)AllocationSize,
                                               (PVOID)(ULONG_PTR)AllocationLimit,
                                               LoaderXIPRom);
    if (!Base)
    {
        WARN("Failed to reserve writable ramdisk buffer (%llu bytes)\n", AllocationSize);
        if (!RamDiskErrorShown && !OptionalRamDisk)
        {
            UiMessageBox("Unable to allocate low-memory buffer for writable RAM disk.");
            RamDiskErrorShown = TRUE;
        }
        return FALSE;
    }

    if (((ULONGLONG)(ULONG_PTR)Base + AllocationSize) > AllocationLimit)
    {
        WARN("Writable ramdisk buffer %p-%p exceeds limit %p\n",
             Base,
             (PVOID)(ULONG_PTR)((ULONG_PTR)Base + (ULONG_PTR)AllocationSize),
             (PVOID)(ULONG_PTR)AllocationLimit);
        MmFreeMemory(Base);
        if (!RamDiskErrorShown && !OptionalRamDisk)
        {
            UiMessageBox("Unable to allocate low-memory buffer for writable RAM disk.");
            RamDiskErrorShown = TRUE;
        }
        return FALSE;
    }

    RamDiskWritableBase = Base;
    RamDiskWritableSize = AllocationSize;
    TRACE("Reserved writable ramdisk buffer at %p (%llu bytes)\n", Base, AllocationSize);
    return TRUE;
}

BOOLEAN
RamDiskGetReservedBuffer(
    IN ULONGLONG MinimumSize,
    OUT PVOID *BaseAddress,
    OUT PULONGLONG ActualSize)
{
    if (!BaseAddress || !ActualSize)
        return FALSE;

    if (RamDiskWritableBase && RamDiskWritableSize >= MinimumSize)
    {
        *BaseAddress = RamDiskWritableBase;
        *ActualSize = RamDiskWritableSize;

        RamDiskWritableBase = NULL;
        RamDiskWritableSize = 0;
        return TRUE;
    }

    return FALSE;
}

/*
 * RamDiskGetBackingStore - Get ramdisk backing store information for kernel.
 *
 * Returns the base page and page count of the ramdisk backing store.
 * This information is used by the kernel's memory manager to mark these
 * pages as non-reclaimable (ROM) in the PFN database.
 */
BOOLEAN
RamDiskGetBackingStore(
    OUT PFN_NUMBER *BasePage,
    OUT PFN_NUMBER *PageCount)
{
    ULONGLONG TotalSize;
    PFN_NUMBER Pages;

    if (!BasePage || !PageCount)
        return FALSE;

    /* Check if ramdisk is active */
    if (!RamDiskBase || RamDiskFileSize == 0)
    {
        *BasePage = 0;
        *PageCount = 0;
        return FALSE;
    }

    /* Calculate the base page and page count */
    *BasePage = (PFN_NUMBER)((ULONG_PTR)RamDiskBase >> PAGE_SHIFT);

    /* Calculate total size including image offset for proper page coverage */
    TotalSize = RamDiskFileSize;
    Pages = (PFN_NUMBER)((TotalSize + PAGE_SIZE - 1) >> PAGE_SHIFT);
    *PageCount = Pages;

    TRACE("RamDiskGetBackingStore: Base=%p BasePage=%lu PageCount=%lu Size=%llu\n",
          RamDiskBase, (ULONG)*BasePage, (ULONG)*PageCount, TotalSize);

    return TRUE;
}

/* FUNCTIONS ******************************************************************/

static ULONGLONG RamDiskGetVisibleLength(VOID)
{
    return (RamDiskVolumeLength != 0)
           ? RamDiskVolumeLength
           : (RamDiskImageLength > RamDiskVolumeOffset)
               ? RamDiskImageLength - RamDiskVolumeOffset
               : 0;
}

static ARC_STATUS RamDiskClose(ULONG FileId)
{
    /* Nothing to do */
    return ESUCCESS;
}

static ARC_STATUS RamDiskGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    ULONGLONG VisibleLength;

    RtlZeroMemory(Information, sizeof(*Information));
    VisibleLength = RamDiskGetVisibleLength();

    Information->EndingAddress.QuadPart = VisibleLength;
    Information->CurrentAddress.QuadPart = RamDiskOffset;

    return ESUCCESS;
}

static ARC_STATUS RamDiskOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    /* Always return success, as contents are already in memory */
    return ESUCCESS;
}

static ARC_STATUS RamDiskRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PVOID StartAddress;
    ULONGLONG VisibleLength;

    /* Don't allow reads past our image */
    VisibleLength = RamDiskGetVisibleLength();

    if ((RamDiskOffset >= VisibleLength) || (RamDiskOffset + N > VisibleLength))
    {
        *Count = 0;
        return EIO;
    }

    /* Get actual pointer without truncating offsets on 32-bit builds. */
    {
        ULONGLONG TotalOffset = RamDiskVolumeOffset + RamDiskOffset;
        ULONG_PTR BaseAddress = (ULONG_PTR)RamDiskBase;
        ULONGLONG MaxOffset = ((ULONGLONG)~(ULONG_PTR)0);

        if (TotalOffset > (MaxOffset - (ULONGLONG)BaseAddress))
        {
            WARN("RamDiskRead: offset overflow (total=%I64u base=%p)\n", TotalOffset, RamDiskBase);
            *Count = 0;
            return EIO;
        }

        StartAddress = (PVOID)(BaseAddress + (ULONG_PTR)TotalOffset);
    }

    /* Do the read */
    RtlCopyMemory(Buffer, StartAddress, N);
    RamDiskOffset += N;
    *Count = N;

    return ESUCCESS;
}

static ARC_STATUS RamDiskSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    LARGE_INTEGER NewPosition = *Position;
    ULONGLONG VisibleLength;

    switch (SeekMode)
    {
        case SeekAbsolute:
            break;
        case SeekRelative:
            NewPosition.QuadPart += RamDiskOffset;
            break;
        default:
            ASSERT(FALSE);
            return EINVAL;
    }

    if (NewPosition.QuadPart < 0)
        return EINVAL;

    VisibleLength = RamDiskGetVisibleLength();

    if ((ULONGLONG)NewPosition.QuadPart > VisibleLength)
        return EINVAL;

    RamDiskOffset = NewPosition.QuadPart;
    return ESUCCESS;
}

static const DEVVTBL RamDiskVtbl =
{
    RamDiskClose,
    RamDiskGetFileInformation,
    RamDiskOpen,
    RamDiskRead,
    RamDiskSeek,
};

static ARC_STATUS
RamDiskLoadVirtualFile(
    IN PCSTR FileName,
    IN PCSTR DefaultPath OPTIONAL,
    IN BOOLEAN OptionalRamDisk)
{
    ARC_STATUS Status;
    ULONG RamFileId;
    ULONG ChunkSize, Count;
    ULONGLONG TotalRead;
    ULONG LastPercent;
    FILEINFORMATION Information;
    LARGE_INTEGER Position;

    /* Display progress */
    UiDrawProgressBarCenter("Loading RamDisk...");

    /*
     * If the firmware or a previous boot stage already provided the ramdisk
     * image in memory, skip the expensive readback and reuse the cached data.
     */
    if (gInitRamDiskBase && gInitRamDiskSize != 0)
    {
        BOOLEAN UseResidentImage = FALSE;
        ULONGLONG ResidentSize = (ULONGLONG)gInitRamDiskSize;

        if ((ULONGLONG)RamDiskImageOffset < ResidentSize)
        {
            ULONGLONG Available = ResidentSize - (ULONGLONG)RamDiskImageOffset;
            ULONGLONG Required = (RamDiskImageLength != 0)
                                  ? RamDiskImageLength
                                  : Available;

            if (Required <= Available)
                UseResidentImage = TRUE;
            else
                WARN("RamDiskLoadVirtualFile: resident image too small (offset=%lu required=%llu available=%llu)\n",
                     (ULONG)RamDiskImageOffset,
                     Required,
                     Available);
        }

        if (UseResidentImage)
        {
            RamDiskBase = gInitRamDiskBase;
            RamDiskFileSize = ResidentSize;
            RamDiskImageOffset = 0;
            RamDiskImageLength = RamDiskFileSize;
            RamDiskResetVisibleRegion();
            UiUpdateProgressBar(100, NULL);
            TRACE("RamDiskLoadVirtualFile: using resident ramdisk image (%llu bytes)\n",
                  ResidentSize);
            return ESUCCESS;
        }
    }

    /* Try opening the Ramdisk file */
    Status = FsOpenFile(FileName, DefaultPath, OpenReadOnly, &RamFileId);
    if (Status != ESUCCESS)
        return Status;

    /* Get the file size */
    Status = ArcGetFileInformation(RamFileId, &Information);
    if (Status != ESUCCESS)
    {
        ArcClose(RamFileId);
        return Status;
    }

    /* Enforce the legacy 4GB limit on 32-bit builds */
#if !defined(_M_AMD64) && !defined(__x86_64__)
    if (Information.EndingAddress.HighPart != 0)
    {
        ArcClose(RamFileId);
        if (!OptionalRamDisk)
            UiMessageBox("RAM disk too big.");
        return ENOMEM;
    }
#endif

    RamDiskFileSize = Information.EndingAddress.QuadPart;
#if !defined(_M_AMD64) && !defined(__x86_64__)
    ASSERT(RamDiskFileSize < 0x100000000); // Legacy limit on 32-bit builds.
#endif

    /* Allocate memory for it */
    ChunkSize = 8 * 1024 * 1024;
    if (DiskReadBufferSize != 0 && DiskReadBufferSize <= ULONG_MAX)
    {
        ULONG PreferredChunk = (ULONG)DiskReadBufferSize;
        if (PreferredChunk > ChunkSize)
            ChunkSize = PreferredChunk;
    }

    if (RamDiskFileSize < ChunkSize && RamDiskFileSize <= ULONG_MAX)
        ChunkSize = (ULONG)RamDiskFileSize;

    ChunkSize &= ~(ISO_SECTOR_SIZE - 1);
    if (ChunkSize == 0)
        ChunkSize = ISO_SECTOR_SIZE;

#if defined(_M_AMD64) || defined(__x86_64__)
    /* Use LoaderXIPRom for unique identification by IopStartRamdisk */
    RamDiskBase = MmAllocateMemoryWithType(RamDiskFileSize, LoaderXIPRom);
    if (!RamDiskBase)
    {
        RamDiskFileSize = 0;
        ArcClose(RamFileId);
        if (!OptionalRamDisk)
            UiMessageBox("Failed to allocate memory for RAM disk.");
        return ENOMEM;
    }
#else
    {
        ULONGLONG AllocationLimit = RamDiskWritableAllocationLimit();

        if (RamDiskFileSize > AllocationLimit)
        {
            RamDiskFileSize = 0;
            ArcClose(RamFileId);
            if (!OptionalRamDisk)
                UiMessageBox("RAM disk image is larger than available low memory.");
            return ENOMEM;
        }

        /* Use LoaderXIPRom for unique identification by IopStartRamdisk */
        RamDiskBase = MmAllocateHighestMemoryBelowAddress((SIZE_T)RamDiskFileSize,
                                                          (PVOID)(ULONG_PTR)AllocationLimit,
                                                          LoaderXIPRom);
        if (!RamDiskBase)
        {
            RamDiskFileSize = 0;
            ArcClose(RamFileId);
            if (!OptionalRamDisk)
                UiMessageBox("Failed to allocate low memory for RAM disk.");
            return ENOMEM;
        }
    }
#endif

    Position.QuadPart = 0;
    Status = ArcSeek(RamFileId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        MmFreeMemory(RamDiskBase);
        RamDiskBase = NULL;
        RamDiskFileSize = 0;
        ArcClose(RamFileId);
        if (!OptionalRamDisk)
            UiMessageBox("Failed to read RAM disk.");
        return Status;
    }

    /*
     * Read it in chunks
     */
    LastPercent = 0;
    for (TotalRead = 0; TotalRead < RamDiskFileSize; )
    {
        ULONG CurrentChunk = ChunkSize;

        /* Check if we're at the last chunk */
        if ((RamDiskFileSize - TotalRead) < CurrentChunk)
        {
            /* Only need the actual data required */
            CurrentChunk = (ULONG)(RamDiskFileSize - TotalRead);
        }

        if (CurrentChunk == 0)
            break;

        /* Update progress no more than once per percent change */
        if (RamDiskFileSize != 0)
        {
            ULONGLONG Completed = TotalRead + CurrentChunk;
            ULONG NewPercent = (ULONG)((Completed * 100ULL) / RamDiskFileSize);
            if (NewPercent > 100)
                NewPercent = 100;

            if ((NewPercent >= LastPercent + 1) ||
                (NewPercent == 100 && NewPercent != LastPercent))
            {
                UiUpdateProgressBar(NewPercent, NULL);
                LastPercent = NewPercent;
            }
        }

        /* Copy the contents */
        Status = ArcRead(RamFileId,
                         (PVOID)((ULONG_PTR)RamDiskBase + (ULONG_PTR)TotalRead),
                         CurrentChunk,
                         &Count);

        /* Check for success */
        if ((Status != ESUCCESS) || (Count != CurrentChunk))
        {
            MmFreeMemory(RamDiskBase);
            RamDiskBase = NULL;
            RamDiskFileSize = 0;
            ArcClose(RamFileId);
            if (!OptionalRamDisk)
                UiMessageBox("Failed to read RAM disk.");
            return ((Status != ESUCCESS) ? Status : EIO);
        }

        TotalRead += CurrentChunk;
    }

    if (LastPercent < 100)
        UiUpdateProgressBar(100, NULL);

    ArcClose(RamFileId);

    return ESUCCESS;
}

ARC_STATUS
RamDiskInitialize(
    IN BOOLEAN InitRamDisk,
    IN PCSTR LoadOptions OPTIONAL,
    IN PCSTR DefaultPath OPTIONAL)
{
    RamDiskErrorShown = FALSE;

    TRACE("RamDiskInitialize: Begin (Init=%s)\n", InitRamDisk ? "true" : "false");

    /* Reset the RAMDISK device */
    if (RamDiskBase && RamDiskBase != gInitRamDiskBase)
    {
        /* This is not the initial Ramdisk, so we can free the allocated memory */
        MmFreeMemory(RamDiskBase);
    }
    RamDiskBase = NULL;
    RamDiskFileSize = 0;
    RamDiskImageLength = 0;
    RamDiskImageOffset = 0;
    RamDiskOffset = 0;
    RamDiskRequestedSize = 0;
    RamDiskVolumeOffset = 0;
    RamDiskVolumeLength = 0;

    if (InitRamDisk)
    {
        /* We initialize the initial Ramdisk: it should be present in memory */
        if (!gInitRamDiskBase || gInitRamDiskSize == 0)
            return ENODEV;

        // TODO: Handle SDI image.

        RamDiskBase = gInitRamDiskBase;
        RamDiskFileSize = gInitRamDiskSize;
        ASSERT(RamDiskFileSize < 0x100000000); // See FIXME about 4GB support in RamDiskLoadVirtualFile().

        if ((ULONGLONG)RamDiskImageOffset >= RamDiskFileSize)
            RamDiskImageOffset = 0;

        if (RamDiskImageLength == 0 ||
            RamDiskImageLength > RamDiskFileSize - RamDiskImageOffset)
        {
            RamDiskImageLength = RamDiskFileSize - RamDiskImageOffset;
        }

        RamDiskResetVisibleRegion();
    }
    else
    {
        /* We initialize the Ramdisk from the load options */
        ARC_STATUS Status;
        CHAR FileName[MAX_PATH] = "";
        PVOID OriginalBase;

        /* If we don't have any load options, initialize an empty Ramdisk */
        if (LoadOptions)
        {
            PCSTR Option;
            ULONG FileNameLength;
            ULONG OptionLength;

            Option = NtLdrGetOptionEx(LoadOptions, "RDRAMSIZE=", &OptionLength);
            if (Option && OptionLength > (sizeof("RDRAMSIZE=") - 1))
            {
                ULONGLONG ParsedSize;

                ParsedSize = RamDiskParseSizeString(
                                Option + (sizeof("RDRAMSIZE=") - 1),
                                OptionLength - (sizeof("RDRAMSIZE=") - 1));
                if (ParsedSize != 0)
                {
                    RamDiskRequestedSize = ParsedSize;
                    TRACE("Requested writable ramdisk size: %llu bytes\n",
                          RamDiskRequestedSize);
                }
                else
                {
                    WARN("Ignoring invalid RDRAMSIZE option value\n");
                }
            }

            /* Ramdisk image file name */
            Option = NtLdrGetOptionEx(LoadOptions, "RDPATH=", &FileNameLength);
            if (Option && (FileNameLength > 7))
            {
                /* Copy the file name */
                Option += 7; FileNameLength -= 7;
                RtlStringCbCopyNA(FileName, sizeof(FileName),
                                  Option, FileNameLength * sizeof(CHAR));
            }

            /* Ramdisk image length */
            Option = NtLdrGetOption(LoadOptions, "RDIMAGELENGTH=");
            if (Option)
            {
                RamDiskImageLength = _atoi64(Option + 14);
            }

            /* Ramdisk image offset */
            Option = NtLdrGetOption(LoadOptions, "RDIMAGEOFFSET=");
            if (Option)
            {
                RamDiskImageOffset = atol(Option + 14);
            }
        }

        BOOLEAN StreamingSucceeded = FALSE;
        ULONGLONG StreamIsoSize = 0;
        BOOLEAN OptionalRamDisk = (RamDiskRequestedSize != 0 && !*FileName);

        if (RamDiskRequestedSize != 0)
        {
            ISO_SOURCE StreamSource;
            PVOID WritableBase;
            ULONGLONG WritableSize;
            ARC_STATUS StreamStatus;
            PCSTR StreamFileName = NULL;
            PCSTR StreamDefaultPath = NULL;

            if (*FileName)
            {
                StreamFileName = FileName;
                StreamDefaultPath = DefaultPath;
            }
            else if (DefaultPath)
            {
                StreamFileName = DefaultPath;
                StreamDefaultPath = NULL;
            }

            if (StreamFileName)
            {
                StreamStatus = RamDiskOpenIsoSource(StreamFileName,
                                                    StreamDefaultPath,
                                                    RamDiskImageOffset,
                                                    RamDiskImageLength,
                                                    &StreamSource);
                if (StreamStatus != ESUCCESS)
                {
                    /* BIOS/legacy fallback: if opening by file/path failed (e.g. DefaultPath
                     * is 'ramdisk(0)' or not a real ISO path), try raw firmware CD devices. */
                    static const PCSTR CdCandidates[] = { "cdrom(0)", "cdrom(1)", NULL };
                    for (int i = 0; CdCandidates[i]; ++i)
                    {
                        StreamStatus = RamDiskOpenIsoSource(CdCandidates[i],
                                                            NULL,
                                                            0, /* ISO starts at LBA 0 */
                                                            0,
                                                            &StreamSource);
                        if (StreamStatus == ESUCCESS)
                            break;
                    }
                }

                if (StreamStatus == ESUCCESS)
                {
                    StreamIsoSize = StreamSource.Size;
                    {
                        ULONGLONG ExtraBytes = RamDiskRequestedSize;
                        if (ExtraBytes < RAMDISK_MINIMUM_EXTRA_SPACE)
                            ExtraBytes = RAMDISK_MINIMUM_EXTRA_SPACE;
                        ULONGLONG TotalTarget = StreamIsoSize + ExtraBytes;

                        if (RamDiskBuildWritableImage(&StreamSource,
                                                       TotalTarget,
                                                       &WritableBase,
                                                       &WritableSize,
                                                       OptionalRamDisk))
                        {
                            RamDiskCloseIsoSource(&StreamSource);
                            RamDiskBase = WritableBase;
                            RamDiskFileSize = WritableSize;
                            RamDiskImageOffset = 0;
                            RamDiskImageLength = WritableSize;
                            StreamingSucceeded = TRUE;
                            TRACE("RamDiskInitialize: writable ramdisk ready from streaming (%llu bytes, ISO=%llu extra=%llu)\n",
                                  RamDiskFileSize, StreamIsoSize, ExtraBytes);
                        }
                        else
                        {
                            /* Try to adaptively shrink the extra overlay to fit the low-memory budget. */
                            ULONGLONG AllocationLimit = RamDiskWritableAllocationLimit();
                            ULONGLONG MaxExtra = (AllocationLimit > StreamIsoSize)
                                                   ? (AllocationLimit - StreamIsoSize)
                                                   : 0;
                            if (MaxExtra >= RAMDISK_MINIMUM_EXTRA_SPACE && MaxExtra < ExtraBytes)
                            {
                                ULONGLONG ShrunkTotal = StreamIsoSize + MaxExtra;
                                TRACE("RamDiskInitialize: streaming expansion failed; retrying with shrunk extra %llu (total %llu)\n",
                                      MaxExtra, ShrunkTotal);

                                if (RamDiskBuildWritableImage(&StreamSource,
                                                               ShrunkTotal,
                                                               &WritableBase,
                                                               &WritableSize,
                                                               OptionalRamDisk))
                                {
                                    RamDiskCloseIsoSource(&StreamSource);
                                    RamDiskBase = WritableBase;
                                    RamDiskFileSize = WritableSize;
                                    RamDiskImageOffset = 0;
                                    RamDiskImageLength = WritableSize;
                                    /* Record the effective requested size so boot path gets retargeted. */
                                    RamDiskRequestedSize = MaxExtra;
                                    StreamingSucceeded = TRUE;
                                    TRACE("RamDiskInitialize: writable ramdisk ready from streaming after shrink (%llu bytes, ISO=%llu extra=%llu)\n",
                                          RamDiskFileSize, StreamIsoSize, MaxExtra);
                                }
                                else
                                {
                                    RamDiskCloseIsoSource(&StreamSource);
                                    TRACE("RamDiskInitialize: streaming expansion failed even after shrink; falling back to in-memory copy\n");
                                }
                            }
                            else
                            {
                                RamDiskCloseIsoSource(&StreamSource);
                                TRACE("RamDiskInitialize: streaming writable expansion failed, falling back to in-memory copy\n");
                            }
                        }
                    }
                }
            }

            if (StreamingSucceeded)
                goto WritableReady;

            if (RamDiskRequestedSize != 0 && StreamIsoSize != 0)
            {
                ULONGLONG IsoSize = StreamIsoSize;
                ULONGLONG ResidentIsoBytes;
                ULONGLONG AllocationLimit = RamDiskWritableAllocationLimit();
                ULONGLONG ExtraBytes = RamDiskRequestedSize;
                ULONGLONG RequiredSize;

                /* New semantics: RDRAMSIZE denotes extra writable bytes
                   beyond the ISO contents. Enforce a minimum of 64 MiB
                   headroom if the request is smaller. */
                if (ExtraBytes < RAMDISK_MINIMUM_EXTRA_SPACE)
                    ExtraBytes = RAMDISK_MINIMUM_EXTRA_SPACE;

                RequiredSize = IsoSize + ExtraBytes;

                if (RequiredSize > AllocationLimit)
                {
                    WARN("RamDiskInitialize: writable overlay request (%llu) exceeds low-memory limit before staging ISO (%llu)\n",
                         RequiredSize,
                         (ULONGLONG)RAMDISK_LOW_ALLOC_MAX);
                    if (!RamDiskErrorShown && !OptionalRamDisk)
                    {
                        UiMessageBox("Writable RAM disk request exceeds available low memory. Continuing with read-only media.");
                        RamDiskErrorShown = TRUE;
                    }
                    RamDiskRequestedSize = 0;
                }
                else
                {
                    ResidentIsoBytes = ALIGN_UP_BY_ULL(IsoSize, RAMDISK_ALLOCATION_ALIGNMENT);

                    if (ResidentIsoBytes > AllocationLimit ||
                        RequiredSize > AllocationLimit - ResidentIsoBytes)
                    {
                        WARN("RamDiskInitialize: ISO (%llu) + writable extra (%llu) would exceed low-memory budget %llu\n",
                             IsoSize,
                             ExtraBytes,
                             (ULONGLONG)RAMDISK_LOW_ALLOC_MAX);
                        if (!RamDiskErrorShown && !OptionalRamDisk)
                        {
                            UiMessageBox("Writable RAM disk request leaves insufficient low memory once the ISO is cached. Continuing with read-only media.");
                            RamDiskErrorShown = TRUE;
                        }
                        RamDiskRequestedSize = 0;
                    }
                }
            }
        }

        if (*FileName)
            Status = RamDiskLoadVirtualFile(FileName, DefaultPath, OptionalRamDisk);
        else
            Status = RamDiskLoadVirtualFile(DefaultPath, NULL, OptionalRamDisk);
        if (Status != ESUCCESS)
            return Status;

        OriginalBase = RamDiskBase;
            if (RamDiskRequestedSize != 0)
            {
            TRACE("RamDiskInitialize: expanding to writable RAMFS (extra %llu bytes requested)\n",
                  RamDiskRequestedSize);
                PVOID WritableBase;
                ULONGLONG WritableSize;
                PVOID IsoImageBase;
                ULONGLONG IsoImageLength;
                ISO_SOURCE MemorySource;

            IsoImageBase = (PVOID)((ULONG_PTR)OriginalBase + RamDiskImageOffset);
            IsoImageLength = RamDiskFileSize - RamDiskImageOffset;

            MemorySource.MemoryBase = IsoImageBase;
            MemorySource.Size = (RamDiskImageLength != 0 &&
                                 RamDiskImageLength <= IsoImageLength)
                                ? RamDiskImageLength
                                : IsoImageLength;
            MemorySource.ArcFileId = INVALID_FILE_ID;
            MemorySource.ArcOffset = 0;
            MemorySource.ArcPosition = 0;

            /* New semantics: total target = ISO length + extra (>=64MiB) */
            {
                ULONGLONG ExtraBytes = RamDiskRequestedSize;
                if (ExtraBytes < RAMDISK_MINIMUM_EXTRA_SPACE)
                    ExtraBytes = RAMDISK_MINIMUM_EXTRA_SPACE;
                ULONGLONG TotalTarget = MemorySource.Size + ExtraBytes;

                if (!RamDiskBuildWritableImage(&MemorySource,
                                               TotalTarget,
                                               &WritableBase,
                                               &WritableSize,
                                               OptionalRamDisk))
                {
                    if (!RamDiskErrorShown && !OptionalRamDisk)
                    {
                        UiMessageBox("Failed to expand LiveCD into writable RAM.");
                        RamDiskErrorShown = TRUE;
                    }
                    RamDiskRequestedSize = 0;
                    TRACE("RamDiskInitialize: continuing with read-only ISO because writable buffer allocation failed\n");
                    RamDiskBase = OriginalBase;
                    RamDiskVolumeOffset = 0;
                    RamDiskVolumeLength = 0;
                    goto WritableFallback;
                }
            }

            if ((OriginalBase != gInitRamDiskBase) &&
                (OriginalBase != WritableBase))
            {
                MmFreeMemory(OriginalBase);
            }

            RamDiskBase = WritableBase;
            RamDiskFileSize = WritableSize;
            RamDiskImageOffset = 0;
            RamDiskImageLength = WritableSize;
            TRACE("RamDiskInitialize: writable ramdisk ready (%llu bytes)\n",
                  RamDiskFileSize);
        }
    }

WritableReady:
    /* Adjust the Ramdisk image length if needed */
    if (!RamDiskImageLength || (RamDiskImageLength > RamDiskFileSize - RamDiskImageOffset))
        RamDiskImageLength = RamDiskFileSize - RamDiskImageOffset;

WritableFallback:

    /* Ensure a fresh filesystem mount the next time ramdisk(0) is accessed. */
    if (RamDiskVolumeLength == 0)
    {
        RamDiskResetVisibleRegion();
    }
    /* Changing the exposed LBA window invalidates any cached FAT mount state. */
    RamDiskInvalidateFatCache();

    RamDiskRegisterArcDevice();

    /* Register the RAMDISK device */
    if (!RamDiskDeviceRegistered)
    {
        FsRegisterDevice("ramdisk(0)", &RamDiskVtbl);
        RamDiskDeviceRegistered = TRUE;
    }

#if DBG
    if (RamDiskBase && RamDiskFileSize != 0)
    {
        PFN_NUMBER BasePfn = RamDiskPointerToPfn(RamDiskBase);
        PFN_NUMBER EndPfn = RamDiskPointerToPfn((const VOID *)((ULONG_PTR)RamDiskBase + (ULONG_PTR)(RamDiskFileSize - 1)));
        TRACE("RamDiskInitialize: base=%p size=%llu basePFN=%lx endPFN=%lx imageOffset=%lu imageLength=%llu volumeOffset=%llu volumeLength=%llu requested=%llu\n",
              RamDiskBase,
              RamDiskFileSize,
              BasePfn,
              EndPfn,
              RamDiskImageOffset,
              RamDiskImageLength,
              RamDiskVolumeOffset,
              RamDiskVolumeLength,
              RamDiskRequestedSize);
        RamDiskTraceSample("  disk[0]", RamDiskBase);
        if (RamDiskImageOffset < RamDiskFileSize)
        {
            RamDiskTraceSample("  disk[image]", (PUCHAR)RamDiskBase + RamDiskImageOffset);
        }
        if (RamDiskVolumeOffset < RamDiskFileSize)
        {
            RamDiskTraceSample("  disk[volume]", (PUCHAR)RamDiskBase + RamDiskVolumeOffset);
        }
    }
#endif

    return ESUCCESS;
}
