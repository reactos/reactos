/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/registry.c
 * PURPOSE:         Registry access object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IRegistryKeyVtbl * lpVtbl;

    LONG ref;
    HANDLE hKey;
    BOOL Deleted;

}IRegistryKeyImpl;


static IRegistryKeyVtbl vt_IRegistryKey;


ULONG
NTAPI
IRegistryKey_fnAddRef(
    IN IRegistryKey* iface)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;

    DPRINT("IRegistryKey_AddRef: This %p\n", This);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IRegistryKey_fnRelease(
    IN IRegistryKey* iface)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;

    InterlockedDecrement(&This->ref);
    DPRINT("IRegistryKey_fnRelease ref %u this %p entered\n", This->ref, This);
    if (This->ref == 0)
    {
        if (This->hKey)
        {
            ZwClose(This->hKey);
        }
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

NTSTATUS
NTAPI
IRegistryKey_fnQueryInterface(
    IN IRegistryKey* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT("IRegistryKey_fnQueryInterface entered\n");
    if (IsEqualGUIDAligned(refiid, &IID_IRegistryKey))
    {
        *Output = (PVOID)&This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    DPRINT("IRegistryKey_QueryInterface: This %p unknown iid\n", This, This->ref);
    DbgBreakPoint();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
IRegistryKey_fnDeleteKey(
    IN IRegistryKey* iface)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    NTSTATUS Status;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    Status = ZwDeleteKey(This->hKey);
    if (NT_SUCCESS(Status))
    {
        This->Deleted = TRUE;
    }
    return Status;
}

NTSTATUS
NTAPI
IRegistryKey_fnEnumerateKey(
    IN IRegistryKey* iface,
    IN ULONG  Index,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwEnumerateKey(This->hKey, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS
NTAPI
IRegistryKey_fnEnumerateKeyValue(
    IN IRegistryKey* iface,
    IN ULONG  Index,
    IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
    OUT PVOID  KeyValueInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwEnumerateValueKey(This->hKey, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS
NTAPI
IRegistryKey_fnNewSubKey(
    IN IRegistryKey* iface,
    OUT PREGISTRYKEY  *RegistrySubKey,
    IN PUNKNOWN  OuterUnknown,
    IN ACCESS_MASK  DesiredAccess,
    IN PUNICODE_STRING  SubKeyName,
    IN ULONG  CreateOptions,
    OUT PULONG  Disposition  OPTIONAL)
{
    OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    HANDLE hKey;
    IRegistryKeyImpl * NewThis, *This = (IRegistryKeyImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    DPRINT("IRegistryKey_fnNewSubKey entered %S\n", SubKeyName->Buffer);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    InitializeObjectAttributes(&Attributes, SubKeyName, 0, This->hKey, NULL);
    Status = ZwCreateKey(&hKey, KEY_READ | KEY_WRITE, &Attributes, 0, NULL, 0, Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IRegistryKey_fnNewSubKey failed with %x\n", Status);
        DbgBreakPoint();
        return Status;
    }

    NewThis = AllocateItem(NonPagedPool, sizeof(IRegistryKeyImpl), TAG_PORTCLASS);
    if (!NewThis)
    {
        ZwClose(hKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (OuterUnknown)
        OuterUnknown->lpVtbl->AddRef(OuterUnknown);

    NewThis->hKey = hKey;
    NewThis->ref = 1;
    NewThis->lpVtbl = &vt_IRegistryKey;
    *RegistrySubKey = (PREGISTRYKEY)&NewThis->lpVtbl;

    DPRINT("IRegistryKey_fnNewSubKey RESULT %p\n", *RegistrySubKey );

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IRegistryKey_fnQueryKey(
    IN IRegistryKey* iface,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwQueryKey(This->hKey, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS
NTAPI
IRegistryKey_fnQueryRegistryValues(
    IN IRegistryKey* iface,
    IN PRTL_QUERY_REGISTRY_TABLE  QueryTable,
    IN PVOID  Context  OPTIONAL)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return RtlQueryRegistryValues(RTL_REGISTRY_HANDLE, (PCWSTR)This->hKey, QueryTable, Context, NULL);
}

NTSTATUS
NTAPI
IRegistryKey_fnQueryValueKey(
    IN IRegistryKey* iface,
    IN PUNICODE_STRING  ValueName,
    IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
    OUT PVOID  KeyValueInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;

    DPRINT("IRegistryKey_fnQueryValueKey entered %p value %wZ\n", This, ValueName);
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwQueryValueKey(This->hKey, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS
NTAPI
IRegistryKey_fnSetValueKey(
    IN IRegistryKey* iface,
    IN PUNICODE_STRING  ValueName  OPTIONAL,
    IN ULONG  Type,
    IN PVOID  Data,
    IN ULONG  DataSize
    )
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT("IRegistryKey_fnSetValueKey entered %S\n", ValueName->Buffer);
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwSetValueKey(This->hKey, ValueName, 0, Type, Data, DataSize);
}

static IRegistryKeyVtbl vt_IRegistryKey =
{
    /* IUnknown methods */
    IRegistryKey_fnQueryInterface,
    IRegistryKey_fnAddRef,
    IRegistryKey_fnRelease,
    /* IRegistryKey methods */
    IRegistryKey_fnQueryKey,
    IRegistryKey_fnEnumerateKey,
    IRegistryKey_fnQueryValueKey,
    IRegistryKey_fnEnumerateKeyValue,
    IRegistryKey_fnSetValueKey,
    IRegistryKey_fnQueryRegistryValues,
    IRegistryKey_fnNewSubKey,
    IRegistryKey_fnDeleteKey
};

/*
 * @implemented
 */
NTSTATUS NTAPI
PcNewRegistryKey(
    OUT PREGISTRYKEY* OutRegistryKey,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  ULONG RegistryKeyType,
    IN  ACCESS_MASK DesiredAccess,
    IN  PVOID DeviceObject OPTIONAL,
    IN  PVOID SubDevice OPTIONAL,
    IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN  ULONG CreateOptions OPTIONAL,
    OUT PULONG Disposition OPTIONAL)
{
    HANDLE hHandle;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    IRegistryKeyImpl * This;
    PPCLASS_DEVICE_EXTENSION DeviceExt;

    DPRINT("PcNewRegistryKey entered\n");

    if (!OutRegistryKey)
        return STATUS_INVALID_PARAMETER;

    if (RegistryKeyType != GeneralRegistryKey &&
        RegistryKeyType != DeviceRegistryKey &&
        RegistryKeyType != DriverRegistryKey &&
        RegistryKeyType != HwProfileRegistryKey &&
        RegistryKeyType != DeviceInterfaceRegistryKey)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* check for the key type */
    if (RegistryKeyType == GeneralRegistryKey)
    {
        /* do we have the required object attributes */
        if (!ObjectAttributes)
        {
            /* object attributes is mandatory */
            return STATUS_INVALID_PARAMETER;
        }
        /* try to open the key */
        Status = ZwOpenKey(&hHandle, DesiredAccess, ObjectAttributes);
    }
    else if (RegistryKeyType == DeviceRegistryKey ||
             RegistryKeyType == DriverRegistryKey ||
             RegistryKeyType == HwProfileRegistryKey)
    {
        /* check for HwProfileRegistryKey case */
        if (RegistryKeyType == HwProfileRegistryKey)
        {
             /* IoOpenDeviceRegistryKey used different constant */
            RegistryKeyType = PLUGPLAY_REGKEY_CURRENT_HWPROFILE;
        }

        /* obtain the new device extension */
        DeviceExt = (PPCLASS_DEVICE_EXTENSION) ((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;

        Status = IoOpenDeviceRegistryKey(DeviceExt->PhysicalDeviceObject, RegistryKeyType, DesiredAccess, &hHandle);
    }
    else if (RegistryKeyType == DeviceInterfaceRegistryKey)
    {
        /* FIXME */
        UNIMPLEMENTED
        DbgBreakPoint();
    }

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    This = AllocateItem(NonPagedPool, sizeof(IRegistryKeyImpl), TAG_PORTCLASS);
    if (!This)
    {
        ZwClose(hHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (OuterUnknown)
        OuterUnknown->lpVtbl->AddRef(OuterUnknown);

    This->hKey = hHandle;
    This->lpVtbl = &vt_IRegistryKey;
    This->ref = 1;

    *OutRegistryKey = (PREGISTRYKEY)&This->lpVtbl;
    DPRINT("PcNewRegistryKey result %p\n", *OutRegistryKey);
    return STATUS_SUCCESS;
}

