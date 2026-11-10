////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*++

Module Name:

    VolInfo.cpp

Abstract:

    This module implements the volume information routines for UDF called by
    the dispatch driver.

--*/

#include            "udffs.h"

// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID                UDF_FILE_VOL_INFORMATION

//  Local support routines
NTSTATUS
UDFQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UDFQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UDFQueryFsFullSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UDFQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UDFQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
UDFSetLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN OUT PULONG Length);

/*
    This is the common routine for querying volume information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

 */
_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFCommonQueryVolInfo(
    _Inout_ PIRP_CONTEXT IrpContext,
    _Inout_ PIRP Irp
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    ULONG Length;
    PVCB Vcb;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb = NULL;

    PAGED_CODE();

    // Reference our input parameters to make things easier

    Length = IrpSp->Parameters.QueryVolume.Length;

    // Decode the file object and fail if this an unopened file object.

    TypeOfOpen = UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb);

    if (TypeOfOpen == UnopenedFileObject) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    // Acquire the Vcb for this volume.

    UDFAcquireVcbShared(IrpContext, Vcb, FALSE);

    // Use a try-finally to facilitate cleanup.

    _SEH2_TRY {

        //  Verify the Vcb.

        UDFVerifyVcb(IrpContext, Fcb->Vcb);

        switch (IrpSp->Parameters.QueryVolume.FsInformationClass) {

        case FileFsVolumeInformation:

            Status = UDFQueryFsVolumeInfo( IrpContext, Vcb, (PFILE_FS_VOLUME_INFORMATION)(Irp->AssociatedIrp.SystemBuffer), &Length );
            break;

        case FileFsSizeInformation:

            Status = UDFQueryFsSizeInfo( IrpContext, Vcb, (PFILE_FS_SIZE_INFORMATION)(Irp->AssociatedIrp.SystemBuffer), &Length );
            break;

        case FileFsDeviceInformation:

            Status = UDFQueryFsDeviceInfo( IrpContext, Vcb, (PFILE_FS_DEVICE_INFORMATION)(Irp->AssociatedIrp.SystemBuffer), &Length );
            break;

        case FileFsAttributeInformation:

            Status = UDFQueryFsAttributeInfo( IrpContext, Vcb, (PFILE_FS_ATTRIBUTE_INFORMATION)(Irp->AssociatedIrp.SystemBuffer), &Length );
            break;

        case FileFsFullSizeInformation:

            Status = UDFQueryFsFullSizeInfo( IrpContext, Vcb, (PFILE_FS_FULL_SIZE_INFORMATION)(Irp->AssociatedIrp.SystemBuffer), &Length );
            break;

        default:

            Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;

        }

        //  Set the information field to the number of bytes actually filled in
        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    } _SEH2_FINALLY {

        // Release the Vcb.

        UDFReleaseVcb(IrpContext, Vcb);

    } _SEH2_END;

    // Complete the request if we didn't raise.

    UDFCompleteRequest(IrpContext, Irp, Status);

    return Status;
} // end UDFCommonQueryVolInfo()


//  Local support routine

/*
    This routine implements the query volume info call

Arguments:

    Vcb    - Vcb for this volume.
    Buffer - Supplies a pointer to the output buffer where the information
             is to be returned
    Length - Supplies the length of the buffer in byte.  This variable
             upon return recieves the remaining bytes free in the buffer
 */
NTSTATUS
UDFQueryFsVolumeInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    )
{
    ULONG BytesToCopy;
    NTSTATUS Status;

    PAGED_CODE();

    // Fill in the data from the Vcb.

    Buffer->VolumeCreationTime.QuadPart = Vcb->VolCreationTime;
    Buffer->VolumeSerialNumber = Vcb->PhSerialNumber;

    Buffer->SupportsObjects = FALSE;

    *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

    //  Check if the buffer we're given is long enough

    if (*Length >= (ULONG) Vcb->VolIdent.Length) {

        BytesToCopy = Vcb->VolIdent.Length;
        Status = STATUS_SUCCESS;

    } else {

        BytesToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    // Copy over what we can of the volume label, and adjust *Length

    Buffer->VolumeLabelLength = BytesToCopy;

    if (BytesToCopy) {

        RtlCopyMemory(&Buffer->VolumeLabel[0], Vcb->VolIdent.Buffer, BytesToCopy);

    }

    *Length -= BytesToCopy;

    return Status;
} // end UDFQueryFsVolumeInfo()

/*
    This routine implements the query volume size call.

Arguments:

    Vcb    - Vcb for this volume.
    Buffer - Supplies a pointer to the output buffer where the information
             is to be returned
    Length - Supplies the length of the buffer in byte.  This variable
             upon return recieves the remaining bytes free in the buffer
 */
NTSTATUS
UDFQueryFsSizeInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )
{
    PAGED_CODE();

    UDFPrint(("  UDFQueryFsSizeInfo: \n"));
    //  Fill in the output buffer.
    if (Vcb->BitmapModified) {
        Vcb->TotalAllocUnits =
        Buffer->TotalAllocationUnits.QuadPart = UDFGetTotalSpace(Vcb);
        Vcb->FreeAllocUnits =
        Buffer->AvailableAllocationUnits.QuadPart = UDFGetFreeSpace(Vcb);
        Vcb->BitmapModified = FALSE;
    } else {
        Buffer->TotalAllocationUnits.QuadPart = Vcb->TotalAllocUnits;
        Buffer->AvailableAllocationUnits.QuadPart = Vcb->FreeAllocUnits;
    }
    Vcb->LowFreeSpace = (Vcb->FreeAllocUnits < 16384);
    if (!Buffer->TotalAllocationUnits.QuadPart)
        Buffer->TotalAllocationUnits.QuadPart = max(1, Vcb->FSBM_BitCount);
    Buffer->SectorsPerAllocationUnit = Vcb->SectorSize >> Vcb->SectorShift;
    Buffer->BytesPerSector = Vcb->SectorSize;

    UDFPrint(("  Space: Total %I64x, Free %I64x\n",
        Buffer->TotalAllocationUnits.QuadPart,
        Buffer->AvailableAllocationUnits.QuadPart));

    //  Adjust the length variable
    *Length -= sizeof( FILE_FS_SIZE_INFORMATION );
    return STATUS_SUCCESS;
} // UDFQueryFsSizeInfo()

/*
    This routine implements the query volume full size call.

Arguments:

    Vcb    - Vcb for this volume.
    Buffer - Supplies a pointer to the output buffer where the information
             is to be returned
    Length - Supplies the length of the buffer in byte.  This variable
             upon return recieves the remaining bytes free in the buffer
 */
NTSTATUS
UDFQueryFsFullSizeInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_FULL_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )
{
    PAGED_CODE();

    UDFPrint(("  UDFQueryFsFullSizeInfo: \n"));
    //  Fill in the output buffer.
    if (Vcb->BitmapModified) {
        Vcb->TotalAllocUnits =
        Buffer->TotalAllocationUnits.QuadPart = UDFGetTotalSpace(Vcb);
        Vcb->FreeAllocUnits =
        Buffer->CallerAvailableAllocationUnits.QuadPart =
        Buffer->ActualAvailableAllocationUnits.QuadPart = UDFGetFreeSpace(Vcb);
        Vcb->BitmapModified = FALSE;
    } else {
        Buffer->TotalAllocationUnits.QuadPart = Vcb->TotalAllocUnits;
        Buffer->CallerAvailableAllocationUnits.QuadPart =
        Buffer->ActualAvailableAllocationUnits.QuadPart = Vcb->FreeAllocUnits;
    }
    if (!Buffer->TotalAllocationUnits.QuadPart)
        Buffer->TotalAllocationUnits.QuadPart = max(1, Vcb->FSBM_BitCount);
    Buffer->SectorsPerAllocationUnit = Vcb->SectorSize >> Vcb->SectorShift;
    Buffer->BytesPerSector = Vcb->SectorSize;

    UDFPrint(("  Space: Total %I64x, Free %I64x\n",
        Buffer->TotalAllocationUnits.QuadPart,
        Buffer->ActualAvailableAllocationUnits.QuadPart));

    //  Adjust the length variable
    *Length -= sizeof( FILE_FS_FULL_SIZE_INFORMATION );
    return STATUS_SUCCESS;
} // UDFQueryFsSizeInfo()

/*
    This routine implements the query volume device call.

Arguments:

    Vcb    - Vcb for this volume.
    Buffer - Supplies a pointer to the output buffer where the information
             is to be returned
    Length - Supplies the length of the buffer in byte.  This variable
             upon return recieves the remaining bytes free in the buffer
 */
NTSTATUS
UDFQueryFsDeviceInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )
{
    PAGED_CODE();

    UDFPrint(("  UDFQueryFsDeviceInfo: \n"));
    //  Update the output buffer.
    if (Vcb->TargetDeviceObject->DeviceType != FILE_DEVICE_CD_ROM && Vcb->TargetDeviceObject->DeviceType != FILE_DEVICE_DVD)
    {
        ASSERT(! (Vcb->TargetDeviceObject->Characteristics & (FILE_READ_ONLY_DEVICE | FILE_WRITE_ONCE_MEDIA)));
        Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics & ~(FILE_READ_ONLY_DEVICE | FILE_WRITE_ONCE_MEDIA);
    }
    else
    {
        Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;
    }
    Buffer->DeviceType = Vcb->TargetDeviceObject->DeviceType;
    UDFPrint(("    Characteristics %x, DeviceType %x\n", Buffer->Characteristics, Buffer->DeviceType));
    //  Adjust the length variable
    *Length -= sizeof( FILE_FS_DEVICE_INFORMATION );
    return STATUS_SUCCESS;
} // end UDFQueryFsDeviceInfo()

/*
    This routine implements the query volume attribute call.

Arguments:

    Vcb    - Vcb for this volume.
    Buffer - Supplies a pointer to the output buffer where the information
             is to be returned
    Length - Supplies the length of the buffer in byte.  This variable
             upon return recieves the remaining bytes free in the buffer
 */
NTSTATUS
UDFQueryFsAttributeInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )
{
    ULONG BytesToCopy;

    NTSTATUS Status = STATUS_SUCCESS;
    PCWSTR FsTypeTitle;
    ULONG FsTypeTitleLen;

    PAGED_CODE();
    UDFPrint(("  UDFQueryFsAttributeInfo: \n"));
    //  Fill out the fixed portion of the buffer.
    Buffer->FileSystemAttributes = FILE_CASE_SENSITIVE_SEARCH |
                                   FILE_CASE_PRESERVED_NAMES |
                                   (UDFIsStreamsSupported(Vcb) ? FILE_NAMED_STREAMS : 0) |
                                   ((Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) ? FILE_READ_ONLY_VOLUME : 0) |
                                   FILE_UNICODE_ON_DISK;

    Buffer->MaximumComponentNameLength = UDF_X_NAME_LEN-1;

    *Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName );
    //  Make sure we can copy full unicode characters.
    *Length &= ~1;
    //  Determine how much of the file system name will fit.

#define UDF_FS_TITLE_UDF L"UDF"

#define UDFSetFsTitle(tit) \
                FsTypeTitle = UDF_FS_TITLE_##tit; \
                FsTypeTitleLen = sizeof(UDF_FS_TITLE_##tit) - sizeof(WCHAR);

    UDFSetFsTitle(UDF);

#undef UDFSetFsTitle
#undef UDF_FS_TITLE_UDF

    if (*Length >= FsTypeTitleLen) {
        BytesToCopy = FsTypeTitleLen;
    } else {
        BytesToCopy = *Length;
        Status = STATUS_BUFFER_OVERFLOW;
    }

    *Length -= BytesToCopy;
    //  Do the file system name.
    Buffer->FileSystemNameLength = BytesToCopy;
    RtlCopyMemory( &Buffer->FileSystemName[0], FsTypeTitle, BytesToCopy );
    //  And return to our caller
    return Status;
} // end UDFQueryFsAttributeInfo()

/*
    This is the common routine for setting volume information called by both
    the fsd and fsp threads.
 */
_Requires_lock_held_(_Global_critical_region_)
NTSTATUS
UDFCommonSetVolInfo(
    PIRP_CONTEXT IrpContext,
    PIRP Irp
    )
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    PCCB Ccb;
    PVCB Vcb;

    PAGED_CODE();

    // Reference our input parameters to make things easier

    Length = IrpSp->Parameters.SetVolume.Length;
    FsInformationClass = IrpSp->Parameters.SetVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    // Decode the file object to get the Vcb

    TypeOfOpen = UDFDecodeFileObject(IrpSp->FileObject, &Fcb, &Ccb);

    if (TypeOfOpen != UserVolumeOpen) {

        UDFCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = Fcb->Vcb;

    ASSERT_CCB(Ccb);
    ASSERT_FCB(Fcb);
    ASSERT_VCB(Vcb);

    if (Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY) {

        if (Vcb->VcbState & VCB_STATE_MEDIA_WRITE_PROTECT) {

            Status = STATUS_MEDIA_WRITE_PROTECTED;
        }
        else if (Vcb->VcbState & VCB_STATE_MOUNTED_DIRTY) {

            Status = STATUS_VOLUME_DIRTY;
        }
        else {

            Status = STATUS_ACCESS_DENIED;
        }

        UDFCompleteRequest(IrpContext, Irp, Status);
        return Status;
    }

    // Acquire the Vcb for this volume.

    UDFAcquireVcbExclusive(IrpContext, Vcb, FALSE);

    // Use a try-finally to facilitate cleanup.

    _SEH2_TRY {

        switch (FsInformationClass) {

        case FileFsLabelInformation:

            Status = UDFSetLabelInfo(IrpContext, Vcb, (PFILE_FS_LABEL_INFORMATION)Irp->AssociatedIrp.SystemBuffer, &Length);
            Irp->IoStatus.Information = 0;
            break;

        default:

            Status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Information = 0;
            break;

        }

        //  Set the information field to the number of bytes actually filled in
        Irp->IoStatus.Information = IrpSp->Parameters.SetVolume.Length - Length;

    } _SEH2_FINALLY {

        UDFReleaseVcb(IrpContext, Vcb);

    } _SEH2_END;

    // Complete the request if we didn't raise.

    UDFCompleteRequest(IrpContext, Irp, Status);

    return Status;
} // end UDFCommonSetVolInfo()

/*
    This sets Volume Label
 */
NTSTATUS
UDFSetLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN OUT PULONG Length
    )
{
    PAGED_CODE();

    UDFPrint(("  UDFSetLabelInfo: \n"));
    if (Buffer->VolumeLabelLength > UDF_VOL_LABEL_LEN*sizeof(WCHAR)) {
        // Too long Volume Label... NT doesn't like it
        UDFPrint(("  UDFSetLabelInfo: STATUS_INVALID_VOLUME_LABEL\n"));
        return STATUS_INVALID_VOLUME_LABEL;
    }

    if (Vcb->VolIdent.Buffer) MyFreePool__(Vcb->VolIdent.Buffer);
    Vcb->VolIdent.Buffer = (PWCHAR)MyAllocatePool__(NonPagedPool, Buffer->VolumeLabelLength+sizeof(WCHAR));
    if (!Vcb->VolIdent.Buffer) return STATUS_INSUFFICIENT_RESOURCES;

    Vcb->VolIdent.Length = (USHORT)Buffer->VolumeLabelLength;
    Vcb->VolIdent.MaximumLength = (USHORT)Buffer->VolumeLabelLength+sizeof(WCHAR);
    RtlCopyMemory(Vcb->VolIdent.Buffer, &(Buffer->VolumeLabel), Buffer->VolumeLabelLength);
    Vcb->VolIdent.Buffer[Buffer->VolumeLabelLength/sizeof(WCHAR)] = 0;
    Vcb->VolIdentChanged = TRUE;
    UDFSetModified(Vcb);

    UDFPrint(("  UDFSetLabelInfo: OK\n"));
    return STATUS_SUCCESS;
} // end UDFSetLabelInfo ()
