
#pragma once

#define GUID_STRING_LENGTH 36

typedef struct _WMIP_GUID_OBJECT
{
    GUID Guid;
} WMIP_GUID_OBJECT, *PWMIP_GUID_OBJECT;


_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
NTSTATUS
NTAPI
WmipDriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath);

NTSTATUS
NTAPI
WmipInitializeGuidObjectType(
    VOID);

NTSTATUS
NTAPI
WmipOpenGuidObject(
    POBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    KPROCESSOR_MODE AccessMode,
    PHANDLE OutGuidObjectHandle,
    PVOID *OutGuidObject);
