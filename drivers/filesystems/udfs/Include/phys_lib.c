////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
 Module Name: Phys_lib.cpp

 Execution: Kernel mode only

 Description:

   Contains code that implement read/write operations for physical device
*/

#include "phys_lib.h"

#define MSF_TO_LBA(Minutes,Seconds,Frames) \
                (ULONG)((60 * 75 * (Minutes)) + (75 * (Seconds)) + ((Frames) - 150))

#define TrkInfo_Dat_Mask    0x0F
#define TrkInfo_Dat_Mode1   0x01
#define TrkInfo_Dat_Mode2   0x02
#define TrkInfo_Dat_XA      0x02
#define TrkInfo_Dat_DDCD    0x02
#define TrkInfo_Dat_unknown 0x0F
#define TrkInfo_Dat_Unknown TrkInfo_Dat_unknown
#define TrkInfo_FP          0x10
#define TrkInfo_Packet      0x20
#define TrkInfo_Blank       0x40
#define TrkInfo_RT          0x80


#define Trk_QSubChan_Type_Audio            0x00
#define Trk_QSubChan_Type_AllowCpy         0x02
#define Trk_QSubChan_Type_Data             0x04
#define Trk_QSubChan_Type_IncrData         0x05
#define Trk_QSubChan_Type_Mask             0x0d

#define WParam_TrkMode_Mask             Trk_QSubChan_Type_Mask           //0x0d
#define WParam_TrkMode_Data             Trk_QSubChan_Type_Data           //0x04
#define WParam_TrkMode_IncrData         Trk_QSubChan_Type_IncrData       //0x05
#define TrkInfo_Trk_unknown 0x0F


#define TrkInfo_Trk_XA      (Trk_QSubChan_Type_Audio | Trk_QSubChan_Type_AllowCpy)

#define TocControl_TrkMode_Mask           WParam_TrkMode_Mask
#define TocControl_TrkMode_Data           WParam_TrkMode_Data
#define TocControl_TrkMode_IncrData       WParam_TrkMode_IncrData


#define DEFAULT_LAST_LBA_FP_CD  276159
#define TOC_LastTrack_ID        0xAA
#define MediaType_UnknownSize_CDRW 0x20

#define UDF_MAX_SECTOR_IO_BYTES  0x100000 // 1 MB which is a multiple of every sector size

// Local functions:

uint32
UDFFixFPAddress(
    IN PVCB           Vcb,               // Volume control block from this DevObj
    IN uint32         Lba
    );

NTSTATUS
UDFReallocTrackMap(
    IN PVCB Vcb,
    IN uint32 TrackNum
    )
{
    if (Vcb->TrackMap) {
        MyFreePool__(Vcb->TrackMap);
        Vcb->TrackMap = NULL;
    }
    Vcb->TrackMap = (PUDFTrackMap)
        MyAllocatePool__(NonPagedPool, TrackNum*sizeof(UDFTrackMap));
    if (!Vcb->TrackMap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Vcb->TrackMap,TrackNum*sizeof(UDFTrackMap));
    return STATUS_SUCCESS;
} // end UDFReallocTrackMap()

/*************************************************************************
*
*  Function: UDFSendSectorIoChunk()
*
*  Description:
*    Build and send a single read/write IRP (capped at the chunk size) to the target device.
*    Waits for completion synchronously.  Handles MDL and IRP cleanup.
*
*************************************************************************/

static
NTSTATUS
UDFSendSectorIoChunk(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID Buffer,
    IN ULONG ByteCount,
    IN LONGLONG Offset,
    IN BOOLEAN IsWrite
    )
{
    KEVENT Event;
    LARGE_INTEGER ROffset;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    PAGED_CODE();

    ROffset.QuadPart = Offset;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildAsynchronousFsdRequest(
        IsWrite ? IRP_MJ_WRITE : IRP_MJ_READ,
        TargetDeviceObject,
        Buffer,
        ByteCount,
        &ROffset,
        NULL);

    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(Irp,
                           &UDFHijackCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    IrpSp = IoGetNextIrpStackLocation(Irp);

    SetFlag(IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME);

    if (IsWrite && FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {
        SetFlag(IrpSp->Flags, SL_WRITE_THROUGH);
    }

    Status = IoCallDriver(TargetDeviceObject, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Irp->IoStatus.Status;
    }

    /* A successful IRP with fewer bytes than requested did not transfer the
     * complete sector range.  Do not report that metadata was persisted. */
    if (NT_SUCCESS(Status) &&
        (!NT_SUCCESS(Irp->IoStatus.Status) ||
         Irp->IoStatus.Information != ByteCount)) {
        Status = NT_SUCCESS(Irp->IoStatus.Status) ?
                 STATUS_IO_DEVICE_ERROR : Irp->IoStatus.Status;
    }

    //  Free MDL chain.

    while (Irp->MdlAddress != NULL) {
        PMDL NextMdl = Irp->MdlAddress->Next;
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NextMdl;
    }

    IoFreeIrp(Irp);

    return Status;
} // end UDFSendSectorIoChunk()

/*************************************************************************
*
*  Function: UDFSendSectorIo()
*
*  Description:
*    Build and send a read/write IRP to the target device as a series of chunks.
*    Waits for completion synchronously.  Handles MDL and IRP cleanup.
*
*    Sending as chunks because, the Mdl is allocated with IoAllocateMdl by 
*    IoBuildAsynchronousFsdRequest, which fails above ~64 MB on x86 and ~32 MB 
*    on amd64 on ReactOS and NT 5.x.
*
*************************************************************************/

static
NTSTATUS
UDFSendSectorIo(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVOID Buffer,
    IN ULONG ByteCount,
    IN LONGLONG Offset,
    IN BOOLEAN IsWrite
    )
{
    PCHAR BufPtr = Buffer;
    ULONG ChunkLen = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    while (ByteCount > 0) {
        ChunkLen = min(ByteCount, UDF_MAX_SECTOR_IO_BYTES);
        Status = UDFSendSectorIoChunk(IrpContext, TargetDeviceObject, BufPtr, ChunkLen, Offset, IsWrite);
        
        if (!NT_SUCCESS(Status)) {

            return Status;
        }
        
        BufPtr += ChunkLen;
        Offset += ChunkLen;
        ByteCount -= ChunkLen;
    }

    return Status;
} // end UDFSendSectorIo()


/*************************************************************************
*
*  Function: UDFReadWriteSectors()
*
*  Description:
*    Single entry point for all metadata sector I/O (read and write).
*    Handles sparing table relocation, fixed-packet address translation,
*    bad sector bitmap checks, and CDR_Mode NWA tracking.
*
*    For file data I/O, use UDFNonCachedIo() in deviosup.cpp instead.
*
*************************************************************************/

static NTSTATUS
UDFReadWriteSectorsRaw(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN BOOLEAN ReturnError,
    IN PVOID Buffer,
    IN BOOLEAN IsWrite
    )
{
    NTSTATUS RC = STATUS_SUCCESS;
    PEXTENT_MAP RelocExtent;
    PEXTENT_MAP RelocExtent_saved = NULL;

    uint32 Lba = (uint32)(StartingOffset >> Vcb->SectorShift);
    uint32 BCount = ByteCount >> Vcb->SectorShift;
    uint32 rLba;

    ASSERT(Buffer);

    if (Vcb->VcbState & UDF_VCB_FLAGS_DEAD) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    //  For writes in CDR_Mode, use NWA instead of the given LBA.
    //  For all other cases, apply sparing relocation.
    //

    if (IsWrite && Vcb->CDR_Mode) {
        RelocExtent = UDF_NO_EXTENT_MAP;
        rLba = Vcb->NWA;
    } else {
        RelocExtent = UDFRelocateSectors(Vcb, Lba, BCount);
        if (!RelocExtent) return STATUS_INSUFFICIENT_RESOURCES;
        rLba = Lba;
    }

    _SEH2_TRY {

        if (RelocExtent == UDF_NO_EXTENT_MAP) {

            //
            //  No relocation — single contiguous I/O.
            //

            if (!IsWrite) {

                //  Read beyond end of recorded area returns zeros.

                if (rLba >= (Vcb->CDR_Mode ? Vcb->NWA : Vcb->SessionEndLba + 1)) {
                    RtlZeroMemory(Buffer, ByteCount);
                    try_return(RC = STATUS_SUCCESS);
                }

                RC = UDFPrepareForReadOperation(IrpContext, Vcb, rLba, BCount);
                if (!NT_SUCCESS(RC)) try_return(RC);

                rLba = UDFFixFPAddress(Vcb, rLba);

                RC = UDFSendSectorIo(IrpContext,
                         Vcb->TargetDeviceObject, Buffer, ByteCount,
                         ((uint64)rLba) << Vcb->SectorShift, FALSE);

                Vcb->VcbState &= ~UDF_VCB_LAST_WRITE;

            } else {

                RC = UDFPrepareForWriteOperation(Vcb, rLba, BCount);
                if (!NT_SUCCESS(RC)) try_return(RC);

                RC = UDFSendSectorIo(IrpContext,
                         Vcb->TargetDeviceObject, Buffer, ByteCount,
                         ((uint64)rLba) << Vcb->SectorShift, TRUE);
            }

            try_return(RC);
        }

        //
        //  Sectors were relocated — walk the relocation map.
        //

        RelocExtent_saved = RelocExtent;

        for (; RelocExtent->extLength; RelocExtent++) {

            rLba = RelocExtent->extLocation;
            uint32 FragLength = RelocExtent->extLength;
            uint32 FragBCount = FragLength >> Vcb->SectorShift;

            if (!IsWrite) {

                if (rLba >= (Vcb->CDR_Mode ? Vcb->NWA : Vcb->SessionEndLba + 1)) {
                    RtlZeroMemory(Buffer, FragLength);
                    *((uint32*)&Buffer) += FragLength;
                    continue;
                }

                RC = UDFPrepareForReadOperation(IrpContext, Vcb, rLba, FragBCount);
                if (!NT_SUCCESS(RC)) break;

                rLba = UDFFixFPAddress(Vcb, rLba);

                RC = UDFSendSectorIo(IrpContext,
                         Vcb->TargetDeviceObject, Buffer, FragLength,
                         ((uint64)rLba) << Vcb->SectorShift, FALSE);

                Vcb->VcbState &= ~UDF_VCB_LAST_WRITE;

            } else {

                RC = UDFPrepareForWriteOperation(Vcb, rLba, FragBCount);
                if (!NT_SUCCESS(RC)) break;

                RC = UDFSendSectorIo(IrpContext,
                         Vcb->TargetDeviceObject, Buffer, FragLength,
                         ((uint64)rLba) << Vcb->SectorShift, TRUE);
            }

            if (!NT_SUCCESS(RC)) break;
            *((uint32*)&Buffer) += FragLength;
        }

try_exit: NOTHING;
    } _SEH2_FINALLY {
        if (RelocExtent_saved) {
            MyFreePool__(RelocExtent_saved);
        }
    } _SEH2_END;

    if (!NT_SUCCESS(RC) && !ReturnError) {
        ExRaiseStatus(RC);
    }

    return RC;
} // end UDFReadWriteSectorsRaw()

/* Sector I/O at the given physical address, with no metadata mirror or
 * copy-on-write handling. For cow.c, which manages both itself. */
NTSTATUS
UDFReadWriteSectorsDirect(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN PVOID Buffer,
    IN BOOLEAN IsWrite
    )
{
    return UDFReadWriteSectorsRaw(IrpContext, Vcb, StartingOffset, ByteCount,
                                  TRUE, Buffer, IsWrite);
}

NTSTATUS
UDFReadWriteSectors(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG StartingOffset,
    IN ULONG ByteCount,
    IN BOOLEAN ReturnError,
    IN PVOID Buffer,
    IN BOOLEAN IsWrite
    )
{
    NTSTATUS status;
    uint32 firstLba, count, i, metadataLbn, mirrorLba, mirrorFlags;
    PEXTENT_MAP mirrorMap;

    /* With copy-on-write, metadata sectors are buffered and redirected to
     * the committed region; cow.c also writes the mirror copies. */
    if (Vcb->Cow && UDFCowIo(IrpContext, Vcb, StartingOffset, ByteCount,
                             Buffer, IsWrite, &status)) {
        if (!NT_SUCCESS(status) && !ReturnError)
            ExRaiseStatus(status);
        return status;
    }

    // Write the primary copy before its mirror, including sector updates
    // made by the file-entry and directory paths.  The mirror has its own
    // physical allocation only when the duplicate flag is set.
    status = UDFReadWriteSectorsRaw(IrpContext, Vcb, StartingOffset,
                                   ByteCount, ReturnError, Buffer, IsWrite);
    if (!NT_SUCCESS(status) || !IsWrite ||
        !(Vcb->MetadataFlags & UDF_METADATA_DUPLICATED) ||
        !Vcb->MetadataFileInfo || !Vcb->MetadataMirrorFileInfo)
        return status;

    mirrorMap = Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping;
    if (!mirrorMap || (StartingOffset & (Vcb->SectorSize - 1)) ||
        (ByteCount & (Vcb->SectorSize - 1)))
        return STATUS_FILE_CORRUPT_ERROR;

    firstLba = (uint32)(StartingOffset >> Vcb->SectorShift);
    count = ByteCount >> Vcb->SectorShift;
    for (i = 0; i < count; i++) {
        metadataLbn = UDFPhysLbaToMetadata(Vcb, firstLba + i);
        if (metadataLbn == LBA_OUT_OF_EXTENT)
            continue;

        mirrorLba = UDFExtentOffsetToLba(Vcb, mirrorMap,
                                         ((int64)metadataLbn) << Vcb->SectorShift,
                                         NULL, NULL, &mirrorFlags, NULL);
        if (mirrorLba == LBA_OUT_OF_EXTENT ||
            mirrorFlags != EXTENT_RECORDED_ALLOCATED) {
            status = STATUS_FILE_CORRUPT_ERROR;
            break;
        }

        status = UDFReadWriteSectorsRaw(IrpContext, Vcb,
                     ((LONGLONG)mirrorLba) << Vcb->SectorShift,
                     Vcb->SectorSize, ReturnError,
                     (PUCHAR)Buffer + (((SIZE_T)i) << Vcb->SectorShift), TRUE);
        if (!NT_SUCCESS(status))
            break;
    }

    if (!NT_SUCCESS(status)) {
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        if (!ReturnError)
            ExRaiseStatus(status);
    }
    return status;
} // end UDFReadWriteSectors()

/*
    This routine performs media-type dependent preparations
    for write operation.

    For CDR/RW it sets WriteParameters according to track parameters,
    in some cases issues SYNC_CACHE command.
    It can also send OPC info if requered.
    If write-requested block is located beyond last formatted LBA
    on incompletely formatted DVD media, this routine performs
    all neccessary formatting operations in order to satisfy
    subsequent write request.
 */
NTSTATUS
UDFPrepareForWriteOperation(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BCount
    )
{
    if (Vcb->BSBM_Bitmap) {
        ULONG i;
        uint32 lbn = Lba - Vcb->Partitions[0].PartitionRoot;
        for(i=0; i<BCount; i++) {
            if (UDFGetBit((uint32*)(Vcb->BSBM_Bitmap), lbn+i)) {
                UDFPrint(("W: Known BB @ %#x\n", Lba));
                //return STATUS_FT_WRITE_RECOVERY; // this shall not be treated as error and
                                                   // we shall get IO request to BAD block
                return STATUS_DEVICE_DATA_ERROR;
            }
        }
    }

    Vcb->VcbState |= UDF_VCB_LAST_WRITE;

    return STATUS_SUCCESS;
} // end UDFPrepareForWriteOperation()

/*
    use standard way to determine disk layout (ReadTOC cmd)
 */
NTSTATUS
UDFDetermineVolumeLayout(
    PIRP_CONTEXT IrpContext,
    PDEVICE_OBJECT DeviceObject,
    PVCB Vcb,
    PULONG SessionStartLba,
    PULONG SessionEndLba
    )
{
    NTSTATUS Status;
    CDROM_TOC_LARGE* toc = NULL;
    CDROM_TOC_SESSION_DATA* LastSes = NULL;
    ULONG LocalTrackCount;
    ULONG TocEntry;
    void* TempBuffer = NULL;
    ULONG OldTrkNum;
    ULONG TrkNum;
    SIZE_T i, len;

    *SessionStartLba = 0;
    *SessionEndLba = 0;

    //// Only process CD-ROM devices

    //if (DeviceObject->DeviceType != FILE_DEVICE_CD_ROM) {

    //    return STATUS_SUCCESS;
    //}

    toc = (CDROM_TOC_LARGE*)MyAllocatePool__(NonPagedPool, sizeof(CDROM_TOC_LARGE));
    LastSes = (CDROM_TOC_SESSION_DATA*)MyAllocatePool__(NonPagedPool, sizeof(CDROM_TOC_SESSION_DATA));

    _SEH2_TRY {

        if (!toc || !LastSes) {
            try_return (Status = STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(toc, sizeof(CDROM_TOC_LARGE));

        CDROM_READ_TOC_EX Command;

        RtlZeroMemory(&Command, sizeof(Command));

        Status = UDFPerformDevIoCtrl(IOCTL_CDROM_READ_TOC_EX,
                                DeviceObject,
                                &Command,
                                sizeof(Command),
                                toc,
                                sizeof(CDROM_TOC_LARGE),
                                TRUE,
                                NULL);

        if (!NT_SUCCESS(Status) && 
            (Status != STATUS_INSUFFICIENT_RESOURCES)) {

            // try using the MSF mode
            Command.Msf = 1;

            Status = UDFPerformDevIoCtrl(IOCTL_CDROM_READ_TOC_EX,
                                    DeviceObject,
                                    &Command,
                                    sizeof(Command),
                                    toc,
                                    sizeof(CDROM_TOC_LARGE),
                                    TRUE,
                                    NULL);
        }

        if (Status == STATUS_INSUFFICIENT_RESOURCES) {

            UDFRaiseStatus(IrpContext, Status);
        }

        // If even standard read toc does not work, then use default values
        if (!NT_SUCCESS(Status)) {

            Status = UDFReallocTrackMap(Vcb, 2);
            if (!NT_SUCCESS(Status)) {
                try_return(Status);
            }

            Vcb->LastSession=1;
            Vcb->FirstTrackNum=1;
            Vcb->LastTrackNum=1;
            Vcb->TrackMap[1].FirstLba = Vcb->SessionStartLba;
            Vcb->TrackMap[1].LastLba = Vcb->SessionEndLba;
            Vcb->TrackMap[1].PacketSize = PACKETSIZE_UDF;

            if (DeviceObject->DeviceType == FILE_DEVICE_DISK) {

                try_return(Status = STATUS_SUCCESS);
            }

            Vcb->TrackMap[1].DataParam = TrkInfo_Dat_XA | TrkInfo_FP | TrkInfo_Packet;
            Vcb->TrackMap[1].TrackParam = TrkInfo_Trk_XA;
            Vcb->TrackMap[1].NWA = 0xffffffff;
            Vcb->NWA = DEFAULT_LAST_LBA_FP_CD + 7 + 1;
            try_return(Status = STATUS_SUCCESS);
        }

        LocalTrackCount = toc->LastTrack - toc->FirstTrack + 1;

        // Get out if there is an immediate problem with the TOC.

        if (toc->LastTrack - toc->FirstTrack >= MAXIMUM_NUMBER_TRACKS_LARGE) {
            try_return(Status = STATUS_DISK_CORRUPT_ERROR);
        }

        Vcb->LastTrackNum = toc->LastTrack;
        Vcb->FirstTrackNum = toc->FirstTrack;
        // some devices report LastTrackNum=0 for full disks
        Vcb->LastTrackNum = max(Vcb->LastTrackNum, Vcb->FirstTrackNum);

        Status = UDFReallocTrackMap(Vcb, MAXIMUM_NUMBER_TRACKS_LARGE+1);

        if (!NT_SUCCESS(Status)) {
            BrutePoint();
            try_return(Status);
        }
        // find 1st and last session
        Status = UDFPerformDevIoCtrl(IOCTL_CDROM_GET_LAST_SESSION,
            DeviceObject,
            NULL,
            0,
            LastSes,
            sizeof(CDROM_TOC_SESSION_DATA),
            TRUE,
            NULL);

        if (NT_SUCCESS(Status) &&
            LastSes->FirstCompleteSession != LastSes->LastCompleteSession) {

            SwapCopyUchar4(SessionStartLba, &LastSes->TrackData[0].Address);

            // Validate: SessionStartLba must be greater than SessionEndLba

            if (*SessionEndLba <= *SessionStartLba) {

                *SessionStartLba = 0;
                *SessionEndLba = 0;
            }

            TrkNum = LastSes->TrackData[0].TrackNumber;

            *SessionStartLba = 0;
            SwapCopyUchar4(SessionStartLba, &LastSes->TrackData[0].Address);

            Vcb->LastSession = LastSes->FirstCompleteSession;
            for(TocEntry=0;TocEntry<LocalTrackCount + 1;TocEntry++) {
                if (toc->TrackData[TocEntry].TrackNumber == TrkNum) {
                    Vcb->TrackMap[TrkNum].Session = Vcb->LastSession;
                }
            }
        }

        OldTrkNum = 0;
        // Scan toc for first & last LBA
        for(TocEntry=0;TocEntry<LocalTrackCount + 1;TocEntry++) {
#define TempMSF toc->TrackData[TocEntry].Address
            TrkNum = toc->TrackData[TocEntry].TrackNumber;
#ifdef UDF_DBG
            if (TrkNum >= MAXIMUM_NUMBER_TRACKS_LARGE &&
                TrkNum != TOC_LastTrack_ID) {
                UDFPrint(("UDFUseStandard: Array out of bounds\n"));
                BrutePoint();
                try_return(Status = STATUS_SUCCESS);
            }
            UDFPrint(("Track N %d (0x%x) first LBA %ld (%lx) \n",TrkNum,TrkNum,
                MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3]),
                MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3])));
#endif // UDF_DBG
            if (TOC_LastTrack_ID == TrkNum) {

                if (Command.Msf) {
                    *SessionEndLba = MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3]) - 1;
                }
                else {
                    // The non-MSF (LBA) mode
                    *SessionEndLba = 0;
                    SwapCopyUchar4(SessionEndLba, &toc->TrackData[TocEntry].Address);
                    if (*SessionEndLba) {
                        *SessionEndLba -= 1;
                    }
                }

                Vcb->TrackMap[OldTrkNum].LastLba = *SessionEndLba;
                UDFPrint(("UDFUseStandard: Last track entry, break TOC scan\n"));
                break;
            } else {
                Vcb->TrackMap[TrkNum].FirstLba = MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3]);
                if (Vcb->TrackMap[TrkNum].FirstLba & 0x80000000)
                    Vcb->TrackMap[TrkNum].FirstLba = 0;
                if (TrkNum) {
                    if (TOC_LastTrack_ID == OldTrkNum) {
                        UDFPrint(("UDFUseStandard: Wrong previous track number\n"));
                        BrutePoint();
                    } else {
                        Vcb->TrackMap[OldTrkNum].LastLba = Vcb->TrackMap[TrkNum].FirstLba-1;
                    }
                }
            }
            // check track type
            switch(toc->TrackData[TocEntry].Control & TocControl_TrkMode_Mask) {
            case TocControl_TrkMode_Data:
            case TocControl_TrkMode_IncrData:
                Vcb->TrackMap[TrkNum].DataParam = TrkInfo_Dat_XA;
                Vcb->TrackMap[TrkNum].TrackParam = TrkInfo_Trk_XA;
                break;
            default:
                Vcb->TrackMap[TrkNum].DataParam = TrkInfo_Dat_unknown;
                Vcb->TrackMap[TrkNum].TrackParam = TrkInfo_Trk_unknown;
            }
            OldTrkNum = TrkNum;
#undef TempMSF
        }

        TrkNum = Vcb->LastTrackNum;
        Status = STATUS_SUCCESS;
        // find last _valid_ track
        for(;TrkNum;TrkNum--) {
            if ((Vcb->TrackMap[TrkNum].DataParam  != TrkInfo_Dat_unknown) &&
               (Vcb->TrackMap[TrkNum].TrackParam != TrkInfo_Trk_unknown)) {
                Status = STATUS_UNSUCCESSFUL;
                Vcb->LastTrackNum = TrkNum;
                break;
            }
        }
        // no valid tracks...
        if (!TrkNum) {
            UDFPrint(("UDFUseStandard: no valid tracks...\n"));
            try_return(Status = STATUS_UNRECOGNIZED_VOLUME);
        }
        i = 0;

        // Check for last Variable Packet(VP) track. Some last sectors may belong to Link-data &
        // be unreadable. We should forget about them, because UDF needs
        // last _readable_ sector.

        TempBuffer = MyAllocatePool__(NonPagedPool, Vcb->SectorSize);

        if (!TempBuffer) { 
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        while(!NT_SUCCESS(Status) && (i<8)) {
            Status = UDFReadWriteSectors(IrpContext, Vcb,
                       ((LONGLONG)(Vcb->TrackMap[TrkNum].LastLba-i)) << Vcb->SectorShift,
                       Vcb->SectorSize, TRUE, TempBuffer, FALSE);
            i++;
        }
        if (NT_SUCCESS(Status)) {
            *SessionEndLba = Vcb->TrackMap[TrkNum].LastLba-i+1;
/*            if (i) {
                Vcb->TrackMap[TrkNum].PacketSize = PACKETSIZE_UDF;
                Vcb->TrackMap[TrkNum].;
            }*/
        } else {

            // Check for Fixed Packet(FP) track. READ_TOC reports actual track length, but
            // Link-data is hidden & unreadable for us. So, available track
            // length may be less than actual. Here we assume that Packet-size
            // is PACKETSIZE_UDF.
            i = 0;
            len = Vcb->TrackMap[TrkNum].LastLba - Vcb->TrackMap[TrkNum].FirstLba + 1;
            len = (uint32)(((int64)len*PACKETSIZE_UDF) / (PACKETSIZE_UDF+7));

            while(!NT_SUCCESS(Status) && (i<9)) {
                Status = UDFReadWriteSectors(IrpContext, Vcb,
                           ((LONGLONG)(Vcb->TrackMap[TrkNum].FirstLba-i+len)) << Vcb->SectorShift,
                           Vcb->SectorSize, TRUE, TempBuffer, FALSE);
                i++;
            }
            if (NT_SUCCESS(Status)) {
                *SessionEndLba =
                Vcb->TrackMap[TrkNum].LastLba = Vcb->TrackMap[TrkNum].FirstLba-i+len+1;
                Vcb->TrackMap[TrkNum].PacketSize = PACKETSIZE_UDF;
//                Vcb->TrackMap[TrkNum].;
            } else
            if (Status == STATUS_INVALID_DEVICE_REQUEST) {
                // wrap return code from Audio-disk
                Status = STATUS_SUCCESS;
            }
        }

#ifdef UDF_CDRW_EMULATION_ON_ROM
        Vcb->NWA = *SessionEndLba+7+1;
#else
        Vcb->NWA = *SessionEndLba+7+1;
#endif //UDF_CDRW_EMULATION_ON_ROM

try_exit: NOTHING;
    } _SEH2_FINALLY {
        if (toc) MyFreePool__(toc);
        if (LastSes) MyFreePool__(LastSes);
        if (TempBuffer) MyFreePool__(TempBuffer);
    } _SEH2_END;

    return Status;
} // end UDFUseStandard()

/*
    Get block size (for read operation)
 */
static
NTSTATUS
UDFGetBlockSize(
    IN PDEVICE_OBJECT DeviceObject,      // the target device object
    IN PVCB           Vcb                // Volume control block from this DevObj
    )
{
    NTSTATUS        RC = STATUS_SUCCESS;
    DISK_GEOMETRY_EX DiskGeometryEx;
    GET_LENGTH_INFORMATION LengthInfo;

    if (DeviceObject->DeviceType == FILE_DEVICE_DISK) {

        UDFPrint(("UDFGetBlockSize: HDD\n"));
        RC = UDFPerformDevIoCtrl(IOCTL_DISK_GET_LENGTH_INFO, DeviceObject,
            NULL,
            0,
            &LengthInfo, 
            sizeof(GET_LENGTH_INFORMATION),
            TRUE,
            NULL);

        if (!NT_SUCCESS(RC))
            try_return(RC);
    } else {
        RC = UDFPerformDevIoCtrl(IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX,DeviceObject,
            &DiskGeometryEx,sizeof(DISK_GEOMETRY_EX),
            &DiskGeometryEx,sizeof(DISK_GEOMETRY_EX),
            TRUE,NULL );

        if (RC == STATUS_DEVICE_NOT_READY) {
            // probably, the device is really busy, may be by CD/DVD recording
            UserPrint(("  busy (0)\n"));
            try_return(RC);
        }
    }

    if (DeviceObject->DeviceType == FILE_DEVICE_DISK ||
        FALSE) {
        Vcb->SessionEndLba = (uint32)(LengthInfo.Length.QuadPart >> Vcb->SectorShift) - 1;
    } else {
        if (NT_SUCCESS(RC)) {
            Vcb->SessionEndLba = (uint32)(DiskGeometryEx.Geometry.Cylinders.QuadPart *
                                    DiskGeometryEx.Geometry.TracksPerCylinder *
                                    DiskGeometryEx.Geometry.SectorsPerTrack - 1);
            if (Vcb->SessionEndLba == 0x7fffffff) {
                ASSERT(FALSE);
            }
        } else {

            try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
        }
    }

    RC = STATUS_SUCCESS;

try_exit:   NOTHING;

    UDFPrint(("UDFGetBlockSize:\nBlock size is %x, Block size bits %x, SessionEndLba is %x\n",
              Vcb->SectorSize, Vcb->SectorShift, Vcb->SessionEndLba));

    return RC;

} // end UDFGetBlockSize()

uint32
UDFFixFPAddress(
    IN PVCB           Vcb,               // Volume control block from this DevObj
    IN uint32         Lba
    )
{
    uint32 i = Vcb->LastReadTrack;
    uint32 pk;
    uint32 rel;

    if(FlagOn(Vcb->VcbState, VCB_STATE_PACKET_RUNOUT_FIXUP)) {
        if (Lba < 0x20)
            return Lba;
        pk = Lba / Vcb->TrackMap[i].PacketSize;
        rel = Lba % Vcb->TrackMap[i].PacketSize;
        UDFPrint(("FixFPAddr: %x -> %x\n", Lba, pk*(Vcb->TrackMap[i].PacketSize+7) + rel));
        return pk*(Vcb->TrackMap[i].PacketSize+7) + rel /*- Vcb->TrackMap[i].PacketFPOffset*/;
    }
    return Lba;
} // end UDFFixFPAddress()

/*
    detect device driver & try to read disk layout (use all methods)
 */
NTSTATUS
UDFGetDiskInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,      // the target device object
    IN PVCB           Vcb                // Volume control block from this DevObj
    )
{
    NTSTATUS        RC = STATUS_UNRECOGNIZED_VOLUME;

    UDFPrint(("UDFGetDiskInfo\n"));

    _SEH2_TRY {
        RC = UDFGetBlockSize(DeviceObject, Vcb);

        if (!NT_SUCCESS(RC)) {
            try_return(RC);
        }

        ULONG SessionStart;
        ULONG SessionEnd;

        RC = UDFDetermineVolumeLayout(IrpContext, DeviceObject, Vcb, &SessionStart, &SessionEnd);

        if (!NT_SUCCESS(RC)) {
            try_return(RC);
        }

        Vcb->SessionStartLba = SessionStart;
        if (SessionEnd) {
            Vcb->SessionEndLba = SessionEnd;
        }

try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if (Vcb->TrackMap) {
            if (Vcb->TrackMap[Vcb->LastTrackNum].LastLba > Vcb->NWA) {
                if (Vcb->NWA) {
                    if (Vcb->TrackMap[Vcb->LastTrackNum].DataParam & TrkInfo_FP) {
                        Vcb->SessionEndLba = Vcb->NWA-1;
                    } else {
                        Vcb->SessionEndLba = Vcb->NWA-7-1;
                    }
                }
            } else {
                if ((Vcb->LastTrackNum > 1) &&
                   (Vcb->TrackMap[Vcb->LastTrackNum-1].FirstLba >= Vcb->TrackMap[Vcb->LastTrackNum-1].LastLba)) {
                    Vcb->SessionEndLba = Vcb->TrackMap[Vcb->LastTrackNum-1].LastLba;
                }
            }
        }

        if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) {
            if (!Vcb->BlankCD && Vcb->MediaType != MediaType_UnknownSize_CDRW) {
                UDFPrint(("UDFGetDiskInfo: R/O+!Blank+!RW -> !RAW\n"));
                Vcb->VcbState &= ~UDF_VCB_FLAGS_RAW_DISK;
            } else {
                UDFPrint(("UDFGetDiskInfo: Blank or RW\n"));
            }
        }

        UDFPrint(("UDF: ------------------------------------------\n"));
        UDFPrint(("UDF: Media characteristics\n"));
        UDFPrint(("UDF: Last session: %d\n",Vcb->LastSession));
        UDFPrint(("UDF: First track: %d, Last track: %d\n",Vcb->FirstTrackNum, Vcb->LastTrackNum));
        UDFPrint(("UDF: Session start LBA: %x\n",Vcb->SessionStartLba));
        UDFPrint(("UDF: Session end LBA: %x\n",Vcb->SessionEndLba));
        UDFPrint(("UDF: NWA: %x\n",Vcb->NWA));
        UDFPrint(("UDF: Flags: %s%s\n",
                 Vcb->VcbState & UDF_VCB_FLAGS_RAW_DISK ? "RAW " : "",
                 Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY ? "R/O " : "WR "
                 ));
        UDFPrint(("UDF: ------------------------------------------\n"));

    } _SEH2_END;

    UDFPrint(("UDFGetDiskInfo: %x\n", RC));
    return(RC);

} // end UDFGetDiskInfo()

NTSTATUS
UDFPrepareForReadOperation(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BCount
    )
{
    if ( (Vcb->FsDeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM) ) {
        Vcb->VcbState &= ~UDF_VCB_LAST_WRITE;
        return STATUS_SUCCESS;
    }
    // uint32 i = Vcb->LastReadTrack;

    if (Vcb->BSBM_Bitmap) {
        ULONG i;
        uint32 lbn = Lba - Vcb->Partitions[0].PartitionRoot;
        for(i=0; i<BCount; i++) {
            if (UDFGetBit((uint32*)(Vcb->BSBM_Bitmap), lbn+i)) {
                UDFPrint(("R: Known BB @ %#x\n", Lba));
                //return STATUS_FT_WRITE_RECOVERY; // this shall not be treated as error and
                                                   // we shall get IO request to BAD block
                return STATUS_DEVICE_DATA_ERROR;
            }
        }
    }

    return STATUS_SUCCESS;
} // end UDFPrepareForReadOperation()

/*
    This routine reads physical sectors.
 */
NTSTATUS
UDFReadSectors(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Translate,
    IN uint32 Lba,
    IN uint32 BCount,
    IN BOOLEAN Direct,
    OUT int8* Buffer
    )
{
    return UDFReadWriteSectors(IrpContext, Vcb,
               ((LONGLONG)Lba) << Vcb->SectorShift,
               BCount * Vcb->SectorSize,
               TRUE, Buffer, FALSE);
} // end UDFReadSectors()

/*
    This routine reads data inside a single physical sector (sub-sector read).
 */
static
NTSTATUS
UDFReadInSector(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 Offset,
    IN uint32 Length,
    OUT int8* Buffer
    )
{
    int8* tmp_buff;
    NTSTATUS status;

    tmp_buff = (int8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize);
    if (!tmp_buff) return STATUS_INSUFFICIENT_RESOURCES;

    status = UDFReadWriteSectors(IrpContext, Vcb,
                 ((LONGLONG)Lba) << Vcb->SectorShift,
                 Vcb->SectorSize,
                 TRUE, tmp_buff, FALSE);
    if (NT_SUCCESS(status)) {
        RtlCopyMemory(Buffer, tmp_buff + Offset, Length);
    }

    MyFreePool__(tmp_buff);
    return status;
} // end UDFReadInSector()

/*
    This routine reads data at unaligned offset & length.
 */
NTSTATUS
UDFReadData(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Translate,
    IN int64 Offset,
    IN uint32 Length,
    IN BOOLEAN Direct,
    OUT int8* Buffer
    )
{
    uint32 i, l, Lba, BS = Vcb->SectorSize;
    uint32 BSh = Vcb->SectorShift;
    NTSTATUS status;

    if (!Length) return STATUS_SUCCESS;
    if (Vcb->VcbState & UDF_VCB_FLAGS_DEAD)
        return STATUS_NO_SUCH_DEVICE;

    // Read tail of the 1st sector if Offset is not sector-aligned.
    Lba = (uint32)(Offset >> BSh);
    if ((i = (uint32)(Offset & (BS - 1)))) {
        l = min(BS - i, Length);
        status = UDFReadInSector(IrpContext, Vcb, Lba, i, l, Buffer);
        if (!NT_SUCCESS(status)) return status;
        Length -= l;
        if (!Length) return STATUS_SUCCESS;
        Lba++;
        Buffer += l;
    }

    // Read sector-aligned middle part.
    if (Length >= BS) {
        uint32 AlignedBytes = (Length >> BSh) << BSh;
        status = UDFReadWriteSectors(IrpContext, Vcb,
                     ((LONGLONG)Lba) << BSh,
                     AlignedBytes,
                     TRUE, Buffer, FALSE);
        if (!NT_SUCCESS(status)) return status;
        Buffer += AlignedBytes;
        Lba += AlignedBytes >> BSh;
        Length -= AlignedBytes;
    }

    // Read head of the last sector.
    if (!Length) return STATUS_SUCCESS;
    return UDFReadInSector(IrpContext, Vcb, Lba, 0, Length, Buffer);
} // end UDFReadData()

NTSTATUS
UDFBarrierFlushDevice(IN PDEVICE_OBJECT TargetDeviceObject)
{
    IO_STATUS_BLOCK iosb;
    KEVENT event;
    PIRP irp;
    NTSTATUS status;

    iosb.Status = STATUS_UNSUCCESSFUL;
    iosb.Information = 0;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
                                       TargetDeviceObject, NULL, 0, NULL,
                                       &event, &iosb);
    if (!irp)
        return STATUS_INSUFFICIENT_RESOURCES;
    status = IoCallDriver(TargetDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = iosb.Status;
    } else if (NT_SUCCESS(status)) {
        status = iosb.Status;
    }
    return status;
}

NTSTATUS
UDFOpenIntegrityForWrite(IN PIRP_CONTEXT IrpContext, IN PVCB Vcb,
                         IN BOOLEAN Direct)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY)
        return STATUS_MEDIA_WRITE_PROTECTED;
    if (Vcb->Modified && Vcb->IntegrityType != INTEGRITY_TYPE_CLOSE &&
        (ExIsResourceAcquiredExclusiveLite(&Vcb->BitMapResource1) ||
         !InterlockedCompareExchange(&Vcb->IntegrityOpenPending, 0, 0)))
        return (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) ?
               STATUS_MEDIA_WRITE_PROTECTED : STATUS_SUCCESS;
    if (Vcb->CDR_Mode) {
        UDFSetModified(Vcb);
        return STATUS_SUCCESS;
    }

    /* A bitmap paging write can be invoked by CcFlushCache while the caller
     * owns this resource. Avoid taking it on the already-OPEN fast path. */
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    /* The first caller may have failed its barrier while this thread waited.
     * Never turn that failed checkpoint into an apparently successful write. */
    if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) {
        status = STATUS_MEDIA_WRITE_PROTECTED;
    } else if (!Vcb->Modified || Vcb->IntegrityType == INTEGRITY_TYPE_CLOSE) {
        /* UDFUpdateLogicalVolInt sets IntegrityType=OPEN before the lower
         * barrier. Its recursive sector write is allowed on this thread;
         * unrelated writers block on BitMapResource1 until the barrier ends. */
        InterlockedExchange(&Vcb->IntegrityOpenPending, 1);
        UDFSetModified(Vcb);
        if (Vcb->LVid && !Direct) {
            status = UDFUpdateLogicalVolInt(IrpContext, Vcb, FALSE);
            if (NT_SUCCESS(status))
                status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
            if (!NT_SUCCESS(status))
                Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        }
        InterlockedExchange(&Vcb->IntegrityOpenPending, 0);
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
    return status;
}

/*
    This routine writes physical sectors.
    Marks volume as modified and updates LVID if needed.
 */
NTSTATUS
UDFWriteSectors(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Translate,
    IN uint32 Lba,
    IN uint32 BCount,
    IN BOOLEAN Direct,
    IN int8* Buffer,
    OUT PSIZE_T WrittenBytes
    )
{
    NTSTATUS status;

    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, Direct);
    if (!NT_SUCCESS(status)) {
        *WrittenBytes = 0;
        return status;
    }

    if (Vcb->CDR_Mode) {
        if (Vcb->SessionEndLba < Lba + BCount - 1)
            Vcb->SessionEndLba = Lba + BCount - 1;
    }

    ULONG ByteCount = BCount << Vcb->SectorShift;
    status = UDFReadWriteSectors(IrpContext, Vcb,
                 ((LONGLONG)Lba) << Vcb->SectorShift,
                 ByteCount,
                 TRUE, Buffer, TRUE);

    *WrittenBytes = NT_SUCCESS(status) ? ByteCount : 0;

    return status;
} // end UDFWriteSectors()

/*
    This routine writes data inside a single physical sector (read-modify-write).
 */
static
NTSTATUS
UDFWriteInSector(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 Offset,
    IN uint32 Length,
    IN int8* Buffer,
    OUT PSIZE_T WrittenBytes
    )
{
    int8* tmp_buff;
    NTSTATUS status;

    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status)) {
        *WrittenBytes = 0;
        return status;
    }

    if (Vcb->CDR_Mode) {
        if (Vcb->SessionEndLba < Lba)
            Vcb->SessionEndLba = Lba;
    }

    *WrittenBytes = 0;

    tmp_buff = (int8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize);
    if (!tmp_buff) {
        BrutePoint();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Read-modify-write cycle.
    status = UDFReadWriteSectors(IrpContext, Vcb,
                 ((LONGLONG)Lba) << Vcb->SectorShift,
                 Vcb->SectorSize,
                 TRUE, tmp_buff, FALSE);
    if (!NT_SUCCESS(status)) goto done;

    /* Equal primary bytes do not establish that an independent metadata
       mirror committed this generation. A failed or short completion can
       follow a completed primary write, before the paired mirror write.
       Keep the paired writer on retry even when this primary slice matches.
       In CDR_Mode, equal writes still advance the next writable address. */
    if (!Vcb->CDR_Mode &&
        (!(Vcb->MetadataFlags & UDF_METADATA_DUPLICATED) ||
         UDFPhysLbaToMetadata(Vcb, Lba) == LBA_OUT_OF_EXTENT) &&
        RtlCompareMemory(tmp_buff + Offset, Buffer, Length) == Length) {
        *WrittenBytes = Length;
        goto done;
    }

    RtlCopyMemory(tmp_buff + Offset, Buffer, Length);

    status = UDFReadWriteSectors(IrpContext, Vcb,
                 ((LONGLONG)Lba) << Vcb->SectorShift,
                 Vcb->SectorSize,
                 TRUE, tmp_buff, TRUE);
    if (NT_SUCCESS(status))
        *WrittenBytes = Length;

done:
    MyFreePool__(tmp_buff);

    ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status)) {
        UDFPrint(("UDFWriteInSector() for LBA %x failed\n", Lba));
    }

    return status;
} // end UDFWriteInSector()

/*
    This routine writes data at unaligned offset & length.
 */
NTSTATUS
UDFWriteData(
    PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Translate,
    IN int64 Offset,
    IN SIZE_T Length,
    IN BOOLEAN Direct,
    IN int8* Buffer,
    OUT PSIZE_T WrittenBytes
    )
{
    uint32 i, l, Lba, BS = Vcb->SectorSize;
    uint32 BSh = Vcb->SectorShift;
    NTSTATUS status;
    SIZE_T _WrittenBytes;

    *WrittenBytes = 0;
    if (!Length) return STATUS_SUCCESS;
    if (Vcb->VcbState & UDF_VCB_FLAGS_DEAD)
        return STATUS_NO_SUCH_DEVICE;

    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, Direct);
    if (!NT_SUCCESS(status))
        return status;

    if (Vcb->CDR_Mode) {
        Lba = (uint32)(Offset >> BSh);
        uint32 EndLba = (uint32)((Offset + Length - 1) >> BSh);
        if (Vcb->SessionEndLba < EndLba)
            Vcb->SessionEndLba = EndLba;
    }

    // Write tail of the 1st sector if Offset is not sector-aligned.
    Lba = (uint32)(Offset >> BSh);
    if ((i = ((uint32)Offset & (BS - 1)))) {
        l = min(BS - i, (uint32)Length);
        status = UDFWriteInSector(IrpContext, Vcb, Lba, i, l, Buffer, WrittenBytes);
        if (!NT_SUCCESS(status)) return status;
        Length -= l;
        if (!Length) return STATUS_SUCCESS;
        Lba++;
        Buffer += l;
    }

    // Write sector-aligned middle part.
    if (Length >= BS) {
        uint32 AlignedBytes = ((uint32)(Length >> BSh)) << BSh;
        status = UDFReadWriteSectors(IrpContext, Vcb,
                     ((LONGLONG)Lba) << BSh,
                     AlignedBytes,
                     TRUE, Buffer, TRUE);

        _WrittenBytes = NT_SUCCESS(status) ? AlignedBytes : 0;
        *WrittenBytes += _WrittenBytes;
        if (!NT_SUCCESS(status)) return status;

        Lba += AlignedBytes >> BSh;
        Buffer += AlignedBytes;
        Length -= AlignedBytes;
    }

    if (!Length) return STATUS_SUCCESS;

    status = UDFWriteInSector(IrpContext, Vcb, Lba, 0, (uint32)Length, Buffer, &_WrittenBytes);
    *WrittenBytes += _WrittenBytes;

    return status;
} // end UDFWriteData()
