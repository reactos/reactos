/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        main.c
 * PURPOSE:     Driver entry point and protocol initialization
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

PDEVICE_OBJECT GlobalDeviceObject;
NDIS_HANDLE GlobalProtocolHandle;

VOID NTAPI NduUnload(PDRIVER_OBJECT DriverObject)
{
    IoDeleteDevice(GlobalDeviceObject);
    
    DPRINT("NDISUIO: Unloaded\n");
}

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    NDIS_PROTOCOL_CHARACTERISTICS Chars;

    /* Setup dispatch functions */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = NduDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = NduDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NduDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_READ] = NduDispatchRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = NduDispatchWrite;
    DriverObject->DriverUnload = NduUnload;

    /* Create the NDISUIO device object */
    Status = IoCreateDevice(DriverObject,
                            0,
                            NULL, // FIXME
                            NDISUIO_DEVICE_NAME,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &GlobalDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create device object with status 0x%x\n", Status);
        return Status;
    }
    
    /* Register the protocol with NDIS */
    RtlZeroMemory(&Chars, sizeof(Chars));
    Chars.MajorNdisVersion = NDIS_MAJOR_VERSION;
    Chars.MinorNdisVersion = NDIS_MINOR_VERSION;
    Chars.OpenAdapterCompleteHandler = NduOpenAdapterComplete;
    Chars.CloseAdapterCompleteHandler = NduCloseAdapterComplete;
    Chars.SendCompleteHandler = NduSendComplete;
    Chars.TransferDataCompleteHandler = NduTransferDataComplete;
    Chars.ResetCompleteHandler = NduResetComplete;
    Chars.RequestCompleteHandler = NduRequestComplete;
    Chars.ReceiveHandler = NduReceive;
    Chars.ReceiveComplete = NduReceiveComplete;
    Chars.StatusHandler = NduStatus;
    Chars.StatusCompleteHandler = NduStatusComplete;
    Chars.Name = NULL; //FIXME
    Chars.ReceivePacketHandler = NULL; //NduReceivePacket
    Chars.BindAdapterHandler = NduBindAdapter;
    Chars.UnbindAdapterHandler = NduUnbindAdapter;
    Chars.PnPEventHandler = NduPnPEvent;
    
    NdisRegisterProtocol(&Status,
                         &GlobalProtocolHandle,
                         &Chars,
                         sizeof(Chars));
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("Failed to register protocol with status 0x%x\n", Status);
        IoDeleteDevice(GlobalDeviceObject);
        return Status;
    }

    DPRINT("NDISUIO: Loaded\n");

    return STATUS_SUCCESS;
}
