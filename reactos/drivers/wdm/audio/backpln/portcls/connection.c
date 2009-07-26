/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/connection.c
 * PURPOSE:         portcls physical connection registration
 * PROGRAMMER:      Johannes Anderwald
 */


#include "private.h"

typedef struct
{
    IUnregisterPhysicalConnectionVtbl *lpVtbl;
    LONG ref;

}IUnregisterPhysicalConnectionImpl;

NTSTATUS
NTAPI
IUnregisterPhysicalConnection_fnQueryInterface(
    IUnregisterPhysicalConnection* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IUnregisterPhysicalConnectionImpl * This = (IUnregisterPhysicalConnectionImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IUnregisterPhysicalConnection) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IUnregisterPhysicalConnection_fnAddRef(
    IUnregisterPhysicalConnection* iface)
{
    IUnregisterPhysicalConnectionImpl * This = (IUnregisterPhysicalConnectionImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IUnregisterPhysicalConnection_fnRelease(
    IUnregisterPhysicalConnection* iface)
{
    IUnregisterPhysicalConnectionImpl * This = (IUnregisterPhysicalConnectionImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    return This->ref;
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
        /* get our private interface */
        Status = FromUnknown->lpVtbl->QueryInterface(FromUnknown, &IID_ISubdevice, (PVOID*)&FromSubDevice);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
    }

    if (ToUnknown)
    {
        Status = ToUnknown->lpVtbl->QueryInterface(ToUnknown, &IID_ISubdevice, (PVOID*)&ToSubDevice);
        if (!NT_SUCCESS(Status))
            goto cleanup;
    }


    Entry = DeviceExt->PhysicalConnectionList.Flink;
    bFound = FALSE;
    /* loop physical connection list */
    while(Entry != &DeviceExt->PhysicalConnectionList)
    {
        Connection = (PPHYSICAL_CONNECTION)CONTAINING_RECORD(Entry, PHYSICAL_CONNECTION, Entry);
         /* compare current entry */
        if (Connection->FromPin == FromPin && Connection->ToPin == ToPin &&
            Connection->FromSubDevice == FromSubDevice && Connection->ToSubDevice == ToSubDevice)
        {
            if (FromString && Connection->FromUnicodeString.Buffer)
            {
                if (!RtlCompareUnicodeString(FromString, &Connection->FromUnicodeString, TRUE))
                {
                    /* UnregisterPhysicalConnectionFromExternal */
                    bFound = TRUE;
                    break;
                }
            }
            else if (ToString && Connection->ToUnicodeString.Buffer)
            {
                if (!RtlCompareUnicodeString(ToString, &Connection->ToUnicodeString, TRUE))
                {
                    /* UnregisterPhysicalConnectionToExternal */
                    bFound = TRUE;
                    break;
                }
            }
            else
            {
                /* UnregisterPhysicalConnection */
                bFound = TRUE;
                break;
            }
        }
        Entry = Entry->Flink;
    }

    if (!bFound)
    {
         /* not found */
         Status = STATUS_NOT_FOUND;
         goto cleanup;
    }

    /* remove list entry */
    RemoveEntryList(&Connection->Entry);

    /* release resources */
    if (Connection->FromSubDevice)
        Connection->FromSubDevice->lpVtbl->Release(Connection->FromSubDevice);


    if (Connection->ToSubDevice)
        Connection->ToSubDevice->lpVtbl->Release(Connection->ToSubDevice);

    if (Connection->FromUnicodeString.Buffer)
        RtlFreeUnicodeString(&Connection->FromUnicodeString);

    if (Connection->ToUnicodeString.Buffer)
        RtlFreeUnicodeString(&Connection->ToUnicodeString);

    FreeItem(Connection, TAG_PORTCLASS);
    Status = STATUS_SUCCESS;

cleanup:

    if (FromSubDevice)
        FromSubDevice->lpVtbl->Release(FromSubDevice);

    if (ToSubDevice)
        ToSubDevice->lpVtbl->Release(ToSubDevice);

    return Status;

}

NTSTATUS
NTAPI
IUnregisterPhysicalConnection_fnUnregisterPhysicalConnection(
    IN IUnregisterPhysicalConnection* iface,
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
IUnregisterPhysicalConnection_fnUnregisterPhysicalConnectionToExternal(
    IN IUnregisterPhysicalConnection* iface,
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
IUnregisterPhysicalConnection_fnUnregisterPhysicalConnectionFromExternal(
    IN IUnregisterPhysicalConnection* iface,
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

static IUnregisterPhysicalConnectionVtbl vt_IUnregisterPhysicalConnection =
{
    IUnregisterPhysicalConnection_fnQueryInterface,
    IUnregisterPhysicalConnection_fnAddRef,
    IUnregisterPhysicalConnection_fnRelease,
    IUnregisterPhysicalConnection_fnUnregisterPhysicalConnection,
    IUnregisterPhysicalConnection_fnUnregisterPhysicalConnectionToExternal,
    IUnregisterPhysicalConnection_fnUnregisterPhysicalConnectionFromExternal
};

NTSTATUS
NTAPI
NewIUnregisterPhysicalConnection(
    OUT PUNREGISTERPHYSICALCONNECTION *OutConnection)
{
    IUnregisterPhysicalConnectionImpl * This = (IUnregisterPhysicalConnectionImpl*)AllocateItem(NonPagedPool, sizeof(IUnregisterPhysicalConnectionImpl), TAG_PORTCLASS);

    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IUnregisterPhysicalConnection;
    This->ref = 1;
    *OutConnection = (PUNREGISTERPHYSICALCONNECTION)&This->lpVtbl;
    return STATUS_SUCCESS;
}

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
);

static
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

    NewConnection = AllocateItem(NonPagedPool, sizeof(PHYSICAL_CONNECTION), TAG_PORTCLASS);
    if (!NewConnection)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    if (FromUnknown)
    {
        Status = FromUnknown->lpVtbl->QueryInterface(FromUnknown, &IID_ISubdevice, (PVOID*)&NewConnection->FromSubDevice);
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
        Status = ToUnknown->lpVtbl->QueryInterface(ToUnknown, &IID_ISubdevice, (PVOID*)&NewConnection->ToSubDevice);
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
        NewConnection->FromSubDevice->lpVtbl->Release(NewConnection->FromSubDevice);

    if (NewConnection->ToSubDevice)
        NewConnection->ToSubDevice->lpVtbl->Release(NewConnection->ToSubDevice);

    if (NewConnection->FromUnicodeString.Buffer)
        RtlFreeUnicodeString(&NewConnection->FromUnicodeString);

    if (NewConnection->ToUnicodeString.Buffer)
        RtlFreeUnicodeString(&NewConnection->ToUnicodeString);

     FreeItem(NewConnection, TAG_PORTCLASS);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
PcRegisterPhysicalConnection(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin)
{
    DPRINT("PcRegisterPhysicalConnection\n");
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!DeviceObject || !FromUnknown || !ToUnknown)
        return STATUS_INVALID_PARAMETER;

    return RegisterConnection(DeviceObject, FromUnknown, NULL, FromPin, ToUnknown, NULL, ToPin);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
PcRegisterPhysicalConnectionFromExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNICODE_STRING FromString,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin)
{
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!DeviceObject || !FromString || !ToUnknown)
        return STATUS_INVALID_PARAMETER;

    return RegisterConnection(DeviceObject, NULL, FromString, FromPin, ToUnknown, NULL, ToPin);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
PcRegisterPhysicalConnectionToExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNICODE_STRING ToString,
    IN  ULONG ToPin)
{
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!DeviceObject || !FromUnknown || !ToString)
        return STATUS_INVALID_PARAMETER;

    return RegisterConnection(DeviceObject, FromUnknown, NULL, FromPin, NULL, ToString, ToPin);
}
