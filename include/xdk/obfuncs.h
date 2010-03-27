$if (_WDMDDK_)
/******************************************************************************
 *                          Object Manager Functions                          *
 ******************************************************************************/
$endif (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_WDMDDK_)
NTKERNELAPI
LONG_PTR
FASTCALL
ObfDereferenceObject(
  IN PVOID Object);
#define ObDereferenceObject ObfDereferenceObject

NTKERNELAPI
NTSTATUS
NTAPI
ObGetObjectSecurity(
  IN PVOID Object,
  OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
  OUT PBOOLEAN MemoryAllocated);

NTKERNELAPI
LONG_PTR
FASTCALL
ObfReferenceObject(
  IN PVOID Object);
#define ObReferenceObject ObfReferenceObject

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByHandle(
  IN HANDLE Handle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_TYPE ObjectType OPTIONAL,
  IN KPROCESSOR_MODE AccessMode,
  OUT PVOID *Object,
  OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByPointer(
  IN PVOID Object,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_TYPE ObjectType OPTIONAL,
  IN KPROCESSOR_MODE AccessMode);

NTKERNELAPI
VOID
NTAPI
ObReleaseObjectSecurity(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN BOOLEAN MemoryAllocated);
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
NTSTATUS
NTAPI
ObInsertObject(
  IN PVOID Object,
  IN OUT PACCESS_STATE PassedAccessState OPTIONAL,
  IN ACCESS_MASK DesiredAccess OPTIONAL,
  IN ULONG ObjectPointerBias,
  OUT PVOID *NewObject OPTIONAL,
  OUT PHANDLE Handle OPTIONAL);

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByPointer(
  IN PVOID Object,
  IN ULONG HandleAttributes,
  IN PACCESS_STATE PassedAccessState OPTIONAL,
  IN ACCESS_MASK DesiredAccess OPTIONAL,
  IN POBJECT_TYPE ObjectType OPTIONAL,
  IN KPROCESSOR_MODE AccessMode,
  OUT PHANDLE Handle);

NTKERNELAPI
VOID
NTAPI
ObMakeTemporaryObject(
  IN PVOID Object);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryNameString(
  IN PVOID Object,
  OUT POBJECT_NAME_INFORMATION ObjectNameInfo OPTIONAL,
  IN ULONG Length,
  OUT PULONG ReturnLength);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryObjectAuditingByHandle(
  IN HANDLE Handle,
  OUT PBOOLEAN GenerateOnClose);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_VISTA)
$if (_WDMDDK_)
NTKERNELAPI
VOID
NTAPI
ObDereferenceObjectDeferDelete(
  IN PVOID Object);
$endif (_WDMDDK_)
$if (_NTIFS_)

NTKERNELAPI
BOOLEAN
NTAPI
ObIsKernelHandle(
  IN HANDLE Handle);
$endif (_NTIFS_)
#endif

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_VISTASP1)
NTKERNELAPI
NTSTATUS
NTAPI
ObRegisterCallbacks(
  IN POB_CALLBACK_REGISTRATION CallbackRegistration,
  OUT PVOID *RegistrationHandle);

NTKERNELAPI
VOID
NTAPI
ObUnRegisterCallbacks(
  IN PVOID RegistrationHandle);

NTKERNELAPI
USHORT
NTAPI
ObGetFilterVersion(VOID);

#endif /* (NTDDI_VERSION >= NTDDI_VISTASP1) */
$endif (_WDMDDK_)

#if (NTDDI_VERSION >= NTDDI_WIN7)
$if (_WDMDDK_)
NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByHandleWithTag(
  IN HANDLE Handle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_TYPE ObjectType OPTIONAL,
  IN KPROCESSOR_MODE AccessMode,
  IN ULONG Tag,
  OUT PVOID *Object,
  OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL);

NTKERNELAPI
LONG_PTR
FASTCALL
ObfReferenceObjectWithTag(
  IN PVOID Object,
  IN ULONG Tag);

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByPointerWithTag(
  IN PVOID Object,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_TYPE ObjectType OPTIONAL,
  IN KPROCESSOR_MODE AccessMode,
  IN ULONG Tag);

NTKERNELAPI
LONG_PTR
FASTCALL
ObfDereferenceObjectWithTag(
  IN PVOID Object,
  IN ULONG Tag);

NTKERNELAPI
VOID
NTAPI
ObDereferenceObjectDeferDeleteWithTag(
  IN PVOID Object,
  IN ULONG Tag);

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
  IN PVOID Object,
  IN ULONG HandleAttributes,
  IN PACCESS_STATE PassedAccessState OPTIONAL,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_TYPE ObjectType OPTIONAL,
  IN KPROCESSOR_MODE AccessMode,
  IN ULONG Tag,
  OUT PHANDLE Handle);
$endif (_NTIFS_)
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

