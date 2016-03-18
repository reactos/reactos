/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             volinfo.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://www.ext2fsd.com
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ext2fs.h"

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2QueryVolumeInformation)
#pragma alloc_text(PAGE, Ext2SetVolumeInformation)
#endif


NTSTATUS
Ext2QueryVolumeInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB               Vcb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FS_INFORMATION_CLASS    FsInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 VcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));

        if (!IsMounted(Vcb)) {
            Status = STATUS_VOLUME_DISMOUNTED;
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceSharedLite(
                    &Vcb->MainResource,
                    IsFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)
                )) {

            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        VcbResourceAcquired = TRUE;

        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        FsInformationClass =
            IoStackLocation->Parameters.QueryVolume.FsInformationClass;

        Length = IoStackLocation->Parameters.QueryVolume.Length;
        Buffer = Irp->AssociatedIrp.SystemBuffer;

        RtlZeroMemory(Buffer, Length);

        switch (FsInformationClass) {

        case FileFsVolumeInformation:
        {
            PFILE_FS_VOLUME_INFORMATION FsVolInfo;
            ULONG                       VolumeLabelLength;
            ULONG                       RequiredLength;

            if (Length < sizeof(FILE_FS_VOLUME_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FsVolInfo = (PFILE_FS_VOLUME_INFORMATION) Buffer;
            FsVolInfo->VolumeCreationTime.QuadPart = 0;
            FsVolInfo->VolumeSerialNumber = Vcb->Vpb->SerialNumber;
            VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;
            FsVolInfo->VolumeLabelLength = VolumeLabelLength;
            /* We don't support ObjectId */
            FsVolInfo->SupportsObjects = FALSE;

            RequiredLength = sizeof(FILE_FS_VOLUME_INFORMATION)
                             + VolumeLabelLength - sizeof(WCHAR);

            if (Length < RequiredLength) {
                Irp->IoStatus.Information =
                    sizeof(FILE_FS_VOLUME_INFORMATION);
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            RtlCopyMemory(FsVolInfo->VolumeLabel, Vcb->Vpb->VolumeLabel, Vcb->Vpb->VolumeLabelLength);

            Irp->IoStatus.Information = RequiredLength;
            Status = STATUS_SUCCESS;
        }
        break;

        case FileFsSizeInformation:
        {
            PFILE_FS_SIZE_INFORMATION FsSizeInfo;

            if (Length < sizeof(FILE_FS_SIZE_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FsSizeInfo = (PFILE_FS_SIZE_INFORMATION) Buffer;
            FsSizeInfo->TotalAllocationUnits.QuadPart =
                ext3_blocks_count(SUPER_BLOCK);
            FsSizeInfo->AvailableAllocationUnits.QuadPart =
                ext3_free_blocks_count(SUPER_BLOCK);
            FsSizeInfo->SectorsPerAllocationUnit =
                Vcb->BlockSize / Vcb->DiskGeometry.BytesPerSector;
            FsSizeInfo->BytesPerSector =
                Vcb->DiskGeometry.BytesPerSector;

            Irp->IoStatus.Information = sizeof(FILE_FS_SIZE_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

        case FileFsDeviceInformation:
        {
            PFILE_FS_DEVICE_INFORMATION FsDevInfo;

            if (Length < sizeof(FILE_FS_DEVICE_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FsDevInfo = (PFILE_FS_DEVICE_INFORMATION) Buffer;
            FsDevInfo->DeviceType =
                Vcb->TargetDeviceObject->DeviceType;

            if (FsDevInfo->DeviceType != FILE_DEVICE_DISK) {
                DbgBreak();
            }

            FsDevInfo->Characteristics =
                Vcb->TargetDeviceObject->Characteristics;

            if (IsVcbReadOnly(Vcb)) {
                SetFlag( FsDevInfo->Characteristics,
                         FILE_READ_ONLY_DEVICE   );
            }

            Irp->IoStatus.Information = sizeof(FILE_FS_DEVICE_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

        case FileFsAttributeInformation:
        {
            PFILE_FS_ATTRIBUTE_INFORMATION  FsAttrInfo;
            ULONG                           RequiredLength;

            if (Length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            FsAttrInfo =
                (PFILE_FS_ATTRIBUTE_INFORMATION) Buffer;
            FsAttrInfo->FileSystemAttributes =
                FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;
            if (IsVcbReadOnly(Vcb)) {
                FsAttrInfo->FileSystemAttributes |= FILE_READ_ONLY_VOLUME;
            }
            FsAttrInfo->MaximumComponentNameLength = EXT2_NAME_LEN;
            FsAttrInfo->FileSystemNameLength = 8;

            RequiredLength = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                             8 - sizeof(WCHAR);

            if (Length < RequiredLength) {
                Irp->IoStatus.Information =
                    sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            if (IsFlagOn(SUPER_BLOCK->s_feature_incompat, EXT4_FEATURE_INCOMPAT_EXTENTS)) {
                RtlCopyMemory(FsAttrInfo->FileSystemName,  L"EXT4\0", 10);
            } else if (Vcb->IsExt3fs) {
                RtlCopyMemory(FsAttrInfo->FileSystemName,  L"EXT3\0", 10);
            } else {
                RtlCopyMemory(FsAttrInfo->FileSystemName,  L"EXT2\0", 10);
            }

            Irp->IoStatus.Information = RequiredLength;
            Status = STATUS_SUCCESS;
        }
        break;

#if (_WIN32_WINNT >= 0x0500)

        case FileFsFullSizeInformation:
        {
            PFILE_FS_FULL_SIZE_INFORMATION PFFFSI;

            if (Length < sizeof(FILE_FS_FULL_SIZE_INFORMATION)) {
                Status = STATUS_BUFFER_OVERFLOW;
                _SEH2_LEAVE;
            }

            PFFFSI = (PFILE_FS_FULL_SIZE_INFORMATION) Buffer;

            /*
                            typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
                                LARGE_INTEGER   TotalAllocationUnits;
                                LARGE_INTEGER   CallerAvailableAllocationUnits;
                                LARGE_INTEGER   ActualAvailableAllocationUnits;
                                ULONG           SectorsPerAllocationUnit;
                                ULONG           BytesPerSector;
                            } FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;
            */

            {
                PFFFSI->TotalAllocationUnits.QuadPart =
                    ext3_blocks_count(SUPER_BLOCK);

                PFFFSI->CallerAvailableAllocationUnits.QuadPart =
                    ext3_free_blocks_count(SUPER_BLOCK);

                /* - Vcb->SuperBlock->s_r_blocks_count; */
                PFFFSI->ActualAvailableAllocationUnits.QuadPart =
                    ext3_free_blocks_count(SUPER_BLOCK);
            }

            PFFFSI->SectorsPerAllocationUnit =
                Vcb->BlockSize / Vcb->DiskGeometry.BytesPerSector;

            PFFFSI->BytesPerSector = Vcb->DiskGeometry.BytesPerSector;

            Irp->IoStatus.Information = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;

#endif // (_WIN32_WINNT >= 0x0500)

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
        }

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}

NTSTATUS
Ext2SetVolumeInformation (IN PEXT2_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PEXT2_VCB               Vcb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FS_INFORMATION_CLASS    FsInformationClass;
    BOOLEAN                 VcbResourceAcquired = FALSE;

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);

        ASSERT((IrpContext->Identifier.Type == EXT2ICX) &&
               (IrpContext->Identifier.Size == sizeof(EXT2_IRP_CONTEXT)));

        DeviceObject = IrpContext->DeviceObject;

        //
        // This request is not allowed on the main device object
        //
        if (IsExt2FsDevice(DeviceObject)) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }

        Vcb = (PEXT2_VCB) DeviceObject->DeviceExtension;
        ASSERT(Vcb != NULL);
        ASSERT((Vcb->Identifier.Type == EXT2VCB) &&
               (Vcb->Identifier.Size == sizeof(EXT2_VCB)));
        ASSERT(IsMounted(Vcb));

        if (IsVcbReadOnly(Vcb)) {
            Status = STATUS_MEDIA_WRITE_PROTECTED;
            _SEH2_LEAVE;
        }

        if (!ExAcquireResourceExclusiveLite(
                    &Vcb->MainResource, TRUE)) {
            Status = STATUS_PENDING;
            _SEH2_LEAVE;
        }
        VcbResourceAcquired = TRUE;

        Ext2VerifyVcb(IrpContext, Vcb);

        Irp = IrpContext->Irp;
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

        //Notes: SetVolume is not defined in ntddk.h of win2k ddk,
        //       But it's same to QueryVolume ....
        FsInformationClass =
            IoStackLocation->Parameters./*SetVolume*/QueryVolume.FsInformationClass;

        switch (FsInformationClass) {

        case FileFsLabelInformation:
        {
            PFILE_FS_LABEL_INFORMATION      VolLabelInfo = NULL;
            ULONG                           VolLabelLen;
            UNICODE_STRING                  LabelName  ;

            OEM_STRING                      OemName;

            VolLabelInfo = (PFILE_FS_LABEL_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
            VolLabelLen = VolLabelInfo->VolumeLabelLength;

            if (VolLabelLen > (16 * sizeof(WCHAR))) {
                Status = STATUS_INVALID_VOLUME_LABEL;
                _SEH2_LEAVE;
            }

            RtlCopyMemory( Vcb->Vpb->VolumeLabel,
                           VolLabelInfo->VolumeLabel,
                           VolLabelLen );

            RtlZeroMemory(Vcb->SuperBlock->s_volume_name, 16);
            LabelName.Buffer = VolLabelInfo->VolumeLabel;
            LabelName.MaximumLength = (USHORT)16 * sizeof(WCHAR);
            LabelName.Length = (USHORT)VolLabelLen;

            OemName.Buffer = SUPER_BLOCK->s_volume_name;
            OemName.Length  = 0;
            OemName.MaximumLength = 16;

            Ext2UnicodeToOEM(Vcb, &OemName, &LabelName);
            Vcb->Vpb->VolumeLabelLength = (USHORT) VolLabelLen;

            if (Ext2SaveSuper(IrpContext, Vcb)) {
                Status = STATUS_SUCCESS;
            }

            Irp->IoStatus.Information = 0;
        }
        break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
        }

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceLite(&Vcb->MainResource);
        }

        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                Ext2QueueRequest(IrpContext);
            } else {
                Ext2CompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;

    return Status;
}