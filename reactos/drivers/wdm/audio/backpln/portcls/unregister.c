/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/unregister.c
 * PURPOSE:         Unregisters a subdevice
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IUnregisterSubdeviceVtbl *lpVtbl;
    LONG ref;

}IUnregisterSubdeviceImpl;

NTSTATUS
NTAPI
IUnregisterSubdevice_fnQueryInterface(
    IUnregisterSubdevice* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IUnregisterSubdeviceImpl * This = (IUnregisterSubdeviceImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IUnregisterSubdevice) || 
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
IUnregisterSubdevice_fnAddRef(
    IUnregisterSubdevice* iface)
{
    IUnregisterSubdeviceImpl * This = (IUnregisterSubdeviceImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IUnregisterSubdevice_fnRelease(
    IUnregisterSubdevice* iface)
{
    IUnregisterSubdeviceImpl * This = (IUnregisterSubdeviceImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    return This->ref;
}

NTSTATUS
NTAPI
IUnregisterSubdevice_fnUnregisterSubdevice(
    IUnregisterSubdevice* iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PUNKNOWN  Unknown)
{
    PPCLASS_DEVICE_EXTENSION DeviceExtension;
    PLIST_ENTRY Entry;
    PSUBDEVICE_ENTRY SubDeviceEntry;
    PSYMBOLICLINK_ENTRY SymLinkEntry;
    ISubdevice *SubDevice;
    ULONG Found;
    ULONG Index;
    NTSTATUS Status;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    DeviceExtension = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension);

    /* look up our undocumented interface */
    Status = Unknown->lpVtbl->QueryInterface(Unknown, &IID_ISubdevice, (LPVOID)&SubDevice);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No ISubdevice interface\n");
        /* the provided port driver doesnt support ISubdevice */
        return STATUS_INVALID_PARAMETER;
    }

    Entry = DeviceExtension->SubDeviceList.Flink;
    Found = FALSE;
    /* loop subdevice entry list and search for the subdevice */
    while(Entry != &DeviceExtension->SubDeviceList)
    {
        SubDeviceEntry = (PSUBDEVICE_ENTRY)CONTAINING_RECORD(Entry, SUBDEVICE_ENTRY, Entry);
        if (SubDeviceEntry->SubDevice == SubDevice)
        {
            Found = TRUE;
            break;
        }
        Entry = Entry->Flink;
    }
    /* release the subdevice */
    SubDevice->lpVtbl->Release(SubDevice);

    if (!Found)
        return STATUS_NOT_FOUND;

    /* remove subdevice entry */
    RemoveEntryList(&SubDeviceEntry->Entry);

    /* loop our create items and disable the create handler */
    for(Index = 0; Index < DeviceExtension->MaxSubDevices; Index++)
    {
        if (!RtlCompareUnicodeString(&SubDeviceEntry->Name, &DeviceExtension->CreateItems[Index].ObjectClass, TRUE))
        {
            DeviceExtension->CreateItems[Index].Create = NULL;
            RtlInitUnicodeString(&DeviceExtension->CreateItems[Index].ObjectClass, NULL);
            break;
        }
    }

    /* now unregister device interfaces */
    while(!IsListEmpty(&SubDeviceEntry->SymbolicLinkList))
    {
        /* remove entry */
        Entry = RemoveHeadList(&SubDeviceEntry->SymbolicLinkList);
        /* get symlink entry */
        SymLinkEntry = (PSYMBOLICLINK_ENTRY)CONTAINING_RECORD(Entry, SYMBOLICLINK_ENTRY, Entry);

        /* unregister device interface */
        IoSetDeviceInterfaceState(&SymLinkEntry->SymbolicLink, FALSE);
        /* free symbolic link */
        RtlFreeUnicodeString(&SymLinkEntry->SymbolicLink);
        /* free sym entry */
        FreeItem(SymLinkEntry, TAG_PORTCLASS);
    }

    /* free subdevice entry */
    ExFreePool(SubDeviceEntry);

    return STATUS_SUCCESS;
}

static IUnregisterSubdeviceVtbl vt_IUnregisterSubdeviceVtbl =
{
    IUnregisterSubdevice_fnQueryInterface,
    IUnregisterSubdevice_fnAddRef,
    IUnregisterSubdevice_fnRelease,
    IUnregisterSubdevice_fnUnregisterSubdevice
};

NTSTATUS
NTAPI
NewIUnregisterSubdevice(
    OUT PUNREGISTERSUBDEVICE *OutDevice)
{
    IUnregisterSubdeviceImpl * This = AllocateItem(NonPagedPool, sizeof(IUnregisterSubdeviceImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IUnregisterSubdeviceVtbl;
    This->ref = 1;

    *OutDevice = (PUNREGISTERSUBDEVICE)&This->lpVtbl;
    return STATUS_SUCCESS;
}
