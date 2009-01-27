#include "private.h"

typedef struct
{
    IRegistryKeyVtbl * lpVtbl;

    LONG ref;
    HANDLE hKey;

}IRegistryKeyImpl;


static IRegistryKeyVtbl vt_IRegistryKey;


ULONG
STDMETHODCALLTYPE
IRegistryKey_fnAddRef(
    IN IRegistryKey* iface)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;

    DPRINT("IRegistryKey_AddRef: This %p\n", This);

    return InterlockedIncrement(&This->ref);
}

ULONG
STDMETHODCALLTYPE
IRegistryKey_fnRelease(
    IN IRegistryKey* iface)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;

    InterlockedDecrement(&This->ref);
    DPRINT1("IRegistryKey_fnRelease ref %u this %p entered\n", This->ref, This);
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
STDMETHODCALLTYPE
IRegistryKey_fnQueryInterface(
    IN IRegistryKey* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_fnQueryInterface entered\n");
    if (IsEqualGUIDAligned(refiid, &IID_IRegistryKey))
    {
        *Output = (PVOID)&This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    DPRINT("IRegistryKey_QueryInterface: This %p unknown iid\n", This, This->ref);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
STDMETHODCALLTYPE
IRegistryKey_fnDeleteKey(
    IN IRegistryKey* iface)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_fnDeleteKey entered\n");
    return ZwDeleteKey(This->hKey);
}

NTSTATUS
STDMETHODCALLTYPE
IRegistryKey_fnEnumerateKey(
    IN IRegistryKey* iface,
    IN ULONG  Index,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_fnEnumerateKey entered\n");
    return ZwEnumerateKey(This->hKey, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS
STDMETHODCALLTYPE
IRegistryKey_fnEnumerateKeyValue(
    IN IRegistryKey* iface,
    IN ULONG  Index,
    IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
    OUT PVOID  KeyValueInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_fnEnumerateKeyValue entered\n");
    return ZwEnumerateValueKey(This->hKey, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS
STDMETHODCALLTYPE
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

    DPRINT1("IRegistryKey_fnNewSubKey entered\n");

    InitializeObjectAttributes(&Attributes, SubKeyName, 0, This->hKey, NULL);
    Status = ZwCreateKey(&hKey, KEY_READ | KEY_WRITE, &Attributes, 0, NULL, 0, Disposition);
    if (!NT_SUCCESS(Status))
        return Status;


    NewThis = AllocateItem(NonPagedPool, sizeof(IRegistryKeyImpl), TAG_PORTCLASS);
    if (!NewThis)
    {
        ZwClose(hKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewThis->hKey = hKey;
    NewThis->ref = 1;
    NewThis->lpVtbl = &vt_IRegistryKey;
    *RegistrySubKey = (PREGISTRYKEY)&NewThis->lpVtbl;

    DPRINT1("IRegistryKey_fnNewSubKey RESULT %p\n", *RegistrySubKey );

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
IRegistryKey_fnQueryKey(
    IN IRegistryKey* iface,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_fnQueryKey entered\n");
    return ZwQueryKey(This->hKey, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS
STDMETHODCALLTYPE
IRegistryKey_fnQueryRegistryValues(
    IN IRegistryKey* iface,
    IN PRTL_QUERY_REGISTRY_TABLE  QueryTable,
    IN PVOID  Context  OPTIONAL)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_QueryRegistryValues: This %p\n", This);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
STDMETHODCALLTYPE
IRegistryKey_fnQueryValueKey(
    IN IRegistryKey* iface,
    IN PUNICODE_STRING  ValueName,
    IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
    OUT PVOID  KeyValueInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_fnQueryValueKey entered %p value %wZ\n", This, ValueName);
    return ZwQueryValueKey(This->hKey, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS
STDMETHODCALLTYPE
IRegistryKey_fnSetValueKey(
    IN IRegistryKey* iface,
    IN PUNICODE_STRING  ValueName  OPTIONAL,
    IN ULONG  Type,
    IN PVOID  Data,
    IN ULONG  DataSize
    )
{
    IRegistryKeyImpl * This = (IRegistryKeyImpl*)iface;
    DPRINT1("IRegistryKey_fnSetValueKey entered\n");
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
 * @unimplemented
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

    DPRINT1("PcNewRegistryKey entered\n");

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
        DPRINT1("fixme\n");
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

    This->hKey = hHandle;
    This->lpVtbl = &vt_IRegistryKey;
    This->ref = 1;

    *OutRegistryKey = (PREGISTRYKEY)&This->lpVtbl;
    DPRINT1("PcNewRegistryKey result %p\n", *OutRegistryKey);
    return STATUS_SUCCESS;
}

