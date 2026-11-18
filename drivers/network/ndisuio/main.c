/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS User I/O driver
 * FILE:        drivers/network/ndisuio/main.c
 * PURPOSE:     Driver entry point and NDIS 6 protocol registration
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 *              Ryan Wilson <ryan@verosnt.com>
 */

/*
 * NDIS 6.84 registration uses the measured characteristics header type
 * 0x95, revision 2, size 0x80, version 6.84 from
 * output/ndis-discovery/windows.log (DIFF END OK) and
 * drivers/network/ndis/src/prot6.rs / drivers/network/lldp/src/lib.rs.
 * Handler offsets follow
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/ns-ndis-_ndis_protocol_driver_characteristics
 * BindAdapterEx is
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nc-ndis-protocol_bind_adapter_ex
 * NdisRegisterProtocolDriver is
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ndis/nf-ndis-ndisregisterprotocoldriver
 * Legacy NdisRegisterProtocol left GlobalAdapterList empty on the NDIS 6
 * stack (NDISUIO SMOKE bind miss 00000103). VerosBindReadyProtocols is
 * drivers/network/ndis/ndis.spec.
 */

#include "ndisuio.h"

#define NDEBUG
#include <debug.h>

PDEVICE_OBJECT GlobalDeviceObject;
NDIS_HANDLE GlobalProtocolHandle;
KSPIN_LOCK GlobalAdapterListLock;
LIST_ENTRY GlobalAdapterList;

NDIS_STRING ProtocolName = RTL_CONSTANT_STRING(L"NDISUIO");

/* Measured NDIS 6.84 protocol characteristics (prot6.rs / METHOD.md). */
#define PR_TYPE 0x95
#define PR_REVISION 2
#define PR_CHARS 0x80
#define PR_VERSION_AT 4
#define PR_NAME_AT 0x10
#define PR_SET_OPTIONS_AT 0x20
#define PR_BIND_AT 0x28
#define PR_UNBIND_AT 0x30
#define PR_OPEN_COMPLETE_AT 0x38
#define PR_CLOSE_COMPLETE_AT 0x40
#define PR_OID_COMPLETE_AT 0x58
#define PR_STATUS_AT 0x60
#define PR_RECEIVE_AT 0x68
#define PR_SEND_COMPLETE_AT 0x70

static UCHAR ProtocolChars[PR_CHARS];

NDIS_STATUS
NTAPI
NdisRegisterProtocolDriver(
    NDIS_HANDLE ProtocolDriverContext,
    PVOID ProtocolCharacteristics,
    PNDIS_HANDLE NdisProtocolHandle);

VOID
NTAPI
VerosBindReadyProtocols(VOID);

static
VOID
PutU16(PUCHAR Buffer, ULONG Offset, USHORT Value)
{
    Buffer[Offset] = (UCHAR)(Value & 0xFF);
    Buffer[Offset + 1] = (UCHAR)((Value >> 8) & 0xFF);
}

static
VOID
PutPtr(PUCHAR Buffer, ULONG Offset, PVOID Value)
{
    ULONG_PTR Address = (ULONG_PTR)Value;
    ULONG i;

    for (i = 0; i < sizeof(PVOID); i++)
    {
        Buffer[Offset + i] = (UCHAR)((Address >> (8 * i)) & 0xFF);
    }
}

static
NDIS_STATUS
NTAPI
NduSetOptions(NDIS_HANDLE DriverHandle,
              NDIS_HANDLE DriverContext)
{
    UNREFERENCED_PARAMETER(DriverHandle);
    UNREFERENCED_PARAMETER(DriverContext);
    return NDIS_STATUS_SUCCESS;
}

static
VOID
NTAPI
NduOidRequestComplete(NDIS_HANDLE ProtocolBindingContext,
                      PVOID OidRequest,
                      NDIS_STATUS Status)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(OidRequest);
    UNREFERENCED_PARAMETER(Status);
}

static
VOID
NTAPI
NduStatusEx(NDIS_HANDLE ProtocolBindingContext,
            PVOID StatusIndication)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(StatusIndication);
}

static
VOID
NTAPI
NduReceiveNetBufferLists(NDIS_HANDLE ProtocolBindingContext,
                         PVOID NetBufferLists,
                         ULONG PortNumber,
                         ULONG NumberOfNetBufferLists,
                         ULONG ReceiveFlags)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(NetBufferLists);
    UNREFERENCED_PARAMETER(PortNumber);
    UNREFERENCED_PARAMETER(NumberOfNetBufferLists);
    UNREFERENCED_PARAMETER(ReceiveFlags);
}

static
VOID
NTAPI
NduSendNetBufferListsComplete(NDIS_HANDLE ProtocolBindingContext,
                              PVOID NetBufferLists,
                              ULONG SendCompleteFlags)
{
    UNREFERENCED_PARAMETER(ProtocolBindingContext);
    UNREFERENCED_PARAMETER(NetBufferLists);
    UNREFERENCED_PARAMETER(SendCompleteFlags);
}

VOID NTAPI NduUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DPRINT("NDISUIO: Unloaded\n");
}

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    NDIS_STATUS Status;
    UNICODE_STRING NtDeviceName = RTL_CONSTANT_STRING(NDISUIO_DEVICE_NAME_NT);
    UNICODE_STRING DosDeviceName = RTL_CONSTANT_STRING(NDISUIO_DEVICE_NAME_DOS);

    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->MajorFunction[IRP_MJ_CREATE] = NduDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = NduDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NduDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_READ] = NduDispatchRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = NduDispatchWrite;
    DriverObject->DriverUnload = NduUnload;

    InitializeListHead(&GlobalAdapterList);
    KeInitializeSpinLock(&GlobalAdapterListLock);

    Status = IoCreateDevice(DriverObject,
                            0,
                            &NtDeviceName,
                            FILE_DEVICE_SECURE_OPEN,
                            0,
                            FALSE,
                            &GlobalDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create device object with status 0x%x\n", Status);
        return Status;
    }

    Status = IoCreateSymbolicLink(&DosDeviceName, &NtDeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create symbolic link with status 0x%x\n", Status);
        IoDeleteDevice(GlobalDeviceObject);
        return Status;
    }

    RtlZeroMemory(ProtocolChars, sizeof(ProtocolChars));
    ProtocolChars[0] = PR_TYPE;
    ProtocolChars[1] = PR_REVISION;
    PutU16(ProtocolChars, 2, PR_CHARS);
    ProtocolChars[PR_VERSION_AT] = 6;
    ProtocolChars[PR_VERSION_AT + 1] = 84;
    PutU16(ProtocolChars, PR_NAME_AT, ProtocolName.Length);
    PutU16(ProtocolChars, PR_NAME_AT + 2, ProtocolName.MaximumLength);
    PutPtr(ProtocolChars, PR_NAME_AT + 8, ProtocolName.Buffer);
    PutPtr(ProtocolChars, PR_SET_OPTIONS_AT, NduSetOptions);
    PutPtr(ProtocolChars, PR_BIND_AT, NduBindAdapterEx);
    PutPtr(ProtocolChars, PR_UNBIND_AT, NduUnbindAdapterEx);
    PutPtr(ProtocolChars, PR_OPEN_COMPLETE_AT, NduOpenAdapterComplete);
    PutPtr(ProtocolChars, PR_CLOSE_COMPLETE_AT, NduCloseAdapterComplete);
    PutPtr(ProtocolChars, PR_OID_COMPLETE_AT, NduOidRequestComplete);
    PutPtr(ProtocolChars, PR_STATUS_AT, NduStatusEx);
    PutPtr(ProtocolChars, PR_RECEIVE_AT, NduReceiveNetBufferLists);
    PutPtr(ProtocolChars, PR_SEND_COMPLETE_AT, NduSendNetBufferListsComplete);

    Status = NdisRegisterProtocolDriver(NULL,
                                        ProtocolChars,
                                        &GlobalProtocolHandle);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        DPRINT1("Failed to register protocol with status 0x%x\n", Status);
        IoDeleteSymbolicLink(&DosDeviceName);
        IoDeleteDevice(GlobalDeviceObject);
        return Status;
    }

    VerosBindReadyProtocols();
    DPRINT("NDISUIO: Loaded NDIS6\n");
    DbgPrint("NDISUIO REG6 %08x\n", Status);

    return STATUS_SUCCESS;
}
