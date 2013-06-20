$if (_WDMDDK_)
/******************************************************************************
 *                          Object Manager Functions                          *
 ******************************************************************************/
$endif (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_WDMDDK_)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG_PTR
FASTCALL
ObfDereferenceObject(
  _In_ PVOID Object);
#define ObDereferenceObject ObfDereferenceObject

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ObGetObjectSecurity(
  _In_ PVOID Object,
  _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
  _Out_ PBOOLEAN MemoryAllocated);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG_PTR
FASTCALL
ObfReferenceObject(
  _In_ PVOID Object);
#define ObReferenceObject ObfReferenceObject

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByHandle(
  _In_ HANDLE Handle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PVOID *Object,
  _Out_opt_ POBJECT_HANDLE_INFORMATION HandleInformation);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByPointer(
  _In_ PVOID Object,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
ObReleaseObjectSecurity(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ BOOLEAN MemoryAllocated);
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
NTSTATUS
NTAPI
ObInsertObject(
  _In_ PVOID Object,
  _Inout_opt_ PACCESS_STATE PassedAccessState,
  _In_opt_ ACCESS_MASK DesiredAccess,
  _In_ ULONG ObjectPointerBias,
  _Out_opt_ PVOID *NewObject,
  _Out_opt_ PHANDLE Handle);

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByPointer(
  _In_ PVOID Object,
  _In_ ULONG HandleAttributes,
  _In_opt_ PACCESS_STATE PassedAccessState,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PHANDLE Handle);

NTKERNELAPI
VOID
NTAPI
ObMakeTemporaryObject(
  _In_ PVOID Object);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryNameString(
  _In_ PVOID Object,
  _Out_writes_bytes_opt_(Length) POBJECT_NAME_INFORMATION ObjectNameInfo,
  _In_ ULONG Length,
  _Out_ PULONG ReturnLength);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryObjectAuditingByHandle(
  _In_ HANDLE Handle,
  _Out_ PBOOLEAN GenerateOnClose);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_VISTA)
$if (_WDMDDK_)
NTKERNELAPI
VOID
NTAPI
ObDereferenceObjectDeferDelete(
  _In_ PVOID Object);
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
BOOLEAN
NTAPI
ObIsKernelHandle(
  _In_ HANDLE Handle);
$endif (_NTIFS_)
#endif

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTKERNELAPI
NTSTATUS
NTAPI
ObRegisterCallbacks(
  _In_ POB_CALLBACK_REGISTRATION CallbackRegistration,
  _Outptr_ PVOID *RegistrationHandle);

NTKERNELAPI
VOID
NTAPI
ObUnRegisterCallbacks(
  _In_ PVOID RegistrationHandle);

NTKERNELAPI
USHORT
NTAPI
ObGetFilterVersion(VOID);

#endif /* (NTDDI_VERSION >= NTDDI_VISTASP1) */
$endif (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN7)
$if (_WDMDDK_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByHandleWithTag(
  _In_ HANDLE Handle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_ ULONG Tag,
  _Out_ PVOID *Object,
  _Out_opt_ POBJECT_HANDLE_INFORMATION HandleInformation);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG_PTR
FASTCALL
ObfReferenceObjectWithTag(
  _In_ PVOID Object,
  _In_ ULONG Tag);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByPointerWithTag(
  _In_ PVOID Object,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_ ULONG Tag);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG_PTR
FASTCALL
ObfDereferenceObjectWithTag(
  _In_ PVOID Object,
  _In_ ULONG Tag);

NTKERNELAPI
VOID
NTAPI
ObDereferenceObjectDeferDeleteWithTag(
  _In_ PVOID Object,
  _In_ ULONG Tag);

#define ObDereferenceObject ObfDereferenceObject
#define ObReferenceObject ObfReferenceObject
#define ObDereferenceObjectWithTag ObfDereferenceObjectWithTag
#define ObReferenceObjectWithTag ObfReferenceObjectWithTag
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByPointerWithTag(
  _In_ PVOID Object,
  _In_ ULONG HandleAttributes,
  _In_opt_ PACCESS_STATE PassedAccessState,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_ ULONG Tag,
  _Out_ PHANDLE Handle);

NTKERNELAPI
ULONG
NTAPI
ObGetObjectPointerCount(
    _In_ PVOID Object
);

$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

