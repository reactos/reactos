#ifndef __NTOSKRNL_INCLUDE_INTERNAL_TEST_H
#define __NTOSKRNL_INCLUDE_INTERNAL_TEST_H

typedef VOID STDCALL
PExFreePool(PVOID Block);

typedef PMDL STDCALL
PMmCreateMdl(PMDL Mdl,
  PVOID Base,
  ULONG Length);

typedef VOID STDCALL
PMmProbeAndLockPages(PMDL Mdl,
  KPROCESSOR_MODE AccessMode,
  LOCK_OPERATION Operation);

typedef VOID FASTCALL
PObDereferenceObject(PVOID Object);

typedef NTSTATUS STDCALL
PObReferenceObjectByHandle(HANDLE Handle,
  ACCESS_MASK DesiredAccess,
  POBJECT_TYPE ObjectType,
  KPROCESSOR_MODE AccessMode,
  PVOID* Object,
  POBJECT_HANDLE_INFORMATION HandleInformation);


NTSTATUS STDCALL
MiLockVirtualMemory(HANDLE ProcessHandle,
  PVOID BaseAddress,
  ULONG NumberOfBytesToLock,
  PULONG NumberOfBytesLocked,
  PObReferenceObjectByHandle pObReferenceObjectByHandle,
  PMmCreateMdl pMmCreateMdl,
  PObDereferenceObject pObDereferenceObject,
  PMmProbeAndLockPages pMmProbeAndLockPages,
  PExFreePool pExFreePool);

NTSTATUS FASTCALL
MiQueryVirtualMemory (IN HANDLE ProcessHandle,
                      IN PVOID Address,
                      IN CINT VirtualMemoryInformationClass,
                      OUT PVOID VirtualMemoryInformation,
                      IN ULONG Length,
                      OUT PULONG ResultLength);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_TEST_H */
