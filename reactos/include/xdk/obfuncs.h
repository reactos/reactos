/******************************************************************************
 *                          Object Manager Functions                          *
 ******************************************************************************/

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WIN2K)

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

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
VOID
NTAPI
ObDereferenceObjectDeferDelete(
  IN PVOID Object);
#endif

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

#if (NTDDI_VERSION >= NTDDI_WIN7)

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

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

$endif

