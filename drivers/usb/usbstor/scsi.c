/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Storage Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbstor/pdo.c
 * PURPOSE:     USB block storage device driver.
 * PROGRAMMERS:
 *              James Tabor
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbstor.h"

NTSTATUS
USBSTOR_BuildCBW(
    IN ULONG Tag,
    IN ULONG DataTransferLength,
    IN UCHAR LUN,
    IN UCHAR CommandBlockLength,
    IN PUCHAR CommandBlock,
    IN OUT PCBW Control)
{
    //
    // sanity check
    //
    ASSERT(CommandBlockLength <= 16);

    //
    // now initialize CBW
    //
    Control->Signature = CBW_SIGNATURE;
    Control->Tag = Tag;
    Control->DataTransferLength = DataTransferLength;
    Control->Flags = 0x80;
    Control->LUN = (LUN & MAX_LUN);
    Control->CommandBlockLength = CommandBlockLength;

    //
    // copy command block
    //
    RtlCopyMemory(Control->CommandBlock, CommandBlock, CommandBlockLength);

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
USBSTOR_SendCBW(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR CommandBlockLength,
    IN PUCHAR CommandBlock,
    IN ULONG DataTransferLength,
    OUT PCBW *OutControl)
{
    PCBW Control;
    NTSTATUS Status;
    PURB Urb;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // first allocate CBW
    //
    Control = (PCBW)AllocateItem(NonPagedPool, 512);
    if (!Control)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // first allocate CBW
    //
    Status = USBSTOR_BuildCBW(0xDEADDEAD, DataTransferLength, PDODeviceExtension->LUN, CommandBlockLength, CommandBlock, Control);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build CBW
        //
        return Status;
    }

    //
    // now build the urb
    //
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(URB));
    if (!Urb)
    {
        //
        // failed to allocate urb
        //
        FreeItem(Control);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // now initialize the urb
    //
    Urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(URB);
    Urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    Urb->UrbBulkOrInterruptTransfer.PipeHandle = FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkOutPipeIndex].PipeHandle;
    Urb->UrbBulkOrInterruptTransfer.TransferBuffer = (PVOID)Control;
    Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = sizeof(CBW);
    Urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT | USBD_SHORT_TRANSFER_OK;

    //
    // now send urb
    //
    Status = USBSTOR_SyncUrbRequest(FDODeviceExtension->LowerDeviceObject, Urb);

    //
    // free urb
    //
    FreeItem(Urb);

    //
    // store cbw
    //
    *OutControl = Control;

    //
    // return operation status
    //
    return Status;
}

NTSTATUS
USBSTOR_SendData(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DataTransferLength,
    IN PVOID DataTransfer)
{
    PMDL TransferBufferMDL;
    PURB Urb;
    NTSTATUS Status;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // allocate mdl for buffer, buffer must be allocated from NonPagedPool
    //
    TransferBufferMDL = IoAllocateMdl(DataTransfer, DataTransferLength, FALSE, FALSE, NULL);
    if (!TransferBufferMDL)
    {
        //
        // failed to allocate MDL
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // build mdl for nonpaged pool
    //
    MmBuildMdlForNonPagedPool(TransferBufferMDL);

    //
    // now build the urb
    //
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(URB));
    if (!Urb)
    {
        //
        // failed to allocate urb
        //
        IoFreeMdl(TransferBufferMDL);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // now initialize the urb
    //
    Urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(URB);
    Urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    Urb->UrbBulkOrInterruptTransfer.PipeHandle = FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkInPipeIndex].PipeHandle;
    Urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = TransferBufferMDL;
    Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DataTransferLength;
    Urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;

    //
    // now send urb
    //
    Status = USBSTOR_SyncUrbRequest(FDODeviceExtension->LowerDeviceObject, Urb);

    //
    // free urb
    //
    FreeItem(Urb);

    //
    // free mdl
    //
    IoFreeMdl(TransferBufferMDL);

    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_SendCSW(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Data,
    IN ULONG DataLength,
    OUT PCSW OutCSW)
{
    NTSTATUS Status;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PURB Urb;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // get FDO device extension
    //
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)PDODeviceExtension->LowerDeviceObject->DeviceExtension;

    //
    // now build the urb
    //
    Urb = (PURB)AllocateItem(NonPagedPool, sizeof(URB));
    if (!Urb)
    {
        //
        // failed to allocate urb
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // now initialize the urb
    //
    Urb->UrbBulkOrInterruptTransfer.Hdr.Length = sizeof(URB);
    Urb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    Urb->UrbBulkOrInterruptTransfer.PipeHandle = FDODeviceExtension->InterfaceInformation->Pipes[FDODeviceExtension->BulkInPipeIndex].PipeHandle;
    Urb->UrbBulkOrInterruptTransfer.TransferBuffer = Data;
    Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DataLength;
    Urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;

    //
    // now send urb
    //
    Status = USBSTOR_SyncUrbRequest(FDODeviceExtension->LowerDeviceObject, Urb);

    if (NT_SUCCESS(Status))
    {
        //
        // copy csw status
        //
        RtlCopyMemory(OutCSW, Data, sizeof(CSW));
    }

    //
    // free urb
    //
    FreeItem(Urb);

    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_SendInquiryCmd(
    IN PDEVICE_OBJECT DeviceObject)
{
    UFI_INQUIRY_CMD Cmd;
    CSW CSW;
    NTSTATUS Status;
    PUFI_INQUIRY_RESPONSE Response;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PCBW OutControl;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // allocate inquiry response
    //
    Response = (PUFI_INQUIRY_RESPONSE)AllocateItem(NonPagedPool, sizeof(UFI_INQUIRY_RESPONSE));
    if (!Response)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize inquiry cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_INQUIRY_CMD));
    Cmd.Code = SCSIOP_INQUIRY;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.AllocationLength = sizeof(UFI_INQUIRY_RESPONSE);

    //
    // now send inquiry cmd
    //
    Status = USBSTOR_SendCBW(DeviceObject, UFI_INQUIRY_CMD_LEN, (PUCHAR)&Cmd, sizeof(UFI_INQUIRY_RESPONSE), &OutControl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendInquiryCmd> USBSTOR_SendCBW failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    //
    // now send inquiry response
    //
    Status = USBSTOR_SendData(DeviceObject, sizeof(UFI_INQUIRY_RESPONSE), Response);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendInquiryCmd> USBSTOR_SendData failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    DPRINT1("Response %p\n", Response);
    DPRINT1("DeviceType %x\n", Response->DeviceType);
    DPRINT1("RMB %x\n", Response->RMB);
    DPRINT1("Version %x\n", Response->Version);
    DPRINT1("Format %x\n", Response->Format);
    DPRINT1("Length %x\n", Response->Length);
    DPRINT1("Reserved %x\n", Response->Reserved);
    DPRINT1("Vendor %c%c%c%c%c%c%c%c\n", Response->Vendor[0], Response->Vendor[1], Response->Vendor[2], Response->Vendor[3], Response->Vendor[4], Response->Vendor[5], Response->Vendor[6], Response->Vendor[7]);
    DPRINT1("Product %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n", Response->Product[0], Response->Product[1], Response->Product[2], Response->Product[3],
                                                          Response->Product[4], Response->Product[5], Response->Product[6], Response->Product[7], 
                                                          Response->Product[8], Response->Product[9], Response->Product[10], Response->Product[11],
                                                          Response->Product[12], Response->Product[13], Response->Product[14], Response->Product[15]);

    DPRINT1("Revision %c%c%c%c\n", Response->Revision[0], Response->Revision[1], Response->Revision[2], Response->Revision[3]);

    //
    // send csw
    //
    Status = USBSTOR_SendCSW(DeviceObject, OutControl, 512, &CSW);

    DPRINT1("------------------------\n");
    DPRINT1("CSW %p\n", &CSW);
    DPRINT1("Signature %x\n", CSW.Signature);
    DPRINT1("Tag %x\n", CSW.Tag);
    DPRINT1("DataResidue %x\n", CSW.DataResidue);
    DPRINT1("Status %x\n", CSW.Status);

    //
    // free item
    //
    FreeItem(OutControl);

    //
    // store inquiry data
    //
    PDODeviceExtension->InquiryData = (PVOID)Response;

    //
    // FIXME: handle error
    //
    ASSERT(CSW.Status == 0);


    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_SendCapacityCmd(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PREAD_CAPACITY_DATA_EX CapacityDataEx,
    OUT PREAD_CAPACITY_DATA CapacityData)
{
    UFI_CAPACITY_CMD Cmd;
    CSW CSW;
    NTSTATUS Status;
    PUFI_CAPACITY_RESPONSE Response;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PCBW OutControl;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // allocate capacity response
    //
    Response = (PUFI_CAPACITY_RESPONSE)AllocateItem(NonPagedPool, sizeof(UFI_CAPACITY_RESPONSE));
    if (!Response)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // initialize capacity cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_INQUIRY_CMD));
    Cmd.Code = SCSIOP_READ_CAPACITY;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);

    //
    // now send capacity cmd
    //
    Status = USBSTOR_SendCBW(DeviceObject, UFI_CAPACITY_CMD_LEN, (PUCHAR)&Cmd, sizeof(UFI_CAPACITY_RESPONSE), &OutControl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendCapacityCmd> USBSTOR_SendCBW failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    //
    // now send inquiry response
    //
    Status = USBSTOR_SendData(DeviceObject, sizeof(UFI_CAPACITY_RESPONSE), Response);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendCapacityCmd> USBSTOR_SendData failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    DPRINT1("LastLogicalBlockAddress %lu\n", NTOHL(Response->LastLogicalBlockAddress));
    DPRINT1("BlockLength %lu\n", NTOHL(Response->BlockLength));
    DPRINT1("Medium Length %lu\n", NTOHL(Response->BlockLength) * NTOHL(Response->LastLogicalBlockAddress));

    //
    // store response
    //
    if (CapacityDataEx)
    {
        CapacityDataEx->LogicalBlockAddress.QuadPart = Response->LastLogicalBlockAddress;
        CapacityDataEx->BytesPerBlock = Response->BlockLength;
    }
    else
    {
        CapacityData->LogicalBlockAddress = Response->LastLogicalBlockAddress;
        CapacityData->BytesPerBlock = Response->BlockLength;
    }

    //
    // store result in device extension
    //
    PDODeviceExtension->LastLogicBlockAddress = NTOHL(Response->LastLogicalBlockAddress);
    PDODeviceExtension->BlockLength =  NTOHL(Response->BlockLength);

    //
    // send csw
    //
    Status = USBSTOR_SendCSW(DeviceObject, OutControl, 512, &CSW);

    DPRINT1("------------------------\n");
    DPRINT1("CSW %p\n", &CSW);
    DPRINT1("Signature %x\n", CSW.Signature);
    DPRINT1("Tag %x\n", CSW.Tag);
    DPRINT1("DataResidue %x\n", CSW.DataResidue);
    DPRINT1("Status %x\n", CSW.Status);

    //
    // FIXME: handle error
    //
    ASSERT(CSW.Status == 0);

    //
    // free item
    //
    FreeItem(OutControl);

    //
    // free response
    //
    FreeItem(Response);

    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_SendModeSenseCmd(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Request,
    OUT PULONG TransferBufferLength)
{
    UFI_SENSE_CMD Cmd;
    CSW CSW;
    NTSTATUS Status;
    PVOID Response;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PCBW OutControl;
    PCDB pCDB;
    PUFI_MODE_PARAMETER_HEADER Header;

    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // allocate sense response from non paged pool
    //
    Response = (PUFI_CAPACITY_RESPONSE)AllocateItem(NonPagedPool, Request->DataTransferLength);
    if (!Response)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // sanity check
    //


    // Supported pages
    // MODE_PAGE_ERROR_RECOVERY
    // MODE_PAGE_FLEXIBILE
    // MODE_PAGE_LUN_MAPPING
    // MODE_PAGE_FAULT_REPORTING
    // MODE_SENSE_RETURN_ALL

    //
    // initialize mode sense cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_INQUIRY_CMD));
    Cmd.Code = SCSIOP_MODE_SENSE;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.PageCode = pCDB->MODE_SENSE.PageCode;
    Cmd.PC = pCDB->MODE_SENSE.Pc;
    Cmd.AllocationLength = HTONS(pCDB->MODE_SENSE.AllocationLength);

    DPRINT1("PageCode %x\n", pCDB->MODE_SENSE.PageCode);
    DPRINT1("PC %x\n", pCDB->MODE_SENSE.Pc);

    //
    // now send mode sense cmd
    //
    Status = USBSTOR_SendCBW(DeviceObject, UFI_SENSE_CMD_LEN, (PUCHAR)&Cmd, Request->DataTransferLength, &OutControl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendCapacityCmd> USBSTOR_SendCBW failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    //
    // now send data block response
    //
    Status = USBSTOR_SendData(DeviceObject, Request->DataTransferLength, Response);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendCapacityCmd> USBSTOR_SendData failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    Header = (PUFI_MODE_PARAMETER_HEADER)Response;

    //
    // TODO: build layout
    //
    // first struct is the header
    // MODE_PARAMETER_HEADER / _MODE_PARAMETER_HEADER10
    //
    // followed by 
    // MODE_PARAMETER_BLOCK
    //
    // 
	DbgBreakPoint();

    //
    // send csw
    //
    Status = USBSTOR_SendCSW(DeviceObject, OutControl, 512, &CSW);

    DPRINT1("------------------------\n");
    DPRINT1("CSW %p\n", &CSW);
    DPRINT1("Signature %x\n", CSW.Signature);
    DPRINT1("Tag %x\n", CSW.Tag);
    DPRINT1("DataResidue %x\n", CSW.DataResidue);
    DPRINT1("Status %x\n", CSW.Status);

    //
    // FIXME: handle error
    //
    ASSERT(CSW.Status == 0);
    ASSERT(CSW.DataResidue == 0);

    //
    // calculate transfer length
    //
    *TransferBufferLength = Request->DataTransferLength - CSW.DataResidue;

    //
    // copy buffer
    //
    RtlCopyMemory(Request->DataBuffer, Response, *TransferBufferLength);

    //
    // free item
    //
    FreeItem(OutControl);

    //
    // free response
    //
    FreeItem(Response);

    //
    // done
    //
    return Status;
}

NTSTATUS
USBSTOR_SendReadCmd(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PSCSI_REQUEST_BLOCK Request,
    OUT PULONG TransferBufferLength)
{
    UFI_READ_CMD Cmd;
    CSW CSW;
    NTSTATUS Status;
    PVOID Response;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PCBW OutControl;
    PCDB pCDB;
    ULONG BlockCount;

    //
    // get SCSI command data block
    //
    pCDB = (PCDB)Request->Cdb;

    //
    // get PDO device extension
    //
    PDODeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // allocate read buffer response from non paged pool
    //
    Response = (PUFI_CAPACITY_RESPONSE)AllocateItem(NonPagedPool, Request->DataTransferLength);
    if (!Response)
    {
        //
        // no memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // FIXME: support more logical blocks
    //
    ASSERT(Request->DataTransferLength == PDODeviceExtension->BlockLength);

    //
    // block count
    //
    BlockCount = Request->DataTransferLength / PDODeviceExtension->BlockLength;

    //
    // initialize read sense cmd
    //
    RtlZeroMemory(&Cmd, sizeof(UFI_READ_CMD));
    Cmd.Code = SCSIOP_READ;
    Cmd.LUN = (PDODeviceExtension->LUN & MAX_LUN);
    Cmd.ContiguousLogicBlocks = _byteswap_ushort(BlockCount);

    RtlCopyMemory(&Cmd.LogicalBlockAddress, pCDB->READ12.LogicalBlock, sizeof(UCHAR) * 4);

    DPRINT1("BlockAddress %lu BlockCount %lu BlockLength %lu\n", NTOHL(Cmd.LogicalBlockAddress), BlockCount, PDODeviceExtension->BlockLength);

    //
    // now send read cmd
    //
    Status = USBSTOR_SendCBW(DeviceObject, UFI_READ_CMD_LEN, (PUCHAR)&Cmd, Request->DataTransferLength, &OutControl);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to send CBW
        //
        DPRINT1("USBSTOR_SendReadCmd> USBSTOR_SendCBW failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }

    //
    // now read the logical block
    //
    Status = USBSTOR_SendData(DeviceObject, Request->DataTransferLength, Response);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to read logical block
        //
        DPRINT1("USBSTOR_SendReadCmd> USBSTOR_SendData failed with %x\n", Status);
        FreeItem(Response);
        ASSERT(FALSE);
        return Status;
    }


    DbgBreakPoint();

    //
    // send csw
    //
    Status = USBSTOR_SendCSW(DeviceObject, OutControl, 512, &CSW);

    DPRINT1("------------------------\n");
    DPRINT1("CSW %p\n", &CSW);
    DPRINT1("Signature %x\n", CSW.Signature);
    DPRINT1("Tag %x\n", CSW.Tag);
    DPRINT1("DataResidue %x\n", CSW.DataResidue);
    DPRINT1("Status %x\n", CSW.Status);

    //
    // FIXME: handle error
    //
    ASSERT(CSW.Status == 0);
    ASSERT(CSW.DataResidue == 0);

    //
    // calculate transfer length
    //
    *TransferBufferLength = Request->DataTransferLength - CSW.DataResidue;

    //
    // copy buffer
    //
    RtlCopyMemory(Request->DataBuffer, Response, *TransferBufferLength);

    //
    // free item
    //
    FreeItem(OutControl);

    //
    // free response
    //
    FreeItem(Response);

    //
    // done
    //
    return Status;
}
