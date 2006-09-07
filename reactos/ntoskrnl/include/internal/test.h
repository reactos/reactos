#ifndef __NTOSKRNL_INCLUDE_INTERNAL_TEST_H
#define __NTOSKRNL_INCLUDE_INTERNAL_TEST_H

typedef VOID
NTAPI
PExFreePool(PVOID Block);

typedef PMDL
NTAPI
PMmCreateMdl(
    PMDL Mdl,
    PVOID Base,
    ULONG Length
);

typedef VOID
NTAPI
PMmProbeAndLockPages(
    PMDL Mdl,
    KPROCESSOR_MODE AccessMode,
    LOCK_OPERATION Operation
);

typedef LONG_PTR
FASTCALL
PObDereferenceObject(PVOID Object);

typedef NTSTATUS
NTAPI
PObReferenceObjectByHandle(
    HANDLE Handle,
    ACCESS_MASK DesiredAccess,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    PVOID* Object,
    POBJECT_HANDLE_INFORMATION HandleInformation
);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_TEST_H */
