/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    mmfuncs.h

Abstract:

    Functions definitions for the Memory Manager.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _MMFUNCS_H
#define _MMFUNCS_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef NTOS_MODE_USER

//
// Section Functions
//
NTSTATUS
NTAPI
MmCreateSection(
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT File OPTIONAL
);

NTSTATUS
NTAPI
MmMapViewOfSection(
    IN PVOID SectionObject,
    IN PEPROCESS Process,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PULONG ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
);

NTSTATUS
NTAPI
MmUnmapViewOfSection(
    struct _EPROCESS* Process,
    PVOID BaseAddress
);

#endif

//
// Native calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAreMappedFilesTheSame(
    IN PVOID File1MappedAsAnImage,
    IN PVOID File2MappedAsFile
);

NTSTATUS
NTAPI
NtAllocateUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN OUT PULONG NumberOfPages,
    IN OUT PULONG UserPfnArray
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePagingFile(
    IN PUNICODE_STRING FileName,
    IN PLARGE_INTEGER InitialSize,
    IN PLARGE_INTEGER MaxiumSize,
    IN ULONG Reserved
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection OPTIONAL,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtExtendSection(
    IN HANDLE SectionHandle,
    IN PLARGE_INTEGER NewMaximumSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytesToFlush,
    OUT PULONG NumberOfBytesFlushed OPTIONAL
);

NTSTATUS
NTAPI
NtFreeUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN OUT PULONG NumberOfPages,
    IN OUT PULONG UserPfnArray
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN PSIZE_T RegionSize,
    IN ULONG FreeType
);


NTSTATUS
NTAPI
NtGetWriteWatch(
    IN HANDLE ProcessHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG RegionSize,
    IN PVOID *UserAddressArray,
    OUT PULONG EntriesInUserAddressArray,
    OUT PULONG Granularity
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    ULONG NumberOfBytesToLock,
    PULONG NumberOfBytesLocked
);

NTSTATUS
NTAPI
NtMapUserPhysicalPages(
    IN PVOID *VirtualAddresses,
    IN ULONG NumberOfPages,
    IN OUT PULONG UserPfnArray
);

NTSTATUS
NTAPI
NtMapUserPhysicalPagesScatter(
    IN PVOID *VirtualAddresses,
    IN ULONG NumberOfPages,
    IN OUT PULONG UserPfnArray
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG AccessProtection
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN ULONG *NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN SIZE_T Length,
    OUT PSIZE_T ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN SIZE_T Length,
    OUT PSIZE_T ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN SIZE_T NumberOfBytesToRead,
    OUT PSIZE_T NumberOfBytesRead
);

NTSTATUS
NTAPI
NtResetWriteWatch(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN SIZE_T RegionSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN SIZE_T  NumberOfBytesToUnlock,
    OUT PSIZE_T NumberOfBytesUnlocked OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  BaseAddress,
    IN PVOID Buffer,
    IN SIZE_T NumberOfBytesToWrite,
    OUT PSIZE_T NumberOfBytesWritten
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAreMappedFilesTheSame(
    IN PVOID File1MappedAsAnImage,
    IN PVOID File2MappedAsFile
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePagingFile(
    IN PUNICODE_STRING FileName,
    IN PLARGE_INTEGER InitialSize,
    IN PLARGE_INTEGER MaxiumSize,
    IN ULONG Reserved
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection OPTIONAL,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwExtendSection(
    IN HANDLE SectionHandle,
    IN PLARGE_INTEGER NewMaximumSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN PSIZE_T RegionSize,
    IN ULONG FreeType
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLockVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    SIZE_T NumberOfBytesToLock,
    PSIZE_T NumberOfBytesLocked
);

NTSYSAPI
NTSTATUS
NTAPI
ZwMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG AccessProtection
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN ULONG *NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN SIZE_T Length,
    OUT PSIZE_T ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN SIZE_T Length,
    OUT PSIZE_T ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN SIZE_T NumberOfBytesToRead,
    OUT PSIZE_T NumberOfBytesRead
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN SIZE_T  NumberOfBytesToUnlock,
    OUT PSIZE_T NumberOfBytesUnlocked OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  BaseAddress,
    IN PVOID Buffer,
    IN ULONG NumberOfBytesToWrite,
    OUT PULONG NumberOfBytesWritten
);

#endif
