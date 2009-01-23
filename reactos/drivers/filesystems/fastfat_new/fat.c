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
    LONGLONG BeyoundLastEntryOffset;
    PVOID PageBuffer;
    PBCB PageBcb;
} FAT_SCAN_CONTEXT;

#define FatEntryToDataOffset(xEntry, xVcb) \
    ((xVcb)->DataArea + (((LONGLONG) ((xEntry) - 0x02)) << (xVcb)->BytesPerClusterLog))

#define FatDataOffsetToEntry(xOffset, xVcb) \
    ((ULONG) ((xOffset - (xVcb)->DataArea) >> (xVcb)->BytesPerClusterLog) + 0x02)

ULONG
FatScanFat12ForContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN BOOLEAN CanWait);

ULONG
FatSetFat12ContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN BOOLEAN CanWait);

ULONG
FatScanFat12ForValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait);

ULONG
FatSetFat12ValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait);

ULONG
FatScanFat16ForContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN BOOLEAN CanWait);

ULONG
FatSetFat16ContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN BOOLEAN CanWait);

ULONG
FatScanFat16ForValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait);

ULONG
FatSetFat16ValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait);

ULONG
FatScanFat32ForContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN BOOLEAN CanWait);

ULONG
FatSetFat32ContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN BOOLEAN CanWait);

ULONG
FatScanFat32ForValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait);

ULONG
FatSetFat32ValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait);

BOOLEAN
NTAPI
FatValidBpb(
    IN PBIOS_PARAMETER_BLOCK Bpb);

/* VARIABLES ****************************************************************/
FAT_METHODS Fat12Methods = {
    FatScanFat12ForContinousRun,
    FatSetFat12ContinousRun,
    FatScanFat16ForValueRun,
    FatSetFat12ValueRun
};

FAT_METHODS Fat16Methods = {
    FatScanFat16ForContinousRun,
    FatSetFat16ContinousRun,
    FatScanFat16ForValueRun,
    FatSetFat16ValueRun
};

FAT_METHODS Fat32Methods = {
    FatScanFat32ForContinousRun,
    FatSetFat32ContinousRun,
    FatScanFat32ForValueRun,
    FatSetFat32ValueRun
};

/* FUNCTIONS ****************************************************************/
FORCEINLINE
ULONG
FatPowerOfTwo(
    ULONG Number)
/*
 * FUNCTION:
 *      Determines the index of the set bit.
 * ARGUMENTS:
 *      Number = Number having a single bit set.
 * RETURNS: Index of the set bit.
 */
{
    ULONG Temp;
    Temp = Number
        - ((Number >> 1) & 033333333333)
        - ((Number >> 2) & 011111111111);
    return (((Temp + (Temp >> 3)) & 030707070707) % 63);
}

ULONG
FatScanFat12ForContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatSetFat12ContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatScanFat12ForValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatSetFat12ValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatScanFat16ForContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatSetFat16ContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatScanFat16ForValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatSetFat16ValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatScanFat32ForContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN BOOLEAN CanWait)
/*
 * FUNCTION:
 *       Scans FAT32 for continous chain of clusters
 * ARGUMENTS:
 *       Context = Pointer to FAT_SCAN_CONTEXT.
 *       Index = Supplies the Index of the first cluster
 *          and receves the last index after the last
 *          cluster in the chain.
 *       CanWait = Indicates if the context allows blocking.
 * RETURNS: Value of the last claster terminated the scan.
 * NOTES: Raises STATUS_CANT_WAIT race condition.
 */
{
    LONGLONG PageOffset;
    SIZE_T OffsetWithinPage, PageValidLength;
    PULONG Entry, BeyoudLastEntry;
   /*
    * Determine page offset and the offset within page
    * for the first cluster.
    */
    PageValidLength = PAGE_SIZE;
    PageOffset = ((LONGLONG) *Index) << 0x2;
    OffsetWithinPage = (SIZE_T) (PageOffset & (PAGE_SIZE - 1));
    PageOffset -= OffsetWithinPage;
   /*
    * Check if the context already has the required page mapped.
    * Map the first page is necessary.
    */
    if (PageOffset != Context->PageOffset.QuadPart)
    {
        Context->PageOffset.QuadPart = PageOffset;
        if (Context->PageBcb != NULL)
        {
            CcUnpinData(Context->PageBcb);
            Context->PageBcb = NULL;
        }
        if (!CcMapData(Context->FileObject, &Context->PageOffset,
            PAGE_SIZE, CanWait, &Context->PageBcb, &Context->PageBuffer))
        {        
            Context->PageOffset.QuadPart = 0LL;
            ExRaiseStatus(STATUS_CANT_WAIT);
        }
    }
    Entry = Add2Ptr(Context->PageBuffer, OffsetWithinPage, PULONG);
   /*
    * Next Page Offset.
    */
    PageOffset = Context->PageOffset.QuadPart + PAGE_SIZE;
    if (PageOffset > Context->BeyoundLastEntryOffset)
        PageValidLength = (SIZE_T) (Context->BeyoundLastEntryOffset
            - Context->PageOffset.QuadPart);
    BeyoudLastEntry = Add2Ptr(Context->PageBuffer, PageValidLength, PULONG);
    while (TRUE)
    {
        do
        {
            if ((*Entry & FAT_CLUSTER_LAST) != ++(*Index) )
                return (*Entry & FAT_CLUSTER_LAST);
        }
        while (++Entry < BeyoudLastEntry);
       /*
        * Check if this is the last available entry.
        */
        if (PageValidLength < PAGE_SIZE)
            break;
       /*
        * We are getting beyound current page and
        * are still in the continous run, map the next page.
        */
        Context->PageOffset.QuadPart = PageOffset;
        CcUnpinData(Context->PageBcb);
        if (!CcMapData(Context->FileObject,
            &Context->PageOffset, PAGE_SIZE, CanWait,
            &Context->PageBcb, &Context->PageBuffer))
        {
            Context->PageBcb = NULL;
            Context->PageOffset.QuadPart = 0LL;
            ExRaiseStatus(STATUS_CANT_WAIT);
        }
        Entry = (PULONG) Context->PageBuffer;
       /*
        * Next Page Offset.
        */
        PageOffset = Context->PageOffset.QuadPart + PAGE_SIZE;
        if (PageOffset > Context->BeyoundLastEntryOffset)
            PageValidLength = (SIZE_T) (Context->BeyoundLastEntryOffset
                - Context->PageOffset.QuadPart);
        BeyoudLastEntry = Add2Ptr(Context->PageBuffer, PageValidLength, PULONG);
    }
    return (*Index - 1);
}

ULONG
FatSetFat32ContinousRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatScanFat32ForValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN OUT PULONG Index,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatSetFat32ValueRun(
    IN OUT PFAT_SCAN_CONTEXT Context,
    IN ULONG Index,
    IN ULONG Length,
    IN ULONG IndexValue,
    IN BOOLEAN CanWait)
{
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}

ULONG
FatScanFat(
    IN PFCB Fcb,
    IN LONGLONG Vbo,
    OUT PLONGLONG Lbo,
    IN OUT PLONGLONG Length,
    OUT PULONG Index,
    IN BOOLEAN CanWait
)
/*
 * FUNCTION:
 *      Queries file MCB for the specified region [Vbo, Vbo + Length],
 *      returns the number of runs in the region as well as the first
 *      run of the range itself.
 *      If the specified region is not fully cached in MCB the routine
 *      scans FAT for the file and fills the MCB until the file offset
 *      (defined as Vbo + Length) is reached.
 * ARGUMENTS:
 *      Fcb = Pointer to FCB structure for the file.
 *      Vbo = Virtual Byte Offset in the file.
 *      Lbo = Receives the Value of Logical Byte offset corresponding
 *            to supplied Vbo Value.
 *      Length = Supplies file range length to be examined and recieves
 *            the length of first run.
 *      OutIndex = Recieves the index (in MCB cache) of first run.
 * RETURNS: Incremented index of the last run (+1).
 * NOTES: Should be called by I/O routines to split the I/O operation
 *      into sequential or parallel I/O operations.
 */
{
    LONGLONG CurrentLbo, CurrentVbo, BeyoundLastVbo, CurrentLength;
    ULONG Entry, NextEntry, NumberOfEntries, CurrentIndex;
    FAT_SCAN_CONTEXT Context;
    PVCB Vcb;

   /*
    * Some often used values
    */
    Vcb = Fcb->Vcb;
    CurrentIndex = 0;
    BeyoundLastVbo = Vbo + *Length;
    CurrentLength = ((LONGLONG) Vcb->Clusters) << Vcb->BytesPerClusterLog;
    if (BeyoundLastVbo > CurrentLength) 
        BeyoundLastVbo = CurrentLength;
   /*
    * Try to locate first run.
    */
    if (FsRtlLookupLargeMcbEntry(&Fcb->Mcb, Vbo, Lbo, Length, NULL, NULL, Index))
    {
       /*
        * Check if we have a single mapped run.
        */
        if (Vbo >= BeyoundLastVbo)
            goto FatScanFcbFatExit;
    } else {
        *Length = 0L;
    }
   /*
    * Get the first scan startup values.
    */
    if (FsRtlLookupLastLargeMcbEntryAndIndex(
        &Fcb->Mcb, &CurrentVbo, &CurrentLbo, &CurrentIndex))
    {
        Entry = FatDataOffsetToEntry(CurrentLbo, Vcb);
    }
    else
    {
       /*
        * Map is empty, set up initial values.
        */
        Entry = Fcb->FirstCluster;
        if (Entry <= 0x2)
            ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
        if (Entry >= Vcb->Clusters)
        {
            if (Entry < FAT_CLUSTER_LAST)
                ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
                BeyoundLastVbo = 0LL;
        }
        CurrentIndex = 0L;
        CurrentVbo = 0LL;
    }
    RtlZeroMemory(&Context, sizeof(Context));
    Context.FileObject = Vcb->VolumeFileObject;
    while (CurrentVbo < BeyoundLastVbo)
    {
       /*
        * Locate Continous run starting with the current entry.
        */
        NumberOfEntries = Entry;
        NextEntry = Vcb->Methods.ScanContinousRun(
        &Context, &NumberOfEntries, CanWait);
        NumberOfEntries -= Entry;
       /*
        * Check value that terminated the for being valid for FAT.
        */
        if (NextEntry <= 0x2)
            ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
        if (NextEntry >= Vcb->Clusters)
        {
            if (NextEntry < FAT_CLUSTER_LAST)
            ExRaiseStatus(STATUS_FILE_CORRUPT_ERROR);
            break;
        }
       /*
        * Add new run.
        */
        CurrentLength = ((LONGLONG) NumberOfEntries) 
            << Vcb->BytesPerClusterLog;
        FsRtlAddLargeMcbEntry(&Fcb->Mcb,
            CurrentVbo,
            FatEntryToDataOffset(Entry, Vcb),
            CurrentLength);
       /*
        * Setup next iteration.
        */
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
FatValidBpb(
    IN PBIOS_PARAMETER_BLOCK Bpb)
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
FatiInitializeVcb(
    PVCB Vcb)
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
    Vcb->BeyoundLastClusterInFat = ((LONGLONG) Vcb->Clusters) * Vcb->IndexDepth / 0x8;
}

NTSTATUS
FatInitializeVcb(
    IN PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb)
{
    NTSTATUS Status;
    PBCB Bcb;
    PVOID Buffer;
    LARGE_INTEGER Offset;

    RtlZeroMemory(Vcb, sizeof(*Vcb));

    /*
    * Initialize list head, so that it will
    * not fail in cleanup.
    */
    InitializeListHead(&Vcb->VcbLinks);

    /* Setup FCB Header. */
    Vcb->Header.NodeTypeCode = FAT_NTC_VCB;
    Vcb->Header.NodeByteSize = sizeof(*Vcb);

    /* Setup Vcb fields */
    Vcb->TargetDeviceObject = TargetDeviceObject;
    ObReferenceObject(TargetDeviceObject);

    /* Setup FCB Header. */
    ExInitializeFastMutex(&Vcb->HeaderMutex);
    FsRtlSetupAdvancedHeader(&Vcb->Header, &Vcb->HeaderMutex);

    /* Create Volume File Object. */
    Vcb->VolumeFileObject = IoCreateStreamFileObject(NULL,
        Vcb->TargetDeviceObject);

    /* We have to setup all FCB fields needed for CC. */
    Vcb->VolumeFileObject->FsContext = Vcb;
    Vcb->VolumeFileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;


    /* At least full boot sector should be available. */
    Vcb->Header.FileSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
    Vcb->Header.AllocationSize.QuadPart = sizeof(PACKED_BOOT_SECTOR);
    Vcb->Header.ValidDataLength.HighPart = MAXLONG;
    Vcb->Header.ValidDataLength.LowPart = MAXULONG;

    /* Initialize CC. */
    CcInitializeCacheMap(Vcb->VolumeFileObject,
        (PCC_FILE_SIZES) &Vcb->Header.AllocationSize,
        FALSE,
        &FatGlobalData.CacheMgrNoopCallbacks,
        Vcb);

    /* Read boot sector */
	Offset.QuadPart = 0;
	Bcb = NULL;
   /*
    * Note: Volume Read path does not require
    * any of the parameters set further
    * in this routine.
    */
    if (CcMapData(Vcb->VolumeFileObject,
            &Offset,
            sizeof(PACKED_BOOT_SECTOR),
            TRUE,
            &Bcb,
            &Buffer))
    {
        PPACKED_BOOT_SECTOR BootSector = (PPACKED_BOOT_SECTOR) Buffer;
        FatUnpackBios(&Vcb->Bpb, &BootSector->PackedBpb);
        if (!(FatBootSectorJumpValid(BootSector->Jump)
            && FatValidBpb(&Vcb->Bpb)))
        {
            Status = STATUS_UNRECOGNIZED_VOLUME;
        }
        CopyUchar4( &Vpb->SerialNumber, BootSector->Id );
        CcUnpinData(Bcb);
    }
    else
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
        goto FatInitializeVcbCleanup;
    }
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
FatUninitializeVcb(
    IN PVCB Vcb)
{
    LARGE_INTEGER ZeroSize;

    ZeroSize.QuadPart = 0LL;

    /* Close volume file */
    if (Vcb->VolumeFileObject != NULL)
    {
        /* Uninitialize CC. */
        CcUninitializeCacheMap(Vcb->VolumeFileObject, &ZeroSize, NULL);
        ObDereferenceObject(Vcb->VolumeFileObject);
        Vcb->VolumeFileObject = NULL;
    }
    /* Unlink from global Vcb list. */
    RemoveEntryList(&Vcb->VcbLinks);

    /* Release Target Device */
    ObReferenceObject(Vcb->TargetDeviceObject);
    Vcb->TargetDeviceObject = NULL;
}

/* EOF */


