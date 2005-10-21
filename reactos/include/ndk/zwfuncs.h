/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/zwfuncs.h
 * PURPOSE:         Defintions for Native Functions not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _ZWFUNCS_H
#define _ZWFUNCS_H

/* DEPENDENCIES **************************************************************/
#define _WMIKM_
#include <evntrace.h>
#include "lpctypes.h"
#include "dbgktypes.h"
#include "kdtypes.h"

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

NTSTATUS
NTAPI
NtAcceptConnectPort(
    PHANDLE PortHandle,
    PVOID PortContext OPTIONAL,
    PPORT_MESSAGE ConnectionRequest,
    BOOLEAN AcceptConnection,
    PPORT_VIEW ServerView OPTIONAL,
    PREMOTE_PORT_VIEW ClientView OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAcceptConnectPort(
    PHANDLE PortHandle,
    PVOID PortContext OPTIONAL,
    PPORT_MESSAGE ConnectionRequest,
    BOOLEAN AcceptConnection,
    PPORT_VIEW ServerView OPTIONAL,
    PREMOTE_PORT_VIEW ClientView OPTIONAL
);

NTSTATUS
NTAPI
NtAccessCheck(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT PULONG ReturnLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheck(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT PULONG ReturnLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN ObjectCreation,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus,
    OUT PBOOLEAN GenerateOnClose
);

NTSTATUS
NTAPI
NtAddAtom(
    IN     PWSTR  AtomName,
    IN     ULONG AtomNameLength,
    IN OUT PRTL_ATOM  Atom
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAddAtom(
    IN     PWSTR  AtomName,
    IN     ULONG AtomNameLength,
    IN OUT PRTL_ATOM  Atom
);

NTSTATUS
NTAPI
NtAddBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAddBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustGroupsToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN  ResetToDefault,
    IN PTOKEN_GROUPS  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_GROUPS  PreviousState OPTIONAL,
    OUT PULONG  ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustGroupsToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN  ResetToDefault,
    IN PTOKEN_GROUPS  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_GROUPS  PreviousState,
    OUT PULONG  ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    IN HANDLE  TokenHandle,
    IN BOOLEAN  DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_PRIVILEGES  PreviousState,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken(
    IN HANDLE  TokenHandle,
    IN BOOLEAN  DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_PRIVILEGES  PreviousState,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
NTAPI
NtAlertThread(
    IN HANDLE ThreadHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread(
    IN HANDLE ThreadHandle
);

NTSTATUS
NTAPI
NtAllocateLocallyUniqueId(
    OUT LUID *LocallyUniqueId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
    OUT PLUID Luid
);

NTSTATUS
NTAPI
NtAllocateUuids(
    PULARGE_INTEGER Time,
    PULONG Range,
    PULONG Sequence,
    PUCHAR Seed
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateUuids(
    PULARGE_INTEGER Time,
    PULONG Range,
    PULONG Sequence,
    PUCHAR Seed
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG  ZeroBits,
    IN OUT PULONG  RegionSize,
    IN ULONG  AllocationType,
    IN ULONG  Protect
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG  ZeroBits,
    IN OUT PULONG  RegionSize,
    IN ULONG  AllocationType,
    IN ULONG  Protect
);

NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSTATUS
NTAPI
NtCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

NTSTATUS
NTAPI
NtCancelIoFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelIoFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSTATUS
NTAPI
NtCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);

NTSTATUS
NTAPI
NtClearEvent(
    IN HANDLE EventHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent(
    IN HANDLE EventHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateDebugObject(
    OUT PHANDLE DebugHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN KillProcessOnExit
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDebugObject(
    OUT PHANDLE DebugHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN KillProcessOnExit
);

NTSTATUS
NTAPI
NtCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
    IN HANDLE Handle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
    IN HANDLE Handle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSTATUS
NTAPI
NtCompleteConnectPort(
    HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCompleteConnectPort(
    HANDLE PortHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSTATUS
NTAPI
NtContinue(
    IN PCONTEXT Context,
    IN BOOLEAN TestAlert
);

NTSYSAPI
NTSTATUS
NTAPI
ZwContinue(
    IN PCONTEXT Context,
    IN BOOLEAN TestAlert
);

NTSTATUS
NTAPI
NtCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSTATUS
NTAPI
NtCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
);

NTSTATUS
NTAPI
NtCreateIoCompletion(
    OUT PHANDLE             IoCompletionHandle,
    IN  ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    IN  ULONG               NumberOfConcurrentThreads
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateIoCompletion(
   OUT PHANDLE             IoCompletionHandle,
   IN  ACCESS_MASK         DesiredAccess,
   IN  POBJECT_ATTRIBUTES  ObjectAttributes,
   IN  ULONG               NumberOfConcurrentThreads
   );

NTSTATUS
NTAPI
NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    IN PULONG Disposition OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    IN PULONG Disposition OPTIONAL
);

NTSTATUS
NTAPI
NtCreateMailslotFile(
    OUT PHANDLE MailSlotFileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG MaxMessageSize,
    IN PLARGE_INTEGER TimeOut
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateMailslotFile(
    OUT PHANDLE MailSlotFileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG MaxMessageSize,
    IN PLARGE_INTEGER TimeOut
);

NTSTATUS
NTAPI
NtCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

NTSTATUS
NTAPI
NtCreateNamedPipeFile(
    OUT PHANDLE NamedPipeFileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG WriteModeMessage,
    IN ULONG ReadModeMessage,
    IN ULONG NonBlocking,
    IN ULONG MaxInstances,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PLARGE_INTEGER DefaultTimeOut
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateNamedPipeFile(
    OUT PHANDLE NamedPipeFileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN ULONG WriteModeMessage,
    IN ULONG ReadModeMessage,
    IN ULONG NonBlocking,
    IN ULONG MaxInstances,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PLARGE_INTEGER DefaultTimeOut
);

NTSTATUS
NTAPI
NtCreatePagingFile(
    IN PUNICODE_STRING FileName,
    IN PLARGE_INTEGER InitialSize,
    IN PLARGE_INTEGER MaxiumSize,
    IN ULONG Reserved
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

NTSTATUS
NTAPI
NtCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectionInfoLength,
    ULONG MaxMessageLength,
    ULONG MaxPoolUsage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectionInfoLength,
    ULONG MaxMessageLength,
    ULONG MaxPoolUsage
);

NTSTATUS
NTAPI
NtCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ParentProcess,
    IN BOOLEAN InheritObjectTable,
    IN HANDLE SectionHandle OPTIONAL,
    IN HANDLE DebugPort OPTIONAL,
    IN HANDLE ExceptionPort OPTIONAL
);

NTSTATUS
NTAPI
NtCreateProfile(
    OUT PHANDLE ProfileHandle,
    IN HANDLE ProcessHandle,
    IN PVOID ImageBase,
    IN ULONG ImageSize,
    IN ULONG Granularity,
    OUT PVOID Buffer,
    IN ULONG ProfilingSize,
    IN KPROFILE_SOURCE Source,
    IN KAFFINITY ProcessorMask
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateProfile(
    OUT PHANDLE ProfileHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG ImageBase,
    IN ULONG ImageSize,
    IN ULONG Granularity,
    OUT PVOID Buffer,
    IN ULONG ProfilingSize,
    IN ULONG ClockSource,
    IN ULONG ProcessorMask
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

NTSTATUS
NTAPI
NtCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

NTSTATUS
NTAPI
NtCreateSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING Name
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING Name
);

NTSTATUS
NTAPI
NtCreateThread(
    OUT PHANDLE ThreadHandle,
    IN  ACCESS_MASK DesiredAccess,
    IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN  HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN  PCONTEXT ThreadContext,
    IN  PINITIAL_TEB UserStack,
    IN  BOOLEAN CreateSuspended
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle,
    OUT PCLIENT_ID ClientId,
    IN PCONTEXT ThreadContext,
    IN PINITIAL_TEB UserStack,
    IN BOOLEAN CreateSuspended
);

NTSTATUS
NTAPI
NtCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);
#endif

NTSTATUS
NTAPI
NtCreateToken(
    OUT PHANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TOKEN_TYPE TokenType,
    IN PLUID AuthenticationId,
    IN PLARGE_INTEGER ExpirationTime,
    IN PTOKEN_USER TokenUser,
    IN PTOKEN_GROUPS TokenGroups,
    IN PTOKEN_PRIVILEGES TokenPrivileges,
    IN PTOKEN_OWNER TokenOwner,
    IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
    IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
    IN PTOKEN_SOURCE TokenSource
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateToken(
    OUT PHANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TOKEN_TYPE TokenType,
    IN PLUID AuthenticationId,
    IN PLARGE_INTEGER ExpirationTime,
    IN PTOKEN_USER TokenUser,
    IN PTOKEN_GROUPS TokenGroups,
    IN PTOKEN_PRIVILEGES TokenPrivileges,
    IN PTOKEN_OWNER TokenOwner,
    IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
    IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
    IN PTOKEN_SOURCE TokenSource
);

NTSTATUS
NTAPI
NtCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDebugFilterState(
     ULONG ComponentId,
     ULONG Level
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDebugContinue(
    IN HANDLE DebugObject,
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDebugContinue(
    IN HANDLE DebugObject,
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
);

NTSTATUS
NTAPI
NtDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSTATUS
NTAPI
NtDeleteAtom(
    IN RTL_ATOM Atom
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteAtom(
    IN RTL_ATOM Atom
);

NTSTATUS
NTAPI
NtDeleteBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSTATUS
NTAPI
NtDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtDeleteKey(
    IN HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteKey(
    IN HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
NtDeleteObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSTATUS
NTAPI
NtDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    IN HANDLE DeviceHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
    IN PVOID UserApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferSize,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    IN HANDLE DeviceHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
    IN PVOID UserApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferSize,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferSize
);

NTSTATUS
NTAPI
NtDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSTATUS
NTAPI
NtDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    OUT PHANDLE TargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    OUT PHANDLE TargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
);

NTSTATUS
NTAPI
NtEnumerateBootEntries(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateBootEntries(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
NTAPI
NtEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtExtendSection(
    IN HANDLE SectionHandle,
    IN PLARGE_INTEGER NewMaximumSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwExtendSection(
    IN HANDLE SectionHandle,
    IN PLARGE_INTEGER NewMaximumSize
);

NTSTATUS
NTAPI
NtFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSTATUS
NTAPI
NtFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);
#endif

NTSTATUS
NTAPI
NtFlushInstructionCache(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytesToFlush
);

NTSTATUS
NTAPI
NtFlushKey(
    IN HANDLE KeyHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
    IN HANDLE KeyHandle
);

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
NtFlushWriteBuffer(VOID);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushWriteBuffer(VOID);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  *BaseAddress,
    IN PULONG  RegionSize,
    IN ULONG  FreeType
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  *BaseAddress,
    IN PULONG  RegionSize,
    IN ULONG  FreeType
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFsControlFile(
    IN HANDLE DeviceHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferSize,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile(
    IN HANDLE DeviceHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferSize,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferSize
);

NTSTATUS
NTAPI
NtGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

NTSYSAPI
NTSTATUS
NTAPI
ZwGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

NTSTATUS
NTAPI
NtGetPlugPlayEvent(
    IN ULONG Reserved1,
    IN ULONG Reserved2,
    OUT PPLUGPLAY_EVENT_BLOCK Buffer,
    IN ULONG BufferSize
);

ULONG
NTAPI
NtGetTickCount(
    VOID
);

NTSYSAPI
ULONG
NTAPI
ZwGetTickCount(
    VOID
);

NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSTATUS
NTAPI
NtImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    POWER_ACTION SystemAction,
    SYSTEM_POWER_STATE MinSystemState,
    ULONG Flags,
    BOOLEAN Asynchronous
);

NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction(
    POWER_ACTION SystemAction,
    SYSTEM_POWER_STATE MinSystemState,
    ULONG Flags,
    BOOLEAN Asynchronous
);

NTSTATUS
NTAPI
NtInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSYSAPI
NTSTATUS
NTAPI
ZwInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSTATUS
NTAPI
NtIsProcessInJob(
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwIsProcessInJob(
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL
);

NTSTATUS
NTAPI
NtListenPort(HANDLE PortHandle,
             PPORT_MESSAGE ConnectionRequest
);

NTSYSAPI
NTSTATUS
NTAPI
ZwListenPort(HANDLE PortHandle,
             PPORT_MESSAGE ConnectionRequest
);

NTSTATUS
NTAPI
NtLoadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSTATUS
NTAPI
NtLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSTATUS
NTAPI
NtLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockFile(
    IN  HANDLE FileHandle,
    IN  HANDLE Event OPTIONAL,
    IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN  PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER ByteOffset,
    IN  PLARGE_INTEGER Length,
    IN  ULONG Key,
    IN  BOOLEAN FailImmediatedly,
    IN  BOOLEAN ExclusiveLock
);


NTSYSAPI
NTSTATUS
NTAPI
ZwLockFile(
    IN  HANDLE FileHandle,
    IN  HANDLE Event OPTIONAL,
    IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN  PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER ByteOffset,
    IN  PLARGE_INTEGER Length,
    IN  ULONG Key,
    IN  BOOLEAN FailImmediatedly,
    IN  BOOLEAN ExclusiveLock
);

NTSTATUS
NTAPI
NtLockVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    ULONG NumberOfBytesToLock,
    PULONG NumberOfBytesLocked
);

NTSYSAPI
NTSTATUS
NTAPI
ZwLockVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    ULONG NumberOfBytesToLock,
    PULONG NumberOfBytesLocked
);

NTSTATUS
NTAPI
NtMakePermanentObject(
    IN HANDLE Object
);

NTSYSAPI
NTSTATUS
NTAPI
ZwMakePermanentObject(
    IN HANDLE Object
);

NTSTATUS
NTAPI
NtMakeTemporaryObject(
    IN HANDLE Handle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
    IN HANDLE Handle
);

NTSTATUS
NTAPI
NtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PULONG ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG AccessProtection
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
    IN OUT PULONG ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG AccessProtection
);

NTSTATUS
NTAPI
NtNotifyChangeDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree
);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree
);

NTSTATUS
NTAPI
NtNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN Asynchroneous,
    OUT PVOID ChangeBuffer,
    IN ULONG Length,
    IN BOOLEAN WatchSubtree
);

NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN Asynchroneous,
    OUT PVOID ChangeBuffer,
    IN ULONG Length,
    IN BOOLEAN WatchSubtree
);

NTSTATUS
NTAPI
NtOpenDirectoryObject(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
);

NTSTATUS
NTAPI
NtOpenIoCompletion(
    OUT PHANDLE CompetionPort,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenIoCompletion(
    OUT PHANDLE CompetionPort,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN ULONG GrantedAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN ObjectCreation,
    IN BOOLEAN AccessGranted,
    OUT PBOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN ULONG GrantedAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN ObjectCreation,
    IN BOOLEAN AccessGranted,
    OUT PBOOLEAN GenerateOnClose
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSTATUS
NTAPI
NtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
NTAPI
NtOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSTATUS
NTAPI
NtOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);
#endif

NTSTATUS
NTAPI
NtPlugPlayControl(
    IN PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPowerInformation(
    POWER_INFORMATION_LEVEL PowerInformationLevel,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    POWER_INFORMATION_LEVEL PowerInformationLevel,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeCheck(
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET RequiredPrivileges,
    IN PBOOLEAN Result
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeCheck(
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET RequiredPrivileges,
    IN PBOOLEAN Result
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegedServiceAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSTATUS
NTAPI
NtProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN ULONG *NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection
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

NTSTATUS
NTAPI
NtPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSTATUS
NTAPI
NtQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
);


NTSTATUS
NTAPI
NtQueryBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
NTAPI
NtQueryBootOptions(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryBootOptions(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
NTAPI
NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
);

NTSTATUS
NTAPI
NtQueryDefaultUILanguage(
    PLANGID LanguageId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultUILanguage(
    PLANGID LanguageId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan
);

NTSTATUS
NTAPI
NtQueryDirectoryObject(
    IN     HANDLE DirectoryHandle,
    OUT    PVOID Buffer,
    IN     ULONG BufferLength,
    IN     BOOLEAN ReturnSingleEntry,
    IN     BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT    PULONG ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    IN     HANDLE DirectoryHandle,
    OUT    PVOID Buffer,
    IN     ULONG BufferLength,
    IN     BOOLEAN ReturnSingleEntry,
    IN     BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT    PULONG ReturnLength OPTIONAL
);

NTSTATUS
NTAPI
NtQueryEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PULONG EaIndex OPTIONAL,
    IN BOOLEAN RestartScan
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEaFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID EaList OPTIONAL,
    IN ULONG EaListLength,
    IN PULONG EaIndex OPTIONAL,
    IN BOOLEAN RestartScan
);
#endif

NTSTATUS
NTAPI
NtQueryEvent(
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEvent(
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
);

NTSTATUS
NTAPI
NtQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
NTAPI
NtQueryInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQueryInformationPort(
    HANDLE PortHandle,
    PORT_INFORMATION_CLASS PortInformationClass,
    PVOID PortInformation,
    ULONG PortInformationLength,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationPort(
    HANDLE PortHandle,
    PORT_INFORMATION_CLASS PortInformationClass,
    PVOID PortInformation,
    ULONG PortInformationLength,
    PULONG ReturnLength
);

#ifndef _NTDDK_
NTSTATUS
NTAPI
NtQueryInformationProcess(
    IN HANDLE  ProcessHandle,
    IN PROCESSINFOCLASS  ProcessInformationClass,
    OUT PVOID  ProcessInformation,
    IN ULONG  ProcessInformationLength,
    OUT PULONG  ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
    IN HANDLE  ProcessHandle,
    IN PROCESSINFOCLASS  ProcessInformationClass,
    OUT PVOID  ProcessInformation,
    IN ULONG  ProcessInformationLength,
    OUT PULONG  ReturnLength OPTIONAL
);
#endif

NTSTATUS
NTAPI
NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQueryInstallUILanguage(
    PLANGID LanguageId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInstallUILanguage(
    PLANGID LanguageId
);

NTSTATUS
NTAPI
NtQueryIntervalProfile(
    IN  KPROFILE_SOURCE ProfileSource,
    OUT PULONG Interval
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryIntervalProfile(
    OUT PULONG Interval,
    OUT KPROFILE_SOURCE ClockSource
);

NTSTATUS
NTAPI
NtQueryIoCompletion(
    IN  HANDLE IoCompletionHandle,
    IN  IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
    OUT PVOID IoCompletionInformation,
    IN  ULONG IoCompletionInformationLength,
    OUT PULONG ResultLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryIoCompletion(
    IN  HANDLE IoCompletionHandle,
    IN  IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
    OUT PVOID IoCompletionInformation,
    IN  ULONG IoCompletionInformationLength,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
NTAPI
NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID SidList OPTIONAL,
    IN ULONG SidListLength,
    IN PSID StartSid OPTIONAL,
    IN BOOLEAN RestartScan
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryQuotaInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN PVOID SidList OPTIONAL,
    IN ULONG SidListLength,
    IN PSID StartSid OPTIONAL,
    IN BOOLEAN RestartScan
);

NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN OUT PKEY_VALUE_ENTRY ValueList,
    IN ULONG NumberOfValues,
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN OUT PKEY_VALUE_ENTRY ValueList,
    IN ULONG NumberOfValues,
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQueryMutant(
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryMutant(
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ResultLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
NTAPI
NtQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSTATUS
NTAPI
NtQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtQuerySemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN  ULONG Length,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN  ULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQuerySymbolicLinkObject(
    IN HANDLE SymLinkObjHandle,
    OUT PUNICODE_STRING LinkTarget,
    OUT PULONG DataWritten OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySymbolicLinkObject(
    IN HANDLE SymLinkObjHandle,
    OUT PUNICODE_STRING LinkName,
    OUT PULONG DataWritten OPTIONAL
);

NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PWSTR Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PVOID Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
NtQuerySystemInformation(
    IN  SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN  ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    IN  SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN  ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSTATUS
NTAPI
NtQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSTATUS
NTAPI
NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
NTAPI
NtQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS
NTAPI
NtQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSTATUS
NTAPI
NtRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSTATUS
NTAPI
NtRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
); 

NTSYSAPI
NTSTATUS
NTAPI
ZwRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN PUNICODE_STRING UnicodeStringParameterMask OPTIONAL,
    IN PVOID *Parameters,
    IN HARDERROR_RESPONSE_OPTION ResponseOption,
    OUT PHARDERROR_RESPONSE Response
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
    IN PVOID UserApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
    IN PVOID UserApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG BufferLength,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
);

NTSTATUS
NTAPI
NtReadFileScatter(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
    IN  PVOID UserApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK UserIoStatusBlock,
    IN FILE_SEGMENT_ELEMENT BufferDescription[],
    IN ULONG BufferLength,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadFileScatter(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE UserApcRoutine OPTIONAL,
    IN  PVOID UserApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK UserIoStatusBlock,
    IN FILE_SEGMENT_ELEMENT BufferDescription[],
    IN ULONG BufferLength,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key OPTIONAL
);

NTSTATUS
NTAPI
NtReadRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
NtReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN ULONG  NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN ULONG  NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead
);

NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSTATUS
NTAPI
NtReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSTATUS
NTAPI
NtReleaseSemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseSemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSTATUS
NTAPI
NtRemoveIoCompletion(
    IN  HANDLE IoCompletionHandle,
    OUT PVOID *CompletionKey,
    OUT PVOID *CompletionContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER Timeout OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRemoveIoCompletion(
    IN  HANDLE IoCompletionHandle,
    OUT PVOID *CompletionKey,
    OUT PVOID *CompletionContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS
NTAPI
NtReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSTATUS
NTAPI
NtReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply
);

NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    HANDLE PortHandle,
    PVOID *PortContext OPTIONAL,
    PPORT_MESSAGE ReplyMessage OPTIONAL,
    PPORT_MESSAGE ReceiveMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReceivePort(
    HANDLE PortHandle,
    PVOID *PortContext OPTIONAL,
    PPORT_MESSAGE ReplyMessage,
    PPORT_MESSAGE ReceiveMessage
);

NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ReplyMessage
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplyWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ReplyMessage
);

NTSTATUS
NTAPI
NtRequestPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcMessage);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply,
    PPORT_MESSAGE LpcRequest
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRequestWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply,
    PPORT_MESSAGE LpcRequest
);

NTSTATUS
NTAPI
NtResetEvent(
    HANDLE EventHandle,
    PLONG NumberOfWaitingThreads OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent(
    HANDLE EventHandle,
    PLONG NumberOfWaitingThreads OPTIONAL
);

NTSTATUS
NTAPI
NtRestoreKey(
    HANDLE KeyHandle,
    HANDLE FileHandle,
    ULONG RestoreFlags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey(
    HANDLE KeyHandle,
    HANDLE FileHandle,
    ULONG RestoreFlags
);

NTSTATUS
NTAPI
NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
NTAPI
NtResumeProcess(
    IN HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResumeProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
NTAPI
NtSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSTATUS
NTAPI
NtSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);


NTSYSCALLAPI
NTSTATUS
NTAPI
NtSecureConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PSID Sid OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSecureConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW ClientView OPTIONAL,
    PSID Sid OPTIONAL,
    PREMOTE_PORT_VIEW ServerView OPTIONAL,
    PULONG MaxMessageLength OPTIONAL,
    PVOID ConnectionInformation OPTIONAL,
    PULONG ConnectionInformationLength OPTIONAL
);

NTSTATUS
NTAPI
NtSetBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
NTAPI
NtSetBootOptions(
    ULONG Unknown1,
    ULONG Unknown2
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetBootOptions(
    ULONG Unknown1,
    ULONG Unknown2
);

NTSTATUS
NTAPI
NtSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);

NTSYSAPI
NTSTATUS
NTAPI
NtSetDebugFilterState(
    ULONG ComponentId,
    ULONG Level,
    BOOLEAN State
);

NTSTATUS
NTAPI
NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
);

NTSTATUS
NTAPI
NtSetDefaultUILanguage(
    LANGID LanguageId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage(
    LANGID LanguageId
);

NTSTATUS
NTAPI
NtSetDefaultHardErrorPort(
    IN HANDLE PortHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultHardErrorPort(
    IN HANDLE PortHandle
);

NTSTATUS
NTAPI
NtSetEaFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    PVOID EaBuffer,
    ULONG EaBufferSize
);

#ifdef NTOS_MODE_USER
NTSYSAPI
NTSTATUS
NTAPI
ZwSetEaFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    PVOID EaBuffer,
    ULONG EaBufferSize
);
#endif

NTSTATUS
NTAPI
NtSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSTATUS
NTAPI
NtSetHighEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighEventPair(
    IN HANDLE EventPairHandle
);
NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
NTAPI
NtSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

NTSTATUS
NTAPI
NtSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

NTSTATUS
NTAPI
NtSetInformationObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG Length
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG Length
);

NTSTATUS
NTAPI
NtSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength
);

NTSTATUS
NTAPI
NtSetIoCompletion(
    IN HANDLE IoCompletionPortHandle,
    IN PVOID CompletionKey,
    IN PVOID CompletionContext,
    IN NTSTATUS CompletionStatus,
    IN ULONG CompletionInformation
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIoCompletion(
    IN HANDLE IoCompletionPortHandle,
    IN PVOID CompletionKey,
    IN PVOID CompletionContext,
    IN NTSTATUS CompletionStatus,
    IN ULONG CompletionInformation
);

NTSTATUS
NTAPI
NtSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSTATUS
NTAPI
NtSetLdtEntries(
    ULONG Selector1,
    LDT_ENTRY LdtEntry1,
    ULONG Selector2,
    LDT_ENTRY LdtEntry2
);

NTSTATUS
NTAPI
NtSetLowEventPair(
    HANDLE EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowEventPair(
    HANDLE EventPair
);

NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetQuotaInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer,
    ULONG BufferLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetQuotaInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID Buffer,
    ULONG BufferLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);

NTSTATUS
NTAPI
NtSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemPowerState(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags
);

NTSTATUS
NTAPI
NtSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSTATUS
NTAPI
NtSetTimer(
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine,
    IN PVOID TimerContext,
    IN BOOLEAN WakeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL
);

NTSTATUS
NTAPI
NtSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSTATUS
NTAPI
NtSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSTATUS
NTAPI
NtSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS
NTAPI
NtShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSYSAPI
NTSTATUS
NTAPI
ZwShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSTATUS
NTAPI
NtSignalAndWaitForSingleObject(
    IN HANDLE SignalObject,
    IN HANDLE WaitObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSignalAndWaitForSingleObject(
    IN HANDLE SignalObject,
    IN HANDLE WaitObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
NTAPI
NtStartProfile(
    HANDLE ProfileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStartProfile(
    HANDLE ProfileHandle
);

NTSTATUS
NTAPI
NtStopProfile(
    HANDLE ProfileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwStopProfile(
    HANDLE ProfileHandle
);

NTSTATUS
NTAPI
NtSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
NTAPI
NtSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSTATUS
NTAPI
NtSystemDebugControl(
    DEBUG_CONTROL_CODE ControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
NtTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
NtTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
NtTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

NTSTATUS
NTAPI
NtTestAlert(
    VOID
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTestAlert(
    VOID
);

NTSTATUS
NTAPI
NtTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN struct _EVENT_TRACE_HEADER* TraceHeader
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN struct _EVENT_TRACE_HEADER* TraceHeader
);

NTSTATUS
NTAPI
NtTranslateFilePath(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

NTSYSAPI
NTSTATUS
NTAPI
ZwTranslateFilePath(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

NTSTATUS
NTAPI
NtUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSTATUS
NTAPI
NtUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER ByteOffset,
    IN PLARGE_INTEGER Lenght,
    OUT ULONG Key OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER ByteOffset,
    IN PLARGE_INTEGER Lenght,
    OUT ULONG Key OPTIONAL
);

NTSTATUS
NTAPI
NtUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG  NumberOfBytesToUnlock,
    OUT PULONG NumberOfBytesUnlocked OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG  NumberOfBytesToUnlock,
    OUT PULONG NumberOfBytesUnlocked OPTIONAL
);

NTSTATUS
NTAPI
NtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
);

NTSTATUS
NTAPI
NtVdmControl(
    ULONG ControlCode,
    PVOID ControlData
);

NTSTATUS
NTAPI
NtW32Call(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID* Result OPTIONAL,
    OUT PULONG ResultLength OPTIONAL
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForDebugEvent(
    IN HANDLE DebugObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForDebugEvent(
    IN HANDLE DebugObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange
);

NTSTATUS
NTAPI
NtWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Object[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Object[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
NTAPI
NtWaitForSingleObject(
    IN HANDLE Object,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
    IN HANDLE Object,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
NTAPI
NtWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
NTAPI
NtWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key OPTIONAL
);

NTSTATUS
NTAPI
NtWriteFileGather(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN FILE_SEGMENT_ELEMENT BufferDescription[],
    IN ULONG BufferLength,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFileGather(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN FILE_SEGMENT_ELEMENT BufferDescription[],
    IN ULONG BufferLength,
    IN PLARGE_INTEGER ByteOffset,
    IN PULONG Key OPTIONAL
);

NTSTATUS
NTAPI
NtWriteRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWriteRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSTATUS
NTAPI
NtWriteVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  BaseAddress,
    IN PVOID Buffer,
    IN ULONG NumberOfBytesToWrite,
    OUT PULONG NumberOfBytesWritten
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

NTSTATUS
NTAPI
NtYieldExecution(
    VOID
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution(
    VOID
);

#endif
