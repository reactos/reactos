/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filesystems/fastfat/fat.c
 * PURPOSE:         FAT support routines
 * PROGRAMMERS:     Alexey Vlasov
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* PROTOTYPES ***************************************************************/
typedef struct _FAT_SCAN_CONTEXT
{
    PFILE_OBJECT FileObject;
    LARGE_INTEGER PageOffset;
    LONGLONG BeyondLastEntryOffset;
    PVOID PageBuffer;
    PBCB PageBcb;
} FAT_SCAN_CONTEXT;

#define FatEntryToDataOffset(xEntry, xVcb) \
    ((xVcb)->DataArea + (((LONGLONG) ((xEntry) - 0x02)) << (xVcb)->BytesPerClusterLog))

#define FatDataOffsetToEntry(xOffset, xVcb) \
    ((ULONG) ((xOffset - (xVcb)->DataArea) >> (xVcb)->BytesPerClusterLog) + 0x02)

ULONG
FatScanFat32ForContinousRun(IN OUT PFAT_PAGE_CONTEXT Context,
                            IN OUT PULONG Index,
                            IN BOOLEAN CanWait);

BOOLEAN
NTAPI
FatValidBpb(IN PBIOS_PARAMETER_BLOCK Bpb);

/* VARIABLES ****************************************************************/
FAT_METHODS Fat12Methods = {
    NULL,
    NULL,
    NULL,
    NULL
};

FAT_METHODS Fat16Methods = {
    NULL,
    NULL,
    NULL,
    NULL
};

FAT_METHODS Fat32Methods = {
    FatScanFat32ForContinousRun,
    NULL,
    NULL,
    NULL
};

/* FUNCTIONS ****************************************************************/

/**
 * Pins the page containing ByteOffset byte.
 *
 * @param Context
 * Keeps current BCB, Buffer pointer
 * and maintains current and next page offset.
 *
 * @param ByteOffset
 * Offset from the beginning of the data stream to be pinned.
 *
 * @return
 * Pointer to the buffer starting with the specified ByteOffset.
 */
PVOID
FatPinPage(
    PFAT_PAGE_CONTEXT Context,
    LONGLONG ByteOffset)
{
    SIZE_T OffsetWithinPage;

    OffsetWithinPage = (SIZE_T) (ByteOffset & (PAGE_SIZE - 1));
    ByteOffset -= OffsetWithinPage;
    if (ByteOffset != Context->Offset.QuadPart)
    {
        Context->Offset.QuadPart = ByteOffset;
        if (Context->Bcb != NULL)
        {
            CcUnpinData(Context->Bcb);
            Context->Bcb = NULL;
        }
        if (!CcMapData(Context->FileObject,
                       &Context->Offset,
                       PAGE_SIZE,
                       Context->CanWait,
                       &Context->Bcb,
                       &Context->Buffer))
        {
            Context->Offset.QuadPart = 0LL;
            ExRaiseStatus(STATUS_CANT_WAIT);
        }
    }
    Context->EndOfPage.QuadPart =
        Context->Offset.QuadPart + PAGE_SIZE;
    if (Context->EndOfPage.QuadPart
        > Context->EndOfData.QuadPart)
    {
        Context->ValidLength = (SIZE_T)
            (Context->EndOfData.QuadPart
                - Context->Offset.QuadPart);
    }
    else
    {
        Context->ValidLength = PAGE_SIZE;
    }
    return Add2Ptr(Context->Buffer, OffsetWithinPage, PVOID);
}

/**
 * Pins the next page of data stream.
 *
 * @param Context
 * Keeps current BCB, Buffer pointer
 * and maintains current and next page offset.
 *
 * @return
 * Pointer to the buffer starting with the beginning of the next page.
 */
PVOID
FatPinNextPage(
    PFAT_PAGE_CONTEXT Context)
{
    ASSERT ((Context->Offset.QuadPart % PAGE_SIZE)
        != (Context->EndOfPage.QuadPart % PAGE_SIZE)
        && Context->Bcb != NULL);

    ASSERT  (Context->ValidLength == PAGE_SIZE);

    Context->Offset = Context->EndOfPage;
    CcUnpinData(Context->Bcb);
    if (!CcMapData(Context->FileObject,
                   &Context->Offset,
                   PAGE_SIZE,
                   Context->CanWait,
                   &Context->Bcb,
                   &Context->Buffer))
    {
        Context->Bcb = NULL;
        Context->Offset.QuadPart = 0LL;
        ExRaiseStatus(STATUS_CANT_WAIT);
    }
    Context->EndOfPage.QuadPart =
        Context->Offset.QuadPart + PAGE_SIZE;
    return Context->Buffer;
}

/**
 * Determines the index of the set bit.
 *
 * @param Number
 * Number having a single bit set.
 *
 * @return
 * Index of the set bit.
 */
FORCEINLINE
ULONG
FatPowerOfTwo(
    ULONG Number)
{
    ULONG Temp;
    Temp = Number
        - ((Number >> 1) & 033333333333)
        - ((Number >> 2) & 011111111111);
    return (((Temp + (Temp >> 3)) & 030707070707) % 63);
}

/**
 * Scans FAT32 for continous chain of clusters
 *
 * @param Context
 * Pointer to FAT_PAGE_CONTEXT.
 *
 * @param Index
 * Supplies the Index of the first cluster
 * and receves the last index after the last
 * cluster in the chain.
 *
 * @param CanWait
 * Indicates if the context allows blocking.
 *
 * @return
 * Value of the last claster terminated the scan.
 *
 * @note
 * Raises STATUS_CANT_WAIT race condition.
 */
ULONG
FatScanFat32ForContinousRun(IN OUT PFAT_PAGE_CONTEXT Context,
                            IN OUT PULONG Index,
                            IN BOOLEAN CanWait)
{
    PULONG Entry, EndOfPage;

    Entry = FatPinPage(Context, ((LONGLONG) *Index) << 0x2);
    EndOfPage = FatPinEndOfPage(Context, PULONG);
    while (TRUE)
    {
        do
        {
            if ((*Entry & FAT_CLUSTER_LAST) != ++(*Index))
                return (*Entry & FAT_CLUSTER_LAST);
        } while (++Entry < EndOfPage);
        /* Check if this is the last available entry */
        if (FatPinIsLastPage(Context))
            break;
        Entry = (PULONG) FatPinNextPage(Context);
        EndOfPage = FatPinEndOfPage(Context, PULONG);
    }
    return (*Index - 1);
}

ULONG
FatSetFat32ContinousRun(IN OUT PFAT_SCAN_CONTEXT Context,
                        IN ULONG Index,
                        IN ULONG Length,
                        IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatScanFat32ForValueRun(IN OUT PFAT_SCAN_CONTEXT Context,
                        IN OUT PULONG Index,
                        IN ULONG IndexValue,
                        IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatSetFat32ValueRun(IN OUT PFAT_SCAN_CONTEXT Context,
                    IN ULONG Index,
                    IN ULONG Length,
                    IN ULONG IndexValue,
                    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

/**
 * Queries file MCB for the specified region [Vbo, Vbo + Length],
 * returns the number of runs in the region as well as the first
 * run of the range itself.
 * If the specified region is not fully cached in MCB the routine
 * scans FAT for the file and fills the MCB until the file offset
 * (defined as Vbo + Length) is reached.
 *
 * @param Fcb
 * Pointer to FCB structure for the file.
 *
 * @param Vbo
 * Virtual Byte Offset in the file.
 *
 * @param Lbo
 * Receives the Value of Logical Byte offset corresponding
 * to supplied Vbo Value.
 *
 * @param Length
 * Supplies file range length to be examined and receives
 * the length of first run.
 *
 * @param OutIndex
 * Receives the index (in MCB cache) of first run.
 *
 * @return
 * Incremented index of the last run (+1).
 *
 * @note
 * Should be called by I/O routines to split the I/O operation
 * into sequential or parallel I/O operations.
 */
ULONG
FatScanFat(IN PFCB Fcb,
           IN LONGLONG Vbo,
           OUT PLONGLONG Lbo,
           IN OUT PLONGLONG Length,
           OUT PULONG Index,
           IN BOOLEAN CanWait)
{
    LONGLONG CurrentLbo, CurrentVbo, BeyondLastVbo, CurrentLength;
    ULONG Entry, NextEntry, NumberOfEntries, CurrentIndex;
    FAT_PAGE_CONTEXT Context;
    PVCB Vcb;

    /* Some often used values */
    Vcb = Fcb->Vcb;
    CurrentIndex = 0;
    BeyondLastVbo = Vbo + *Length;
    CurrentLength = ((LONGLONG) Vcb->Clusters) << Vcb->BytesPerClusterLog;
    if (BeyondLastVbo > CurrentLength) 
        BeyondLastVbo = CurrentLength;
    /* Try to locate first run */
    if (FsRtlLookupLargeMcbEntry(&Fcb->Mcb, Vbo, Lbo, Length, NULL, NULL, Index))
    {
        /* Check if we have a single mapped run */
        if (Vbo >= BeyondLastVbo)
            goto FatScanFcbFatExit;
    } else {
        *Length = 0L;
    }
    /* Get the first scan startup values */
    if (FsRtlLookupLastLargeMcbEntryAndIndex(
        &Fcb->Mcb, &CurrentVbo, &CurrentLbo, &CurrentIndex))
    {
        Entry = FatDataOffsetToEntry(CurrentLbo, Vcb);
    }
    else
    {
        /* Map is empty, set up initial values */
        Entry = Fcb->FirstCluster;
        if (Entry <= 0x2)
            ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
        if (Entry >= Vcb->Clusters)
        {
            if (Entry < FAT_CLUSTER_LAST)
                ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
                BeyondLastVbo = 0LL;
        }
        CurrentIndex = 0L;
        CurrentVbo = 0LL;
    }
    /* Initialize Context */
    RtlZeroMemory(&Context, sizeof(Context));
    Context.FileObject = Vcb->StreamFileObject;
    Context.EndOfData.QuadPart = Vcb->BeyondLastClusterInFat;

    while (CurrentVbo < BeyondLastVbo)
    {
        /* Locate Continous run starting with the current entry */
        NumberOfEntries = Entry;
        NextEntry = Vcb->Methods.ScanContinousRun(
        &Context, &NumberOfEntries, CanWait);
        NumberOfEntries -= Entry;
        /* Check value that terminated the for being valid for FAT */
        if (NextEntry <= 0x2)
            ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
        if (NextEntry >= Vcb->Clusters)
        {
            if (NextEntry < FAT_CLUSTER_LAST)
            ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
            break;
        }
       /* Add new run */
        CurrentLength = ((LONGLONG) NumberOfEntries) 
            << Vcb->BytesPerClusterLog;
        FsRtlAddLargeMcbEntry(&Fcb->Mcb,
            CurrentVbo,
            FatEntryToDataOffset(Entry, Vcb),
            CurrentLength);
        /* Setup next iteration */
        Entry = NextEntry;
        CurrentVbo += CurrentLength;
        CurrentIndex ++;
    }
    if (*Length == 0LL && CurrentIndex > 0)
    {
        if (!FsRtlLookupLargeMcbEntry(&Fcb->Mcb,
            Vbo, Lbo, Length, NULL, NULL, Index))
        {
            *Index = 0L;
            *Lbo = 0LL;
        }
    }
FatScanFcbFatExit:
    return CurrentIndex;
}

BOOLEAN
NTAPI
FatValidBpb(IN PBIOS_PARAMETER_BLOCK Bpb)
{
    return (FatValidBytesPerSector(Bpb->BytesPerSector)
        && FatValidSectorsPerCluster(Bpb->SectorsPerCluster)
        && Bpb->ReservedSectors > 0
        && Bpb->Fats > 0
        && (Bpb->Sectors > 0 || Bpb->LargeSectors > 0)
        && (Bpb->SectorsPerFat > 0
            || (Bpb->LargeSectorsPerFat > 0 && Bpb->FsVersion == 0))
        && (Bpb->Media == 0xf0
            || Bpb->Media == 0xf8
            || Bpb->Media == 0xf9
            || Bpb->Media == 0xfb
            || Bpb->Media == 0xfc
            || Bpb->Media == 0xfd
            || Bpb->Media == 0xfe
            || Bpb->Media == 0xff)
        && (Bpb->SectorsPerFat == 0 || Bpb->RootEntries > 0)
        && (Bpb->SectorsPerFat > 0 || !Bpb->MirrorDisabled));
}

VOID
FatiInitializeVcb(PVCB Vcb)
{
    ULONG ClustersCapacity;

    /* Various characteristics needed for navigation in FAT */
    if ((Vcb->Sectors = Vcb->Bpb.Sectors) == 0)
        Vcb->Sectors = Vcb->Bpb.LargeSectors;
    if ((Vcb->SectorsPerFat = Vcb->Bpb.SectorsPerFat) == 0)
        Vcb->SectorsPerFat = Vcb->Bpb.LargeSectorsPerFat;
    Vcb->RootDirent = Vcb->Bpb.ReservedSectors + Vcb->Bpb.Fats * Vcb->SectorsPerFat;
    Vcb->RootDirentSectors = BytesToSectors(Vcb,
        Vcb->Bpb.RootEntries * sizeof(DIR_ENTRY));
    Vcb->DataArea = Vcb->RootDirent + Vcb->RootDirentSectors;
    Vcb->Clusters = (Vcb->Sectors - Vcb->Bpb.ReservedSectors
        - Vcb->Bpb.Fats * Vcb->SectorsPerFat
        - Vcb->RootDirentSectors) / Vcb->Bpb.SectorsPerCluster;
    if (Vcb->BytesPerClusterLog < 4087)
    {
        Vcb->IndexDepth = 0x0c;
        Vcb->Methods = Fat12Methods;
    }
    else
    {
        Vcb->IndexDepth = 0x10;
        Vcb->Methods = Fat16Methods;
    }
    /* Large Sectors are used for FAT32 */
    if (Vcb->Bpb.Sectors == 0) {
        Vcb->IndexDepth = 0x20;
        Vcb->Methods = Fat32Methods;
    }
    ClustersCapacity = (SectorsToBytes(Vcb, Vcb->Sectors) * 0x8 / Vcb->IndexDepth) - 1;
    if (Vcb->Clusters > ClustersCapacity)
        Vcb->Clusters = ClustersCapacity;
    Vcb->BytesPerCluster = SectorsToBytes(Vcb, Vcb->Bpb.SectorsPerCluster);
    Vcb->BytesPerClusterLog = FatPowerOfTwo(Vcb->BytesPerCluster);
    Vcb->BeyondLastClusterInFat = ((LONGLONG) Vcb->Clusters) * Vcb->IndexDepth / 0x8;

    /* Update real volume size with the real value. */
    Vcb->Header.FileSize.QuadPart =
    Vcb->Header.AllocationSize.QuadPart = SectorsToBytes(Vcb, Vcb->Sectors);
}

NTSTATUS
FatInitializeVcb(IN PVCB Vcb,
                 IN PDEVICE_OBJECT TargetDeviceObject,
                 IN PVPB Vpb)
{
    NTSTATUS Status;
    PBCB Bcb;
    PVOID Buffer;
    LARGE_INTEGER Offset;

    RtlZeroMemory(Vcb, sizeof(*Vcb));

    /* Initialize list head, so that it will
     * not fail in cleanup.
     */
    InitializeListHead(&Vcb->VcbLinks);

    /* Setup FCB Header */
    Vcb->Header.NodeTypeCode = FAT_NTC_VCB;
    Vcb->Header.NodeByteSize = sizeof(*Vcb);

    /* Setup Vcb fields */
    Vcb->TargetDeviceObject = TargetDeviceObject;
    ObReferenceObject(TargetDeviceObject);
    Vcb->Vpb = Vpb;

    /* Setup FCB Header */
    ExInitializeFastMutex(&Vcb->HeaderMutex);
    FsRtlSetupAdvancedHeader(&Vcb->Header, &Vcb->HeaderMutex);

    /* Create Volume File Object */
    Vcb->StreamFileObject = IoCreateStreamFileObject(NULL,
                                                     Vcb->TargetDeviceObject);

    /* We have to setup all FCB fields needed for CC */
    Vcb->StreamFileObject->FsContext = Vcb;
    Vcb->StreamFileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;

    /* At least full boot sector should be available */
    Vcb->Header.FileSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
    Vcb->Header.AllocationSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
    Vcb->Header.ValidDataLength.HighPart = MAXLONG;
    Vcb->Header.ValidDataLength.LowPart = MAXULONG;

    /* Initialize CC */
    CcInitializeCacheMap(Vcb->StreamFileObject,
                         (PCC_FILE_SIZES)&Vcb->Header.AllocationSize,
                         FALSE,
                         &FatGlobalData.CacheMgrNoopCallbacks,
                         Vcb);

    /* Read boot sector */
    Offset.QuadPart = 0;
    Bcb = NULL;

    /* Note: Volume Read path does not require
     * any of the parameters set further
     * in this routine.
     */
    if (CcMapData(Vcb->StreamFileObject,
                  &Offset,
                  sizeof(PACKED_BOOT_SECTOR),
                  TRUE,
                  &Bcb,
                  &Buffer))
    {
        PPACKED_BOOT_SECTOR BootSector = (PPACKED_BOOT_SECTOR) Buffer;
        FatUnpackBios(&Vcb->Bpb, &BootSector->PackedBpb);
        if (!(FatBootSectorJumpValid(BootSector->Jump) &&
            FatValidBpb(&Vcb->Bpb)))
        {
            Status = STATUS_UNRECOGNIZED_VOLUME;
        }
        CopyUchar4(&Vcb->Vpb->SerialNumber, BootSector->Id);
        CcUnpinData(Bcb);
    }
    else
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto FatInitializeVcbCleanup;
    }

    /* Set up notifications */
    FsRtlNotifyInitializeSync(&Vcb->NotifySync);
    InitializeListHead(&Vcb->NotifyList);

    /* Call helper function */
    FatiInitializeVcb(Vcb);

    /* Add this Vcb to grobal Vcb list. */
    InsertTailList(&FatGlobalData.VcbListHead, &Vcb->VcbLinks);
    return STATUS_SUCCESS;

FatInitializeVcbCleanup:

    /* Unwind the routine actions */
    FatUninitializeVcb(Vcb);
    return Status;
}

VOID
FatUninitializeVcb(IN PVCB Vcb)
{
    LARGE_INTEGER ZeroSize;

    ZeroSize.QuadPart = 0LL;

    /* Close volume file */
    if (Vcb->StreamFileObject != NULL)
    {
        /* Uninitialize CC. */
        CcUninitializeCacheMap(Vcb->StreamFileObject, &ZeroSize, NULL);
        ObDereferenceObject(Vcb->StreamFileObject);
        Vcb->StreamFileObject = NULL;
    }

    /* Free notifications stuff */
    FsRtlNotifyUninitializeSync(&Vcb->NotifySync);

    /* Unlink from global Vcb list. */
    RemoveEntryList(&Vcb->VcbLinks);

    /* Release Target Device */
    ObDereferenceObject(Vcb->TargetDeviceObject);
    Vcb->TargetDeviceObject = NULL;
}

/* EOF */


