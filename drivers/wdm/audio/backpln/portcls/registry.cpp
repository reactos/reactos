/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/registry.cpp
 * PURPOSE:         Registry access object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CRegistryKey : public CUnknownImpl<IRegistryKey>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IRegistryKey;
    CRegistryKey(IUnknown * OuterUnknown, HANDLE hKey, BOOL CanDelete) :
        m_hKey(hKey),
        m_Deleted(FALSE),
        m_CanDelete(CanDelete)
    {
    }
    virtual ~CRegistryKey();

protected:

    HANDLE m_hKey;
    BOOL m_Deleted;
    BOOL m_CanDelete;
};

CRegistryKey::~CRegistryKey()
{
    if (!m_Deleted)
    {
         // close key only when has not been deleted yet
         ZwClose(m_hKey);
    }
}

NTSTATUS
NTAPI
CRegistryKey::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    DPRINT("CRegistryKey::QueryInterface entered\n");
    if (IsEqualGUIDAligned(refiid, IID_IRegistryKey) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PREGISTRYKEY(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("CRegistryKey::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CRegistryKey::DeleteKey()
{
    NTSTATUS Status;
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_Deleted)
    {
        // key already deleted
        return STATUS_INVALID_HANDLE;
    }

    if (!m_CanDelete)
    {
        // only general keys can be deleted
        return STATUS_ACCESS_DENIED;
    }

    // delete key
    Status = ZwDeleteKey(m_hKey);
    if (NT_SUCCESS(Status))
    {
        m_Deleted = TRUE;
        m_hKey = NULL;
    }
    return Status;
}

NTSTATUS
NTAPI
CRegistryKey::EnumerateKey(
    IN ULONG  Index,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwEnumerateKey(m_hKey, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS
NTAPI
CRegistryKey::EnumerateValueKey(
    IN ULONG  Index,
    IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
    OUT PVOID  KeyValueInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwEnumerateValueKey(m_hKey, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
}

NTSTATUS
NTAPI
CRegistryKey::NewSubKey(
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
    CRegistryKey * RegistryKey;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    DPRINT("CRegistryKey::NewSubKey entered %S\n", SubKeyName->Buffer);

    if (m_Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    InitializeObjectAttributes(&Attributes, SubKeyName, OBJ_INHERIT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_KERNEL_HANDLE, m_hKey, NULL);
    Status = ZwCreateKey(&hKey, DesiredAccess, &Attributes, 0, NULL, CreateOptions, Disposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("CRegistryKey::NewSubKey failed with %x\n", Status);
        return Status;
    }

    RegistryKey = new(NonPagedPool, TAG_PORTCLASS)CRegistryKey(OuterUnknown, hKey, TRUE);
    if (!RegistryKey)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = RegistryKey->QueryInterface(IID_IRegistryKey, (PVOID*)RegistrySubKey);

    if (!NT_SUCCESS(Status))
    {
        delete RegistryKey;
        return Status;
    }

    DPRINT("CRegistryKey::NewSubKey RESULT %p\n", *RegistrySubKey);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CRegistryKey::QueryKey(
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwQueryKey(m_hKey, KeyInformationClass, KeyInformation, Length, ResultLength);
}

NTSTATUS
NTAPI
CRegistryKey::QueryRegistryValues(
    IN PRTL_QUERY_REGISTRY_TABLE  QueryTable,
    IN PVOID  Context  OPTIONAL)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return RtlQueryRegistryValues(RTL_REGISTRY_HANDLE, (PCWSTR)m_hKey, QueryTable, Context, NULL);
}

NTSTATUS
NTAPI
CRegistryKey::QueryValueKey(
    IN PUNICODE_STRING  ValueName,
    IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
    OUT PVOID  KeyValueInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength)
{
    NTSTATUS Status;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    Status = ZwQueryValueKey(m_hKey, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
    DPRINT("CRegistryKey::QueryValueKey entered %p value %wZ Status %x\n", this, ValueName, Status);
    return Status;
}

NTSTATUS
NTAPI
CRegistryKey::SetValueKey(
    IN PUNICODE_STRING  ValueName  OPTIONAL,
    IN ULONG  Type,
    IN PVOID  Data,
    IN ULONG  DataSize
    )
{
    DPRINT("CRegistryKey::SetValueKey entered %S\n", ValueName->Buffer);
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_Deleted)
    {
        return STATUS_INVALID_HANDLE;
    }

    return ZwSetValueKey(m_hKey, ValueName, 0, Type, Data, DataSize);
}

NTSTATUS
NTAPI
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
    CRegistryKey * RegistryKey;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;
    ISubdevice * Device;
    PSYMBOLICLINK_ENTRY SymEntry;
    BOOL CanDelete = FALSE;

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

    // check for the key type
    if (RegistryKeyType == GeneralRegistryKey)
    {
        // do we have the required object attributes
        if (!ObjectAttributes)
        {
            // object attributes is mandatory
            return STATUS_INVALID_PARAMETER;
        }
        // try to create the key
        Status = ZwCreateKey(&hHandle, DesiredAccess, ObjectAttributes, 0, NULL, CreateOptions, Disposition);

        // key can be deleted
        CanDelete = TRUE;
    }
    else if (RegistryKeyType == DeviceRegistryKey ||
             RegistryKeyType == DriverRegistryKey ||
             RegistryKeyType == HwProfileRegistryKey)
    {
        // check for HwProfileRegistryKey case
        if (RegistryKeyType == HwProfileRegistryKey)
        {
             // IoOpenDeviceRegistryKey used different constant
            RegistryKeyType = PLUGPLAY_REGKEY_CURRENT_HWPROFILE | PLUGPLAY_REGKEY_DEVICE;
        }

        // obtain the new device extension
        DeviceExt = (PPCLASS_DEVICE_EXTENSION) ((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;

        Status = IoOpenDeviceRegistryKey(DeviceExt->PhysicalDeviceObject, RegistryKeyType, DesiredAccess, &hHandle);
    }
    else if (RegistryKeyType == DeviceInterfaceRegistryKey)
    {
        if (SubDevice == NULL)
        {
            // invalid parameter
            return STATUS_INVALID_PARAMETER;
        }

        // look up our undocumented interface
        Status = ((PUNKNOWN)SubDevice)->QueryInterface(IID_ISubdevice, (LPVOID*)&Device);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("No ISubdevice interface\n");
            // invalid parameter
            return STATUS_INVALID_PARAMETER;
        }

        // get the subdevice descriptor
        Status = Device->GetDescriptor(&SubDeviceDescriptor);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Failed to get subdevice descriptor %x\n", Status);
            ((PUNKNOWN)SubDevice)->Release();
            return STATUS_UNSUCCESSFUL;
        }

        // is there an registered device interface
        if (IsListEmpty(&SubDeviceDescriptor->SymbolicLinkList))
        {
            DPRINT("No device interface registered\n");
            ((PUNKNOWN)SubDevice)->Release();
            return STATUS_UNSUCCESSFUL;
        }

        // get the first symbolic link
        SymEntry = (PSYMBOLICLINK_ENTRY)CONTAINING_RECORD(SubDeviceDescriptor->SymbolicLinkList.Flink, SYMBOLICLINK_ENTRY, Entry);

        // open device interface
        Status = IoOpenDeviceInterfaceRegistryKey(&SymEntry->SymbolicLink, DesiredAccess, &hHandle);

        // release subdevice interface
        ((PUNKNOWN)SubDevice)->Release();
    }

    // check for success
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PcNewRegistryKey failed with %lx\n", Status);
        return Status;
    }

    // allocate new registry key object
    RegistryKey = new(NonPagedPool, TAG_PORTCLASS)CRegistryKey(OuterUnknown, hHandle, CanDelete);
    if (!RegistryKey)
    {
        // not enough memory
        ZwClose(hHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // query for interface
    Status = RegistryKey->QueryInterface(IID_IRegistryKey, (PVOID*)OutRegistryKey);

    if (!NT_SUCCESS(Status))
    {
        // out of memory
        delete RegistryKey;
    }

    DPRINT("PcNewRegistryKey result %p\n", *OutRegistryKey);
    return STATUS_SUCCESS;
}
