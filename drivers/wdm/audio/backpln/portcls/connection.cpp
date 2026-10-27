/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/connection.c
 * PURPOSE:         portcls physical connection registration
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

extern
"C"
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);

class CUnregisterPhysicalConnection : public CUnknownImpl<IUnregisterPhysicalConnection>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IUnregisterPhysicalConnection;

    CUnregisterPhysicalConnection(IUnknown *OuterUnknown){}

    virtual ~CUnregisterPhysicalConnection(){}
};

NTSTATUS
NTAPI
CUnregisterPhysicalConnection::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IUnregisterPhysicalConnection) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));

        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("CUnregisterPhysicalConnection::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
UnRegisterConnection(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN PUNKNOWN FromUnknown,
    IN PUNICODE_STRING FromString,
    IN ULONG FromPin,
    IN PUNKNOWN ToUnknown,
    IN PUNICODE_STRING ToString,
    IN ULONG ToPin)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CUnregisterPhysicalConnection::UnregisterPhysicalConnection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNKNOWN FromUnknown,
    IN ULONG FromPin,
    IN PUNKNOWN ToUnknown,
    IN ULONG ToPin)
{
    if (!DeviceObject || !FromUnknown || !ToUnknown)
        return STATUS_INVALID_PARAMETER;

    return UnRegisterConnection(DeviceObject, FromUnknown, NULL, FromPin, ToUnknown, NULL, ToPin);
}

NTSTATUS
NTAPI
CUnregisterPhysicalConnection::UnregisterPhysicalConnectionToExternal(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNKNOWN FromUnknown,
    IN ULONG FromPin,
    IN PUNICODE_STRING ToString,
    IN ULONG ToPin)
{
    if (!DeviceObject || !FromUnknown || !ToString)
        return STATUS_INVALID_PARAMETER;

    return UnRegisterConnection(DeviceObject, FromUnknown, NULL, FromPin, NULL, ToString, ToPin);
}

NTSTATUS
NTAPI
CUnregisterPhysicalConnection::UnregisterPhysicalConnectionFromExternal(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING FromString,
    IN ULONG FromPin,
    IN PUNKNOWN ToUnknown,
    IN ULONG ToPin)
{
    if (!DeviceObject || !FromString || !ToUnknown)
        return STATUS_INVALID_PARAMETER;

    return UnRegisterConnection(DeviceObject, NULL, FromString, FromPin, ToUnknown, NULL, ToPin);
}

NTSTATUS
NTAPI
NewIUnregisterPhysicalConnection(
    OUT PUNREGISTERPHYSICALCONNECTION *OutConnection)
{

    CUnregisterPhysicalConnection *new_ptr = new(NonPagedPool, TAG_PORTCLASS) CUnregisterPhysicalConnection(NULL);

    if (!new_ptr)
        return STATUS_INSUFFICIENT_RESOURCES;

    new_ptr->AddRef();
    *OutConnection = (PUNREGISTERPHYSICALCONNECTION)new_ptr;
    return STATUS_SUCCESS;
}

NTSTATUS
RegisterConnection(
    IN OUT PDEVICE_OBJECT DeviceObject,
    IN PUNKNOWN FromUnknown,
    IN PUNICODE_STRING FromString,
    IN ULONG FromPin,
    IN PUNKNOWN ToUnknown,
    IN PUNICODE_STRING ToString,
    IN ULONG ToPin)
{
    PSUBDEVICE_DESCRIPTOR FromSubDeviceDescriptor = NULL, ToSubDeviceDescriptor = NULL;
    PSYMBOLICLINK_ENTRY SymEntry;
    ISubdevice * FromSubDevice = NULL, *ToSubDevice = NULL;
    NTSTATUS Status;
    PPHYSICAL_CONNECTION_ENTRY FromEntry = NULL, ToEntry = NULL;

    if (FromUnknown)
    {
        Status = FromUnknown->QueryInterface(IID_ISubdevice, (PVOID*)&FromSubDevice);
        if (!NT_SUCCESS(Status))
            goto cleanup;

        Status = FromSubDevice->GetDescriptor(&FromSubDeviceDescriptor);
        if (!NT_SUCCESS(Status))
            goto cleanup;

        if (IsListEmpty(&FromSubDeviceDescriptor->SymbolicLinkList))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }

        SymEntry = (PSYMBOLICLINK_ENTRY)CONTAINING_RECORD(FromSubDeviceDescriptor->SymbolicLinkList.Flink, SYMBOLICLINK_ENTRY, Entry);
        FromString = &SymEntry->SymbolicLink;
    }

    if (ToUnknown)
    {
        Status = ToUnknown->QueryInterface(IID_ISubdevice, (PVOID*)&ToSubDevice);
        if (!NT_SUCCESS(Status))
            goto cleanup;

        Status = ToSubDevice->GetDescriptor(&ToSubDeviceDescriptor);
        if (!NT_SUCCESS(Status))
            goto cleanup;

        if (IsListEmpty(&ToSubDeviceDescriptor->SymbolicLinkList))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto cleanup;
        }

        SymEntry = (PSYMBOLICLINK_ENTRY)CONTAINING_RECORD(ToSubDeviceDescriptor->SymbolicLinkList.Flink, SYMBOLICLINK_ENTRY, Entry);
        ToString = &SymEntry->SymbolicLink;

    }

    if (FromSubDeviceDescriptor)
    {
        FromEntry = (PPHYSICAL_CONNECTION_ENTRY)AllocateItem(NonPagedPool, sizeof(PHYSICAL_CONNECTION_ENTRY) + ToString->MaximumLength + sizeof(WCHAR), TAG_PORTCLASS);
        if (!FromEntry)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
    }

    if (ToSubDeviceDescriptor)
    {
        ToEntry = (PPHYSICAL_CONNECTION_ENTRY)AllocateItem(NonPagedPool, sizeof(PHYSICAL_CONNECTION_ENTRY) + FromString->MaximumLength + sizeof(WCHAR), TAG_PORTCLASS);
        if (!ToEntry)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
    }

    if (FromSubDeviceDescriptor)
    {
        ASSERT(ToString->MaximumLength);
        FromEntry->FromPin = FromPin;
        FromEntry->Connection.Pin = ToPin;
        FromEntry->Connection.Size = sizeof(KSPIN_PHYSICALCONNECTION) + ToString->MaximumLength + sizeof(WCHAR);
        RtlMoveMemory(&FromEntry->Connection.SymbolicLinkName, ToString->Buffer, ToString->MaximumLength);
        FromEntry->Connection.SymbolicLinkName[ToString->Length / sizeof(WCHAR)] = UNICODE_NULL;
        InsertTailList(&FromSubDeviceDescriptor->PhysicalConnectionList, &FromEntry->Entry);
    }

    if (ToSubDeviceDescriptor)
    {
        ToEntry->FromPin = ToPin;
        ToEntry->Connection.Pin = FromPin;
        ToEntry->Connection.Size = sizeof(KSPIN_PHYSICALCONNECTION) + FromString->MaximumLength + sizeof(WCHAR);
        RtlMoveMemory(&ToEntry->Connection.SymbolicLinkName, FromString->Buffer, FromString->MaximumLength);
        ToEntry->Connection.SymbolicLinkName[FromString->Length /  sizeof(WCHAR)] = UNICODE_NULL;

        InsertTailList(&ToSubDeviceDescriptor->PhysicalConnectionList, &ToEntry->Entry);

    }

    if (FromSubDevice)
        FromSubDevice->Release();

    if (ToSubDevice)
        ToSubDevice->Release();

    return STATUS_SUCCESS;

cleanup:

    if (FromSubDevice)
        FromSubDevice->Release();

    if (ToSubDevice)
        ToSubDevice->Release();

    if (FromEntry)
        FreeItem(FromEntry, TAG_PORTCLASS);

    if (ToEntry)
        FreeItem(ToEntry, TAG_PORTCLASS);

    return Status;
}

NTSTATUS
NTAPI
PcRegisterPhysicalConnection(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin)
{
    DPRINT("PcRegisterPhysicalConnection\n");
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!DeviceObject || !FromUnknown || !ToUnknown)
        return STATUS_INVALID_PARAMETER;

    return RegisterConnection(DeviceObject, FromUnknown, NULL, FromPin, ToUnknown, NULL, ToPin);
}

NTSTATUS
NTAPI
PcRegisterPhysicalConnectionFromExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNICODE_STRING FromString,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!DeviceObject || !FromString || !ToUnknown)
        return STATUS_INVALID_PARAMETER;

    return RegisterConnection(DeviceObject, NULL, FromString, FromPin, ToUnknown, NULL, ToPin);
}

NTSTATUS
NTAPI
PcRegisterPhysicalConnectionToExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNICODE_STRING ToString,
    IN  ULONG ToPin)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!DeviceObject || !FromUnknown || !ToString)
        return STATUS_INVALID_PARAMETER;

    return RegisterConnection(DeviceObject, FromUnknown, NULL, FromPin, NULL, ToString, ToPin);
}
