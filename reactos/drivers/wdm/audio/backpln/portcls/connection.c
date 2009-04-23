/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/connection.c
 * PURPOSE:         portcls physical connection registration
 * PROGRAMMER:      Johannes Anderwald
 */


#include "private.h"


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
    UNICODE_STRING FromUnicodeString = {0, 0, 0};
    UNICODE_STRING ToUnicodeString  = {0, 0, 0};
    ISubdevice * FromSubDevice = NULL;
    ISubdevice * ToSubDevice = NULL;
    PPCLASS_DEVICE_EXTENSION DeviceExt;

    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    NTSTATUS Status = STATUS_SUCCESS;

    if (FromUnknown)
    {
        Status = FromUnknown->lpVtbl->QueryInterface(FromUnknown, &IID_ISubdevice, (PVOID*)&FromSubDevice);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
    }
    else
    {
        if (!RtlCreateUnicodeString(&FromUnicodeString, (PCWSTR)FromString))
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ToUnknown)
    {
        Status = ToUnknown->lpVtbl->QueryInterface(ToUnknown, &IID_ISubdevice, (PVOID*)&ToSubDevice);
    }
    else
    {
        if (!RtlCreateUnicodeString(&ToUnicodeString, (PCWSTR)ToString))
            Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(Status))
    {
        goto cleanup;
    }

    NewConnection = AllocateItem(NonPagedPool, sizeof(PHYSICAL_CONNECTION), TAG_PORTCLASS);
    if (!NewConnection)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    NewConnection->FromPin = FromPin;
    NewConnection->FromSubDevice = FromSubDevice;
    NewConnection->FromUnicodeString = FromUnicodeString.Buffer;
    NewConnection->ToPin = ToPin;
    NewConnection->ToSubDevice = ToSubDevice;
    NewConnection->ToUnicodeString = ToUnicodeString.Buffer;


    InsertTailList(&DeviceExt->PhysicalConnectionList, &NewConnection->Entry);
    return STATUS_SUCCESS;

cleanup:

    if (FromSubDevice)
        FromSubDevice->lpVtbl->Release(FromSubDevice);

    if (ToSubDevice)
        ToSubDevice->lpVtbl->Release(ToSubDevice);

    if (FromUnicodeString.Buffer)
        RtlFreeUnicodeString(&FromUnicodeString);

    if (ToUnicodeString.Buffer)
        RtlFreeUnicodeString(&ToUnicodeString);

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
