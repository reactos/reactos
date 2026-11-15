/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/filesystems/ntfs/volinfo.c
 * PURPOSE:     NTFS filesystem driver
 * COPYRIGHT:   Copyright (C) 2002, 2014 ReactOS Team
 *              See COPYING in the top level directory
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Eric Kohl
 *              Pierre Schweitzer (pierre@reactos.org)
 *              Ryan Wilson <ryan@verosnt.com>
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
NtfsBringClusterWindow(PDEVICE_EXTENSION DeviceExt,
                       ULONGLONG Bit);

/**
* @name NtfsLoadVolumeBitmap
*
* Reads $Bitmap into memory once at mount. Cluster allocation and freeing work
* on this copy and write back only the sectors they change.
*
* @param DeviceExt
* Volume being mounted. Its file record lookaside list and MFT context must be ready.
*
* @return
* STATUS_SUCCESS, or an error if $Bitmap could not be found or read.
*/
NTSTATUS
NtfsLoadVolumeBitmap(
    _In_ PDEVICE_EXTENSION DeviceExt)
{
    PFILE_RECORD_HEADER BitmapRecord;
    ULONGLONG BitmapSize;
    ULONG BufferSize;
    NTSTATUS Status;

    BitmapRecord = ExAllocateFromNPagedLookasideList(&DeviceExt->FileRecLookasideList);
    if (BitmapRecord == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = ReadFileRecord(DeviceExt, NTFS_FILE_BITMAP, BitmapRecord);
    if (NT_SUCCESS(Status))
    {
        /* The context keeps its own copy of the attribute record */
        Status = FindAttribute(DeviceExt, BitmapRecord, AttributeData, L"", 0, &DeviceExt->BitmapContext, NULL);
    }

    ExFreeToNPagedLookasideList(&DeviceExt->FileRecLookasideList, BitmapRecord);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to find $Bitmap (Status %lx)\n", Status);
        DeviceExt->BitmapContext = NULL;
        return Status;
    }

    BitmapSize = AttributeDataLength(DeviceExt->BitmapContext->pRecord);
    if (BitmapSize > MAXULONG ||
        BitmapSize * 8 < DeviceExt->NtfsInfo.ClusterCount ||
        DeviceExt->NtfsInfo.ClusterCount > MAXULONG)
    {
        DPRINT1("$Bitmap size %I64u does not cover %I64u clusters\n",
                BitmapSize, DeviceExt->NtfsInfo.ClusterCount);
        Status = STATUS_FILE_CORRUPT_ERROR;
        goto Fail;
    }

    /* One window, not the whole $Bitmap. A large volume must not need one
     * contiguous allocation of every cluster bit. */
    DeviceExt->BitmapSize = (ULONG)BitmapSize;
    DeviceExt->ClusterWindowBytes = 64 * 1024;
    if (DeviceExt->ClusterWindowBytes > DeviceExt->BitmapSize)
        DeviceExt->ClusterWindowBytes = DeviceExt->BitmapSize;
    DeviceExt->ClusterWindowBytes = ROUND_UP(DeviceExt->ClusterWindowBytes,
                                            DeviceExt->NtfsInfo.BytesPerSector);
    if (DeviceExt->ClusterWindowBytes > DeviceExt->BitmapSize)
        DeviceExt->ClusterWindowBytes = DeviceExt->BitmapSize;

    BufferSize = DeviceExt->ClusterWindowBytes;
    DeviceExt->BitmapBuffer = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_NTFS);
    if (DeviceExt->BitmapBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Fail;
    }
    RtlZeroMemory(DeviceExt->BitmapBuffer, BufferSize);
    DeviceExt->ClusterWindowBase = 0;

    if (ReadAttribute(DeviceExt, DeviceExt->BitmapContext, 0, (PCHAR)DeviceExt->BitmapBuffer, BufferSize) != BufferSize)
    {
        DPRINT1("Unable to read $Bitmap\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Fail;
    }

    {
        ULONG WindowBits = BufferSize * 8;
        PUCHAR Scan;
        ULONG ScanBytes;
        ULONGLONG Free = 0;
        ULONGLONG Offset = 0;

        if ((ULONGLONG)WindowBits > DeviceExt->NtfsInfo.ClusterCount)
            WindowBits = (ULONG)DeviceExt->NtfsInfo.ClusterCount;
        RtlInitializeBitMap(&DeviceExt->ClusterBitmap, DeviceExt->BitmapBuffer, WindowBits);

        Scan = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_NTFS);
        if (Scan == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Fail;
        }

        while (Offset < BitmapSize)
        {
            ULONG Bit;
            ScanBytes = BufferSize;
            if (Offset + ScanBytes > BitmapSize)
                ScanBytes = (ULONG)(BitmapSize - Offset);
            if (ReadAttribute(DeviceExt, DeviceExt->BitmapContext, Offset, (PCHAR)Scan, ScanBytes) != ScanBytes)
            {
                ExFreePoolWithTag(Scan, TAG_NTFS);
                Status = STATUS_UNSUCCESSFUL;
                goto Fail;
            }
            for (Bit = 0; Bit < ScanBytes * 8; Bit++)
            {
                ULONGLONG Cluster = Offset * 8 + Bit;
                if (Cluster >= DeviceExt->NtfsInfo.ClusterCount)
                    break;
                if ((Scan[Bit / 8] & (1 << (Bit % 8))) == 0)
                    Free++;
            }
            Offset += ScanBytes;
        }
        ExFreePoolWithTag(Scan, TAG_NTFS);
        DeviceExt->FreeClusterCount = Free;
    }

    DPRINT("Volume bitmap loaded: %I64u of %I64u clusters free\n",
           DeviceExt->FreeClusterCount, DeviceExt->NtfsInfo.ClusterCount);

    return STATUS_SUCCESS;

Fail:
    NtfsFreeVolumeBitmap(DeviceExt);
    return Status;
}

/**
* @name NtfsFreeVolumeBitmap
*
* Releases what NtfsLoadVolumeBitmap() set up. Safe on a partly loaded volume.
*
* @param DeviceExt
* Volume whose bitmap copy is released.
*/
VOID
NtfsFreeVolumeBitmap(
    _In_ PDEVICE_EXTENSION DeviceExt)
{
    if (DeviceExt->BitmapBuffer != NULL)
    {
        ExFreePoolWithTag(DeviceExt->BitmapBuffer, TAG_NTFS);
        DeviceExt->BitmapBuffer = NULL;
    }

    if (DeviceExt->BitmapContext != NULL)
    {
        ReleaseAttributeContext(DeviceExt->BitmapContext);
        DeviceExt->BitmapContext = NULL;
    }
}

/**
* @name NtfsWriteVolumeBitmap
*
* Writes the sectors of $Bitmap that hold the given clusters' bits.
* The caller holds BitmapResource exclusively.
*
* @param DeviceExt
* Volume whose bitmap changed.
*
* @param FirstCluster
* First cluster whose bit changed.
*
* @param ClusterCount
* Number of clusters, starting at FirstCluster, to cover. Can't be 0.
*
* @return
* Status from WriteAttribute().
*/
NTSTATUS
NtfsWriteVolumeBitmap(
    _In_ PDEVICE_EXTENSION DeviceExt,
    _In_ ULONG FirstCluster,
    _In_ ULONG ClusterCount)
{
    ULONG SectorSize = DeviceExt->NtfsInfo.BytesPerSector;
    ULONG Cursor = FirstCluster;
    ULONG Last = FirstCluster + ClusterCount;

    ASSERT(ClusterCount != 0);

    while (Cursor < Last)
    {
        ULONG BaseBit;
        ULONG WindowBits;
        ULONG ByteStart;
        ULONG ByteEnd;
        ULONG LengthWritten;
        NTSTATUS Status;

        Status = NtfsBringClusterWindow(DeviceExt, Cursor);
        if (!NT_SUCCESS(Status))
            return Status;

        BaseBit = (ULONG)(DeviceExt->ClusterWindowBase * 8);
        WindowBits = DeviceExt->ClusterBitmap.SizeOfBitMap;
        ByteStart = ROUND_DOWN((Cursor - BaseBit) / 8, SectorSize);
        if (BaseBit + WindowBits >= Last)
            ByteEnd = (Last - BaseBit + 7) / 8;
        else
            ByteEnd = WindowBits / 8;
        ByteEnd = ROUND_UP(ByteEnd, SectorSize);
        if (ByteEnd > WindowBits / 8)
            ByteEnd = WindowBits / 8;
        if (ByteEnd <= ByteStart)
            return STATUS_UNSUCCESSFUL;

        Status = WriteAttribute(DeviceExt,
                                DeviceExt->BitmapContext,
                                DeviceExt->ClusterWindowBase + ByteStart,
                                (PUCHAR)DeviceExt->BitmapBuffer + ByteStart,
                                ByteEnd - ByteStart,
                                &LengthWritten,
                                NULL);
        if (!NT_SUCCESS(Status))
            return Status;

        Cursor = BaseBit + WindowBits;
        if (Cursor <= BaseBit)
            break;
    }

    DeviceExt->ClusterWindowDirty = FALSE;
    return STATUS_SUCCESS;
}

ULONGLONG
NtfsGetFreeClusters(PDEVICE_EXTENSION DeviceExt)
{
    ULONGLONG FreeClusters;

    DPRINT("NtfsGetFreeClusters(%p)\n", DeviceExt);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&DeviceExt->BitmapResource, TRUE);
    FreeClusters = DeviceExt->FreeClusterCount;
    ExReleaseResourceLite(&DeviceExt->BitmapResource);
    KeLeaveCriticalRegion();

    return FreeClusters;
}

/**
* Claim up to Want clear clusters inside [From, ToExclusive). A full run is
* preferred. The bits are marked in use. Returns FALSE when the window has none.
*/
static
BOOLEAN
NtfsClaimRun(PRTL_BITMAP Bitmap,
             ULONG From,
             ULONG ToExclusive,
             ULONG Want,
             PULONG Assigned,
             PULONG Count)
{
    ULONG Pass;
    ULONG RunStart;
    ULONG RunLength;
    ULONG Take;

    if (From >= ToExclusive || Want == 0)
        return FALSE;

    for (Pass = 0; Pass < 2; Pass++)
    {
        ULONG Cursor = From;

        RunLength = RtlFindNextForwardRunClear(Bitmap, Cursor, &RunStart);
        while (RunLength != 0 && RunStart < ToExclusive)
        {
            if (RunStart < From)
            {
                ULONG Skip = From - RunStart;

                if (RunLength <= Skip)
                    goto Next;
                RunStart = From;
                RunLength -= Skip;
            }

            Take = min(RunLength, ToExclusive - RunStart);
            if (Pass == 0 && Take < Want)
                goto Next;

            Take = min(Take, Want);
            if (Take != 0)
            {
                RtlSetBits(Bitmap, RunStart, Take);
                *Assigned = RunStart;
                *Count = Take;
                return TRUE;
            }

Next:
            if (RunStart + RunLength <= RunStart)
                break;
            Cursor = RunStart + RunLength;
            if (Cursor >= Bitmap->SizeOfBitMap)
                break;
            RunLength = RtlFindNextForwardRunClear(Bitmap, Cursor, &RunStart);
        }
    }

    return FALSE;
}

/* Load the window that contains Bit. The caller holds BitmapResource.
 * The previous window is already on disk: every allocation writes its bits
 * back before it returns. */
NTSTATUS
NtfsBringClusterWindow(PDEVICE_EXTENSION DeviceExt,
                       ULONGLONG Bit)
{
    ULONGLONG Byte;
    ULONGLONG Base;
    ULONG Window;
    ULONG Bits;
    ULONG Got;

    Window = DeviceExt->ClusterWindowBytes;
    if (Window == 0 || DeviceExt->BitmapBuffer == NULL)
        return STATUS_UNSUCCESSFUL;

    Byte = Bit / 8;
    Base = Byte - (Byte % Window);
    if (Base == DeviceExt->ClusterWindowBase)
        return STATUS_SUCCESS;

    if (DeviceExt->ClusterWindowDirty)
    {
        ULONG LengthWritten;
        ULONG Have = (ULONG)min((ULONGLONG)Window, DeviceExt->BitmapSize - DeviceExt->ClusterWindowBase);

        if (WriteAttribute(DeviceExt,
                           DeviceExt->BitmapContext,
                           DeviceExt->ClusterWindowBase,
                           (PUCHAR)DeviceExt->BitmapBuffer,
                           Have,
                           &LengthWritten,
                           NULL) != STATUS_SUCCESS)
            return STATUS_UNSUCCESSFUL;
        DeviceExt->ClusterWindowDirty = FALSE;
    }

    if (Base >= DeviceExt->BitmapSize)
        return STATUS_END_OF_FILE;

    RtlZeroMemory(DeviceExt->BitmapBuffer, Window);
    Got = (ULONG)min((ULONGLONG)Window, DeviceExt->BitmapSize - Base);
    if (ReadAttribute(DeviceExt, DeviceExt->BitmapContext, Base, (PCHAR)DeviceExt->BitmapBuffer, Got) != Got)
        return STATUS_UNSUCCESSFUL;

    DeviceExt->ClusterWindowBase = Base;
    Bits = Got * 8;
    if (Base * 8 + Bits > DeviceExt->NtfsInfo.ClusterCount)
        Bits = (ULONG)(DeviceExt->NtfsInfo.ClusterCount - Base * 8);
    RtlInitializeBitMap(&DeviceExt->ClusterBitmap, DeviceExt->BitmapBuffer, Bits);
    return STATUS_SUCCESS;
}

static
BOOLEAN
NtfsClaimAbsolute(PDEVICE_EXTENSION DeviceExt,
                  ULONG From,
                  ULONG To,
                  ULONG Want,
                  PULONG Assigned,
                  PULONG Count)
{
    ULONG Cursor = From;

    while (Cursor < To)
    {
        ULONG Base;
        ULONG Size;
        ULONG LocalFrom;
        ULONG LocalTo;

        if (!NT_SUCCESS(NtfsBringClusterWindow(DeviceExt, Cursor)))
            return FALSE;

        Base = (ULONG)(DeviceExt->ClusterWindowBase * 8);
        Size = DeviceExt->ClusterBitmap.SizeOfBitMap;
        LocalFrom = (From <= Base) ? 0 : From - Base;
        LocalTo = (To >= Base + Size) ? Size : To - Base;
        if (LocalFrom < LocalTo &&
            NtfsClaimRun(&DeviceExt->ClusterBitmap, LocalFrom, LocalTo, Want, Assigned, Count))
        {
            *Assigned += Base;
            DeviceExt->ClusterWindowDirty = TRUE;
            return TRUE;
        }

        if (Base + Size <= Cursor)
            return FALSE;
        Cursor = Base + Size;
    }

    return FALSE;
}

/**
* NtfsAllocateClusters
* Allocates a run of clusters. The run allocated might be smaller than DesiredClusters.
* ForMft is set when the clusters extend $MFT. Other files leave the reservation
* zone alone until nowhere else is free.
*/
NTSTATUS
NtfsAllocateClusters(PDEVICE_EXTENSION DeviceExt,
                     ULONG FirstDesiredCluster,
                     ULONG DesiredClusters,
                     PULONG FirstAssignedCluster,
                     PULONG AssignedClusters,
                     BOOLEAN ForMft)
{
    NTSTATUS Status;
    ULONG BitmapSize;
    ULONG ZoneStart;
    ULONG ZoneEnd;
    BOOLEAN Claimed = FALSE;

    DPRINT("NtfsAllocateClusters(%p, %lu, %lu, %p, %p)\n", DeviceExt, FirstDesiredCluster, DesiredClusters, FirstAssignedCluster, AssignedClusters);

    *AssignedClusters = 0;

    /* Files growing in parallel must not both claim the same free run */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&DeviceExt->BitmapResource, TRUE);

    if (DeviceExt->FreeClusterCount < DesiredClusters)
    {
        Status = STATUS_DISK_FULL;
        goto Quit;
    }

    /* The hint is just past the file's last cluster, which can be past the volume end */
    BitmapSize = (ULONG)DeviceExt->NtfsInfo.ClusterCount;
    if (FirstDesiredCluster >= BitmapSize)
        FirstDesiredCluster = 0;

    ZoneStart = (ULONG)min(DeviceExt->NtfsInfo.MftZoneStart, BitmapSize);
    ZoneEnd = (ULONG)min(DeviceExt->NtfsInfo.MftZoneEnd, BitmapSize);
    if (ZoneEnd < ZoneStart)
        ZoneEnd = ZoneStart;

    if (ForMft)
    {
        ULONG Hint = FirstDesiredCluster;

        if (Hint < ZoneStart || Hint >= ZoneEnd)
            Hint = ZoneStart;
        Claimed = NtfsClaimAbsolute(DeviceExt, Hint, ZoneEnd, DesiredClusters, FirstAssignedCluster, AssignedClusters);
        if (!Claimed)
            Claimed = NtfsClaimAbsolute(DeviceExt, ZoneEnd, BitmapSize, DesiredClusters, FirstAssignedCluster, AssignedClusters);
        if (!Claimed)
            Claimed = NtfsClaimAbsolute(DeviceExt, 0, ZoneStart, DesiredClusters, FirstAssignedCluster, AssignedClusters);
    }
    else
    {
        if (FirstDesiredCluster < ZoneStart)
            Claimed = NtfsClaimAbsolute(DeviceExt, FirstDesiredCluster, ZoneStart, DesiredClusters, FirstAssignedCluster, AssignedClusters);
        else if (FirstDesiredCluster >= ZoneEnd)
            Claimed = NtfsClaimAbsolute(DeviceExt, FirstDesiredCluster, BitmapSize, DesiredClusters, FirstAssignedCluster, AssignedClusters);

        if (!Claimed)
            Claimed = NtfsClaimAbsolute(DeviceExt, ZoneEnd, BitmapSize, DesiredClusters, FirstAssignedCluster, AssignedClusters);
        if (!Claimed)
            Claimed = NtfsClaimAbsolute(DeviceExt, 0, ZoneStart, DesiredClusters, FirstAssignedCluster, AssignedClusters);
        if (!Claimed)
            Claimed = NtfsClaimAbsolute(DeviceExt, ZoneStart, ZoneEnd, DesiredClusters, FirstAssignedCluster, AssignedClusters);
    }

    if (!Claimed)
    {
        Status = STATUS_DISK_FULL;
        goto Quit;
    }

    Status = NtfsWriteVolumeBitmap(DeviceExt, *FirstAssignedCluster, *AssignedClusters);
    if (NT_SUCCESS(Status))
    {
        DeviceExt->FreeClusterCount -= *AssignedClusters;
    }
    else
    {
        /* Keep the copy in step with what the disk says */
        if (NT_SUCCESS(NtfsBringClusterWindow(DeviceExt, *FirstAssignedCluster)))
        {
            ULONG Base = (ULONG)(DeviceExt->ClusterWindowBase * 8);
            RtlClearBits(&DeviceExt->ClusterBitmap, *FirstAssignedCluster - Base, *AssignedClusters);
            DeviceExt->ClusterWindowDirty = TRUE;
        }
        *AssignedClusters = 0;
    }

Quit:
    ExReleaseResourceLite(&DeviceExt->BitmapResource);
    KeLeaveCriticalRegion();

    return Status;
}

static
NTSTATUS
NtfsGetFsVolumeInformation(PDEVICE_OBJECT DeviceObject,
                           PFILE_FS_VOLUME_INFORMATION FsVolumeInfo,
                           PULONG BufferLength)
{
    DPRINT("NtfsGetFsVolumeInformation() called\n");
    DPRINT("FsVolumeInfo = %p\n", FsVolumeInfo);
    DPRINT("BufferLength %lu\n", *BufferLength);

    DPRINT("Vpb %p\n", DeviceObject->Vpb);

    DPRINT("Required length %lu\n",
           sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength);
    DPRINT("LabelLength %hu\n",
           DeviceObject->Vpb->VolumeLabelLength);
    DPRINT("Label %.*S\n",
           DeviceObject->Vpb->VolumeLabelLength / sizeof(WCHAR),
           DeviceObject->Vpb->VolumeLabel);

    if (*BufferLength < sizeof(FILE_FS_VOLUME_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    if (*BufferLength < (sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength))
        return STATUS_BUFFER_OVERFLOW;

    /* valid entries */
    FsVolumeInfo->VolumeSerialNumber = DeviceObject->Vpb->SerialNumber;
    FsVolumeInfo->VolumeLabelLength = DeviceObject->Vpb->VolumeLabelLength;
    memcpy(FsVolumeInfo->VolumeLabel,
           DeviceObject->Vpb->VolumeLabel,
           DeviceObject->Vpb->VolumeLabelLength);

    /* dummy entries */
    FsVolumeInfo->VolumeCreationTime.QuadPart = 0;
    FsVolumeInfo->SupportsObjects = FALSE;

    *BufferLength -= (sizeof(FILE_FS_VOLUME_INFORMATION) + DeviceObject->Vpb->VolumeLabelLength);

    DPRINT("BufferLength %lu\n", *BufferLength);
    DPRINT("NtfsGetFsVolumeInformation() done\n");

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetFsAttributeInformation(PDEVICE_EXTENSION DeviceExt,
                              PFILE_FS_ATTRIBUTE_INFORMATION FsAttributeInfo,
                              PULONG BufferLength)
{
    UNREFERENCED_PARAMETER(DeviceExt);

    DPRINT("NtfsGetFsAttributeInformation()\n");
    DPRINT("FsAttributeInfo = %p\n", FsAttributeInfo);
    DPRINT("BufferLength %lu\n", *BufferLength);
    DPRINT("Required length %lu\n", (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8));

    if (*BufferLength < sizeof (FILE_FS_ATTRIBUTE_INFORMATION))
        return STATUS_INFO_LENGTH_MISMATCH;

    if (*BufferLength < (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8))
        return STATUS_BUFFER_OVERFLOW;

    FsAttributeInfo->FileSystemAttributes =
        FILE_CASE_PRESERVED_NAMES | FILE_UNICODE_ON_DISK | FILE_READ_ONLY_VOLUME;
    FsAttributeInfo->MaximumComponentNameLength = 255;
    FsAttributeInfo->FileSystemNameLength = 8;

    memcpy(FsAttributeInfo->FileSystemName, L"NTFS", 8);

    DPRINT("Finished NtfsGetFsAttributeInformation()\n");

    *BufferLength -= (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 8);
    DPRINT("BufferLength %lu\n", *BufferLength);

    return STATUS_SUCCESS;
}


static
NTSTATUS
NtfsGetFsSizeInformation(PDEVICE_OBJECT DeviceObject,
                         PFILE_FS_SIZE_INFORMATION FsSizeInfo,
                         PULONG BufferLength)
{
    PDEVICE_EXTENSION DeviceExt;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("NtfsGetFsSizeInformation()\n");
    DPRINT("FsSizeInfo = %p\n", FsSizeInfo);

    if (*BufferLength < sizeof(FILE_FS_SIZE_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    DeviceExt = DeviceObject->DeviceExtension;

    FsSizeInfo->AvailableAllocationUnits.QuadPart = NtfsGetFreeClusters(DeviceExt);
    FsSizeInfo->TotalAllocationUnits.QuadPart = DeviceExt->NtfsInfo.ClusterCount;
    FsSizeInfo->SectorsPerAllocationUnit = DeviceExt->NtfsInfo.SectorsPerCluster;
    FsSizeInfo->BytesPerSector = DeviceExt->NtfsInfo.BytesPerSector;

    DPRINT("Finished NtfsGetFsSizeInformation()\n");
    if (NT_SUCCESS(Status))
        *BufferLength -= sizeof(FILE_FS_SIZE_INFORMATION);

    return Status;
}


static
NTSTATUS
NtfsGetFsDeviceInformation(PDEVICE_OBJECT DeviceObject,
                           PFILE_FS_DEVICE_INFORMATION FsDeviceInfo,
                           PULONG BufferLength)
{
    DPRINT("NtfsGetFsDeviceInformation()\n");
    DPRINT("FsDeviceInfo = %p\n", FsDeviceInfo);
    DPRINT("BufferLength %lu\n", *BufferLength);
    DPRINT("Required length %lu\n", sizeof(FILE_FS_DEVICE_INFORMATION));

    if (*BufferLength < sizeof(FILE_FS_DEVICE_INFORMATION))
        return STATUS_BUFFER_OVERFLOW;

    FsDeviceInfo->DeviceType = FILE_DEVICE_DISK;
    FsDeviceInfo->Characteristics = DeviceObject->Characteristics;

    DPRINT("NtfsGetFsDeviceInformation() finished.\n");

    *BufferLength -= sizeof(FILE_FS_DEVICE_INFORMATION);
    DPRINT("BufferLength %lu\n", *BufferLength);

    return STATUS_SUCCESS;
}


NTSTATUS
NtfsQueryVolumeInformation(PNTFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;
    FS_INFORMATION_CLASS FsInformationClass;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID SystemBuffer;
    ULONG BufferLength;
    PDEVICE_EXTENSION DeviceExt;

    DPRINT("NtfsQueryVolumeInformation() called\n");

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
    DeviceObject = IrpContext->DeviceObject;
    DeviceExt = DeviceObject->DeviceExtension;
    Stack = IrpContext->Stack;

    if (!ExAcquireResourceSharedLite(&DeviceExt->DirResource,
                                     BooleanFlagOn(IrpContext->Flags, IRPCONTEXT_CANWAIT)))
    {
        return NtfsMarkIrpContextForQueue(IrpContext);
    }

    FsInformationClass = Stack->Parameters.QueryVolume.FsInformationClass;
    BufferLength = Stack->Parameters.QueryVolume.Length;
    SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    RtlZeroMemory(SystemBuffer, BufferLength);

    DPRINT("FsInformationClass %d\n", FsInformationClass);
    DPRINT("SystemBuffer %p\n", SystemBuffer);

    switch (FsInformationClass)
    {
        case FileFsVolumeInformation:
            Status = NtfsGetFsVolumeInformation(DeviceObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        case FileFsAttributeInformation:
            Status = NtfsGetFsAttributeInformation(DeviceObject->DeviceExtension,
                                                   SystemBuffer,
                                                   &BufferLength);
            break;

        case FileFsSizeInformation:
            Status = NtfsGetFsSizeInformation(DeviceObject,
                                              SystemBuffer,
                                              &BufferLength);
            break;

        case FileFsDeviceInformation:
            Status = NtfsGetFsDeviceInformation(DeviceObject,
                                                SystemBuffer,
                                                &BufferLength);
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
    }

    ExReleaseResourceLite(&DeviceExt->DirResource);

    if (NT_SUCCESS(Status))
        Irp->IoStatus.Information =
            Stack->Parameters.QueryVolume.Length - BufferLength;
    else
        Irp->IoStatus.Information = 0;

    return Status;
}


NTSTATUS
NtfsSetVolumeInformation(PNTFS_IRP_CONTEXT IrpContext)
{
    PIRP Irp;

    DPRINT("NtfsSetVolumeInformation() called\n");

    ASSERT(IrpContext);

    Irp = IrpContext->Irp;
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    return STATUS_NOT_SUPPORTED;
}

/* EOF */
