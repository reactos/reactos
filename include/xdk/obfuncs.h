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

$endif

