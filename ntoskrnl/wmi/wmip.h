
#pragma once

extern POBJECT_TYPE WmipGuidObjectType;

#define GUID_STRING_LENGTH 36

typedef struct _WMIP_IRP_CONTEXT
{
    LIST_ENTRY GuidObjectListHead;
} WMIP_IRP_CONTEXT, *PWMIP_IRP_CONTEXT;

typedef struct _WMIP_GUID_OBJECT
{
    GUID Guid;
    PIRP Irp;
    LIST_ENTRY IrpLink;
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
    _In_ LPCGUID Guid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PHANDLE OutGuidObjectHandle,
    _Outptr_ PVOID *OutGuidObject);

NTSTATUS
NTAPI
WmipOpenGuidObjectByName(
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PHANDLE OutGuidObjectHandle,
    _Outptr_ PVOID *OutGuidObject);

NTSTATUS
NTAPI
WmipQueryRawSMBiosTables(
    _Inout_ ULONG *InOutBufferSize,
    _Out_opt_ PVOID OutBuffer);

