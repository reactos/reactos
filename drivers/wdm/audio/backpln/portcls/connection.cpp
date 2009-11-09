/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/connection.c
 * PURPOSE:         portcls physical connection registration
 * PROGRAMMER:      Johannes Anderwald
 */


#include "private.hpp"

extern
"C"
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);


class CUnregisterPhysicalConnection : public IUnregisterPhysicalConnection
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IUnregisterPhysicalConnection;

    CUnregisterPhysicalConnection(IUnknown *OuterUnknown){}

    virtual ~CUnregisterPhysicalConnection(){}

protected:
    LONG m_Ref;

};

NTSTATUS
NTAPI
CUnregisterPhysicalConnection::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnregisterPhysicalConnection) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));

        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
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
    PLIST_ENTRY Entry;
    PPHYSICAL_CONNECTION Connection;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;
    ISubdevice * FromSubDevice = NULL;
    ISubdevice * ToSubDevice = NULL;
    ULONG bFound;

    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (FromUnknown)
    {
        // get our private interface
        Status = FromUnknown->QueryInterface(IID_ISubdevice, (PVOID*)&FromSubDevice);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
    }

    if (ToUnknown)
    {
        Status = ToUnknown->QueryInterface(IID_ISubdevice, (PVOID*)&ToSubDevice);
        if (!NT_SUCCESS(Status))
            goto cleanup;
    }


    Entry = DeviceExt->PhysicalConnectionList.Flink;
    bFound = FALSE;
    // loop physical connection list
    while(Entry != &DeviceExt->PhysicalConnectionList)
    {
        Connection = (PPHYSICAL_CONNECTION)CONTAINING_RECORD(Entry, PHYSICAL_CONNECTION, Entry);
         // compare current entry
        if (Connection->FromPin == FromPin && Connection->ToPin == ToPin &&
            Connection->FromSubDevice == FromSubDevice && Connection->ToSubDevice == ToSubDevice)
        {
            if (FromString && Connection->FromUnicodeString.Buffer)
            {
                if (!RtlCompareUnicodeString(FromString, &Connection->FromUnicodeString, TRUE))
                {
                    // UnregisterPhysicalConnectionFromExternal
                    bFound = TRUE;
                    break;
                }
            }
            else if (ToString && Connection->ToUnicodeString.Buffer)
            {
                if (!RtlCompareUnicodeString(ToString, &Connection->ToUnicodeString, TRUE))
                {
                    // UnregisterPhysicalConnectionToExternal
                    bFound = TRUE;
                    break;
                }
            }
            else
            {
                // UnregisterPhysicalConnection
                bFound = TRUE;
                break;
            }
        }
        Entry = Entry->Flink;
    }

    if (!bFound)
    {
         // not found
         Status = STATUS_NOT_FOUND;
         goto cleanup;
    }

    // remove list entry
    RemoveEntryList(&Connection->Entry);

    // release resources
    if (Connection->FromSubDevice)
        Connection->FromSubDevice->Release();


    if (Connection->ToSubDevice)
        Connection->ToSubDevice->Release();

    if (Connection->FromUnicodeString.Buffer)
        RtlFreeUnicodeString(&Connection->FromUnicodeString);

    if (Connection->ToUnicodeString.Buffer)
        RtlFreeUnicodeString(&Connection->ToUnicodeString);

    FreeItem(Connection, TAG_PORTCLASS);
    Status = STATUS_SUCCESS;

cleanup:

    if (FromSubDevice)
        FromSubDevice->Release();

    if (ToSubDevice)
        ToSubDevice->Release();

    return Status;

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
    PHYSICAL_CONNECTION *NewConnection;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    NTSTATUS Status;

    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    NewConnection = (PPHYSICAL_CONNECTION)AllocateItem(NonPagedPool, sizeof(PHYSICAL_CONNECTION), TAG_PORTCLASS);
    if (!NewConnection)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    if (FromUnknown)
    {
        Status = FromUnknown->QueryInterface(IID_ISubdevice, (PVOID*)&NewConnection->FromSubDevice);
        if (!NT_SUCCESS(Status))
            goto cleanup;
    }
    else
    {
        if (!RtlCreateUnicodeString(&NewConnection->FromUnicodeString, (PCWSTR)FromString))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
    }

    if (ToUnknown)
    {
        Status = ToUnknown->QueryInterface(IID_ISubdevice, (PVOID*)&NewConnection->ToSubDevice);
        if (!NT_SUCCESS(Status))
            goto cleanup;
    }
    else
    {
        if (!RtlCreateUnicodeString(&NewConnection->ToUnicodeString, (PCWSTR)ToString))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }
    }

    InsertTailList(&DeviceExt->PhysicalConnectionList, &NewConnection->Entry);
    return STATUS_SUCCESS;

cleanup:

    if (NewConnection->FromSubDevice)
        NewConnection->FromSubDevice->Release();

    if (NewConnection->ToSubDevice)
        NewConnection->ToSubDevice->Release();

    if (NewConnection->FromUnicodeString.Buffer)
        RtlFreeUnicodeString(&NewConnection->FromUnicodeString);

    if (NewConnection->ToUnicodeString.Buffer)
        RtlFreeUnicodeString(&NewConnection->ToUnicodeString);

     FreeItem(NewConnection, TAG_PORTCLASS);

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
