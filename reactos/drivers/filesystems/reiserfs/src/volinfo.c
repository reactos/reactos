/*
 * COPYRIGHT:        GNU GENERAL PUBLIC LICENSE VERSION 2
 * PROJECT:          ReiserFs file system driver for Windows NT/2000/XP/Vista.
 * FILE:             volinfo.c
 * PURPOSE:          
 * PROGRAMMER:       Mark Piper, Matt Wu, Bo Brantén.
 * HOMEPAGE:         
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include "rfsd.h"

/* GLOBALS ***************************************************************/

extern PRFSD_GLOBAL RfsdGlobal;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RfsdQueryVolumeInformation)
#if !RFSD_READ_ONLY
#pragma alloc_text(PAGE, RfsdSetVolumeInformation)
#endif // !RFSD_READ_ONLY
#endif

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdQueryVolumeInformation (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PRFSD_VCB               Vcb = 0;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FS_INFORMATION_CLASS    FsInformationClass;
    ULONG                   Length;
    PVOID                   Buffer;
    BOOLEAN                 VcbResourceAcquired = FALSE;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        //
        // This request is not allowed on the main device object
        //
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

        if (!ExAcquireResourceSharedLite(
            &Vcb->MainResource,
            IrpContext->IsSynchronous
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
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FsVolInfo = (PFILE_FS_VOLUME_INFORMATION) Buffer;
                
                FsVolInfo->VolumeCreationTime.QuadPart = 0;
                
                FsVolInfo->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

                VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;
                
                FsVolInfo->VolumeLabelLength = VolumeLabelLength;
                
                // I don't know what this means
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
                _SEH2_LEAVE;
            }
            
        case FileFsSizeInformation:
            {
                PFILE_FS_SIZE_INFORMATION FsSizeInfo;
                
                if (Length < sizeof(FILE_FS_SIZE_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FsSizeInfo = (PFILE_FS_SIZE_INFORMATION) Buffer;
                
                FsSizeInfo->TotalAllocationUnits.QuadPart = 
					Vcb->SuperBlock->s_blocks_count;
                    
                FsSizeInfo->AvailableAllocationUnits.QuadPart =
					Vcb->SuperBlock->s_free_blocks_count;

                FsSizeInfo->SectorsPerAllocationUnit =
                    Vcb->BlockSize / Vcb->DiskGeometry.BytesPerSector;
                
                FsSizeInfo->BytesPerSector =
                    Vcb->DiskGeometry.BytesPerSector;
                
                Irp->IoStatus.Information = sizeof(FILE_FS_SIZE_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            
        case FileFsDeviceInformation:
            {
                PFILE_FS_DEVICE_INFORMATION FsDevInfo;
                
                if (Length < sizeof(FILE_FS_DEVICE_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
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
                
                if (FlagOn(Vcb->Flags, VCB_READ_ONLY)) {

                    SetFlag( FsDevInfo->Characteristics,
                             FILE_READ_ONLY_DEVICE   );
                }
                
                Irp->IoStatus.Information = sizeof(FILE_FS_DEVICE_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }
            
        case FileFsAttributeInformation:
            {
                PFILE_FS_ATTRIBUTE_INFORMATION  FsAttrInfo;
                ULONG                           RequiredLength;
                
                if (Length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    _SEH2_LEAVE;
                }
                
                FsAttrInfo =
                    (PFILE_FS_ATTRIBUTE_INFORMATION) Buffer;
                
                FsAttrInfo->FileSystemAttributes =
                    FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;
                
                FsAttrInfo->MaximumComponentNameLength = RFSD_NAME_LEN;
                
                FsAttrInfo->FileSystemNameLength = 10;
                
                RequiredLength = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                    10 - sizeof(WCHAR);
                
                if (Length < RequiredLength) {
                    Irp->IoStatus.Information =
                        sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
                    Status = STATUS_BUFFER_OVERFLOW;
                    _SEH2_LEAVE;
                }
                
                RtlCopyMemory(
                    FsAttrInfo->FileSystemName,
                    L"RFSD\0", 10);
                
                Irp->IoStatus.Information = RequiredLength;
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }

#if (_WIN32_WINNT >= 0x0500)

        case FileFsFullSizeInformation:
            {
                PFILE_FS_FULL_SIZE_INFORMATION PFFFSI;

                if (Length < sizeof(FILE_FS_FULL_SIZE_INFORMATION)) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
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
						Vcb->SuperBlock->s_blocks_count;

                    PFFFSI->CallerAvailableAllocationUnits.QuadPart =
						Vcb->SuperBlock->s_free_blocks_count;

                    /* - Vcb->SuperBlock->s_r_blocks_count; */

                    PFFFSI->ActualAvailableAllocationUnits.QuadPart =
						Vcb->SuperBlock->s_free_blocks_count;
                }

                PFFFSI->SectorsPerAllocationUnit =
                    Vcb->BlockSize / Vcb->DiskGeometry.BytesPerSector;

                PFFFSI->BytesPerSector = Vcb->DiskGeometry.BytesPerSector;

                Irp->IoStatus.Information = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
                Status = STATUS_SUCCESS;
                _SEH2_LEAVE;
            }

#endif // (_WIN32_WINNT >= 0x0500)

        default:
            Status = STATUS_INVALID_INFO_CLASS;
        }

    } _SEH2_FINALLY {

        if (VcbResourceAcquired) {
            ExReleaseResourceForThreadLite(
                &Vcb->MainResource,
                ExGetCurrentResourceThread() );
        }
        
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                RfsdQueueRequest(IrpContext);
            } else {
                RfsdCompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;
    
    return Status;
}

#if !RFSD_READ_ONLY

__drv_mustHoldCriticalRegion
NTSTATUS
RfsdSetVolumeInformation (IN PRFSD_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT          DeviceObject;
    NTSTATUS                Status = STATUS_UNSUCCESSFUL;
    PRFSD_VCB               Vcb;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IoStackLocation;
    FS_INFORMATION_CLASS    FsInformationClass;

    PAGED_CODE();

    _SEH2_TRY {

        ASSERT(IrpContext != NULL);
        
        ASSERT((IrpContext->Identifier.Type == RFSDICX) &&
            (IrpContext->Identifier.Size == sizeof(RFSD_IRP_CONTEXT)));
        
        DeviceObject = IrpContext->DeviceObject;
    
        //
        // This request is not allowed on the main device object
        //
        if (DeviceObject == RfsdGlobal->DeviceObject) {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            _SEH2_LEAVE;
        }
        
        Vcb = (PRFSD_VCB) DeviceObject->DeviceExtension;
        
        ASSERT(Vcb != NULL);
        
        ASSERT((Vcb->Identifier.Type == RFSDVCB) &&
            (Vcb->Identifier.Size == sizeof(RFSD_VCB)));

        ASSERT(IsMounted(Vcb));

        if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
            Status = STATUS_MEDIA_WRITE_PROTECTED;
            _SEH2_LEAVE;
        }
        
        Irp = IrpContext->Irp;
        
        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
        
        //Notes: SetVolume is not defined in ntddk.h of win2k ddk,
        //       But it's same to QueryVolume ....
        FsInformationClass =
            IoStackLocation->Parameters./*SetVolume*/QueryVolume.FsInformationClass;
        
        switch (FsInformationClass) {
#if 0
        case FileFsLabelInformation:
            {
                PFILE_FS_LABEL_INFORMATION      VolLabelInfo = NULL;
                ULONG                           VolLabelLen;
                UNICODE_STRING                  LabelName  ;

                OEM_STRING                      OemName;

                VolLabelInfo = (PFILE_FS_LABEL_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
        
                VolLabelLen = VolLabelInfo->VolumeLabelLength;

                if(VolLabelLen > (16 * sizeof(WCHAR))) {
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

                RfsdUnicodeToOEM( &OemName,
                                  &LabelName);

                Vcb->Vpb->VolumeLabelLength = 
                       (USHORT) VolLabelLen;

                if (RfsdSaveSuper(IrpContext, Vcb)) {
                    Status = STATUS_SUCCESS;
                }

                Irp->IoStatus.Information = 0;

            }
            break;
#endif // 0
          default:
            Status = STATUS_INVALID_INFO_CLASS;
        }

    } _SEH2_FINALLY {
       
        if (!IrpContext->ExceptionInProgress) {
            if (Status == STATUS_PENDING) {
                RfsdQueueRequest(IrpContext);
            } else {
                RfsdCompleteIrpContext(IrpContext, Status);
            }
        }
    } _SEH2_END;
    
    return Status;
}

#endif // !RFSD_READ_ONLY
