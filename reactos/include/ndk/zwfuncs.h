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
#include "lpctypes.h"
#include "zwtypes.h"
#include "kdtypes.h"
#define _WMIKM_
#include <evntrace.h>

/* FUNCTION TYPES ************************************************************/

/* PROTOTYPES ****************************************************************/

NTSTATUS
STDCALL
NtAcceptConnectPort(
    PHANDLE PortHandle,
    HANDLE NamedPortHandle,
    PPORT_MESSAGE ServerReply,
    BOOLEAN AcceptIt,
    PPORT_VIEW WriteMap,
    PREMOTE_PORT_VIEW ReadMap
);

NTSTATUS
STDCALL
NtAccessCheck(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAcces,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT PULONG ReturnLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSTATUS
STDCALL
ZwAccessCheck(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAcces,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT PULONG ReturnLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSTATUS
STDCALL
NtAccessCheckAndAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PHANDLE ObjectHandle,
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
STDCALL
NtAddAtom(
    IN     PWSTR  AtomName,
    IN     ULONG AtomNameLength,
    IN OUT PRTL_ATOM  Atom
);

NTSTATUS
STDCALL
ZwAddAtom(
    IN     PWSTR  AtomName,
    IN     ULONG AtomNameLength,
    IN OUT PRTL_ATOM  Atom
);

NTSTATUS
STDCALL
NtAddBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSTATUS
STDCALL
ZwAddBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSTATUS
STDCALL
NtAdjustGroupsToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN  ResetToDefault,
    IN PTOKEN_GROUPS  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_GROUPS  PreviousState OPTIONAL,
    OUT PULONG  ReturnLength
);

NTSTATUS
STDCALL
ZwAdjustGroupsToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN  ResetToDefault,
    IN PTOKEN_GROUPS  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_GROUPS  PreviousState,
    OUT PULONG  ReturnLength
);

NTSTATUS
STDCALL
NtAdjustPrivilegesToken(
    IN HANDLE  TokenHandle,
    IN BOOLEAN  DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_PRIVILEGES  PreviousState,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwAdjustPrivilegesToken(
    IN HANDLE  TokenHandle,
    IN BOOLEAN  DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES  NewState,
    IN ULONG  BufferLength,
    OUT PTOKEN_PRIVILEGES  PreviousState,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
NtAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
STDCALL
ZwAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
STDCALL
NtAlertThread(
    IN HANDLE ThreadHandle
);

NTSTATUS
STDCALL
ZwAlertThread(
    IN HANDLE ThreadHandle
);

NTSTATUS
STDCALL
NtAllocateLocallyUniqueId(
    OUT LUID *LocallyUniqueId
);

NTSTATUS
STDCALL
ZwAllocateLocallyUniqueId(
    OUT PLUID Luid
);

NTSTATUS
STDCALL
NtAllocateUuids(
    PULARGE_INTEGER Time,
    PULONG Range,
    PULONG Sequence,
    PUCHAR Seed
);

NTSTATUS
STDCALL
ZwAllocateUuids(
    PULARGE_INTEGER Time,
    PULONG Range,
    PULONG Sequence,
    PUCHAR Seed
);

NTSTATUS
STDCALL
NtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG  ZeroBits,
    IN OUT PULONG  RegionSize,
    IN ULONG  AllocationType,
    IN ULONG  Protect
);

NTSTATUS
STDCALL
ZwAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG  ZeroBits,
    IN OUT PULONG  RegionSize,
    IN ULONG  AllocationType,
    IN ULONG  Protect
);

NTSTATUS
STDCALL
NtAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSTATUS
STDCALL
ZwAssignProcessToJobObject(
    HANDLE JobHandle,
    HANDLE ProcessHandle
);

NTSTATUS
STDCALL
NtCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

NTSTATUS
STDCALL
ZwCallbackReturn(
    PVOID Result,
    ULONG ResultLength,
    NTSTATUS Status
);

NTSTATUS
STDCALL
NtCancelIoFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSTATUS
STDCALL
ZwCancelIoFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSTATUS
STDCALL
NtCancelTimer(
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
);

NTSTATUS
STDCALL
NtClearEvent(
    IN HANDLE EventHandle
);

NTSTATUS
STDCALL
ZwClearEvent(
    IN HANDLE EventHandle
);

NTSTATUS
STDCALL
NtCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwCreateJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtClose(
    IN HANDLE Handle
);

NTSTATUS
STDCALL
ZwClose(
    IN HANDLE Handle
);

NTSTATUS
STDCALL
NtCloseObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSTATUS
STDCALL
ZwCloseObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSTATUS
STDCALL
NtCompleteConnectPort(
    HANDLE PortHandle
);

NTSTATUS
STDCALL
ZwCompleteConnectPort(
    HANDLE PortHandle
);

NTSTATUS
STDCALL
NtConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW SectionInfo,
    PREMOTE_PORT_VIEW MapInfo,
    PULONG MaxMessageSize,
    PVOID ConnectInfo,
    PULONG ConnectInfoLength
);

NTSTATUS
STDCALL
ZwConnectPort(
    PHANDLE PortHandle,
    PUNICODE_STRING PortName,
    PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    PPORT_VIEW SectionInfo,
    PREMOTE_PORT_VIEW MapInfo,
    PULONG MaxMessageSize,
    PVOID ConnectInfo,
    PULONG ConnectInfoLength
);

NTSTATUS
STDCALL
NtContinue(
    IN PCONTEXT Context,
    IN BOOLEAN TestAlert
);

NTSTATUS
STDCALL
ZwContinue(
    IN PCONTEXT Context,
    IN CINT IrqLevel
);

NTSTATUS
STDCALL
NtCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSTATUS
STDCALL
ZwCreateEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
);

NTSTATUS
STDCALL
NtCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwCreateEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtCreateIoCompletion(
    OUT PHANDLE             IoCompletionHandle,
    IN  ACCESS_MASK         DesiredAccess,
    IN  POBJECT_ATTRIBUTES  ObjectAttributes,
    IN  ULONG               NumberOfConcurrentThreads
    );

NTSTATUS
STDCALL
ZwCreateIoCompletion(
   OUT PHANDLE             IoCompletionHandle,
   IN  ACCESS_MASK         DesiredAccess,
   IN  POBJECT_ATTRIBUTES  ObjectAttributes,
   IN  ULONG               NumberOfConcurrentThreads
   );

NTSTATUS
STDCALL
NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    IN PULONG Disposition OPTIONAL
);

NTSTATUS
STDCALL
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
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

NTSTATUS
STDCALL
ZwCreateMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN InitialOwner
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtCreatePagingFile(
    IN PUNICODE_STRING FileName,
    IN PLARGE_INTEGER InitialSize,
    IN PLARGE_INTEGER MaxiumSize,
    IN ULONG Reserved
);

NTSTATUS
STDCALL
ZwCreatePagingFile(
    IN PUNICODE_STRING FileName,
    IN PLARGE_INTEGER InitialSize,
    IN PLARGE_INTEGER MaxiumSize,
    IN ULONG Reserved
);

NTSTATUS
STDCALL
NtCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSTATUS
STDCALL
NtCreatePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
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

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
NtCreateSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection OPTIONAL,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
);

NTSTATUS
STDCALL
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
STDCALL
NtCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

NTSTATUS
STDCALL
ZwCreateSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
);

NTSTATUS
STDCALL
NtCreateSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING Name
);

NTSTATUS
STDCALL
ZwCreateSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING Name
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);

NTSTATUS
STDCALL
ZwCreateTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSTATUS
STDCALL
ZwCreateWaitablePort(
    PHANDLE PortHandle,
    POBJECT_ATTRIBUTES ObjectAttributes,
    ULONG MaxConnectInfoLength,
    ULONG MaxDataLength,
    ULONG NPMessageQueueSize OPTIONAL
);

NTSTATUS
STDCALL
NtDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSTATUS
STDCALL
ZwDelayExecution(
    IN BOOLEAN Alertable,
    IN LARGE_INTEGER *Interval
);

NTSTATUS
STDCALL
NtDeleteAtom(
    IN RTL_ATOM Atom
);

NTSTATUS
STDCALL
ZwDeleteAtom(
    IN RTL_ATOM Atom
);

NTSTATUS
STDCALL
NtDeleteBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSTATUS
STDCALL
ZwDeleteBootEntry(
    IN PUNICODE_STRING EntryName,
    IN PUNICODE_STRING EntryValue
);

NTSTATUS
STDCALL
NtDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtDeleteKey(
    IN HANDLE KeyHandle
);

NTSTATUS
STDCALL
ZwDeleteKey(
    IN HANDLE KeyHandle
);

NTSTATUS
STDCALL
NtDeleteObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSTATUS
STDCALL
ZwDeleteObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN BOOLEAN GenerateOnClose
);

NTSTATUS
STDCALL
NtDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

NTSTATUS
STDCALL
ZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSTATUS
STDCALL
ZwDisplayString(
    IN PUNICODE_STRING DisplayString
);

NTSTATUS
STDCALL
NtDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    OUT PHANDLE TargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
);

NTSTATUS
STDCALL
ZwDuplicateObject(
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle,
    OUT PHANDLE TargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
);

NTSTATUS
STDCALL
NtDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
);

NTSTATUS
STDCALL
NtEnumerateBootEntries(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
STDCALL
ZwEnumerateBootEntries(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
STDCALL
NtEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtExtendSection(
    IN HANDLE SectionHandle,
    IN PLARGE_INTEGER NewMaximumSize
);

NTSTATUS
STDCALL
ZwExtendSection(
    IN HANDLE SectionHandle,
    IN PLARGE_INTEGER NewMaximumSize
);

NTSTATUS
STDCALL
NtFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSTATUS
STDCALL
ZwFindAtom(
    IN  PWSTR AtomName,
    IN  ULONG AtomNameLength,
    OUT PRTL_ATOM Atom OPTIONAL
);

NTSTATUS
STDCALL
NtFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSTATUS
STDCALL
ZwFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

NTSTATUS
STDCALL
NtFlushInstructionCache(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN UINT NumberOfBytesToFlush
);

NTSTATUS
STDCALL
NtFlushKey(
    IN HANDLE KeyHandle
);

NTSTATUS
STDCALL
ZwFlushKey(
    IN HANDLE KeyHandle
);

NTSTATUS
STDCALL
NtFlushVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytesToFlush,
    OUT PULONG NumberOfBytesFlushed OPTIONAL
);

NTSTATUS
STDCALL
NtFlushWriteBuffer(VOID);

NTSTATUS
STDCALL
ZwFlushWriteBuffer(VOID);

NTSTATUS
STDCALL
NtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  *BaseAddress,
    IN PULONG  RegionSize,
    IN ULONG  FreeType
);

NTSTATUS
STDCALL
ZwFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  *BaseAddress,
    IN PULONG  RegionSize,
    IN ULONG  FreeType
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

NTSTATUS
STDCALL
ZwGetContextThread(
    IN HANDLE ThreadHandle,
    OUT PCONTEXT Context
);

NTSTATUS
STDCALL
NtGetPlugPlayEvent(
    IN ULONG Reserved1,
    IN ULONG Reserved2,
    OUT PPLUGPLAY_EVENT_BLOCK Buffer,
    IN ULONG BufferSize
);

ULONG
STDCALL
NtGetTickCount(
    VOID
);

ULONG
STDCALL
ZwGetTickCount(
    VOID
);

NTSTATUS
STDCALL
NtImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSTATUS
STDCALL
ZwImpersonateClientOfPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ClientMessage
);

NTSTATUS
STDCALL
NtImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSTATUS
STDCALL
ZwImpersonateThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ThreadToImpersonate,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
);

NTSTATUS
STDCALL
NtInitiatePowerAction(
    POWER_ACTION SystemAction,
    SYSTEM_POWER_STATE MinSystemState,
    ULONG Flags,
    BOOLEAN Asynchronous
);

NTSTATUS
STDCALL
ZwInitiatePowerAction(
    POWER_ACTION SystemAction,
    SYSTEM_POWER_STATE MinSystemState,
    ULONG Flags,
    BOOLEAN Asynchronous
);

NTSTATUS
STDCALL
NtInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSTATUS
STDCALL
ZwInitializeRegistry(
    BOOLEAN SetUpBoot
);

NTSTATUS
STDCALL
NtIsProcessInJob(
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL
);

NTSTATUS
STDCALL
ZwIsProcessInJob(
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL
);

NTSTATUS
STDCALL
NtListenPort(HANDLE PortHandle,
             PPORT_MESSAGE LpcMessage
);

NTSTATUS
STDCALL
ZwListenPort(HANDLE PortHandle,
             PPORT_MESSAGE LpcMessage
);

NTSTATUS
STDCALL
NtLoadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSTATUS
STDCALL
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSTATUS
STDCALL
NtLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSTATUS
STDCALL
ZwLoadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSTATUS
STDCALL
NtLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

NTSTATUS
STDCALL
ZwLoadKey2(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes,
    IN ULONG Flags
);

NTSTATUS
STDCALL
NtLockFile(
    IN  HANDLE FileHandle,
    IN  HANDLE Event OPTIONAL,
    IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN  PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER ByteOffset,
    IN  PLARGE_INTEGER Length,
    IN  PULONG Key,
    IN  BOOLEAN FailImmediatedly,
    IN  BOOLEAN ExclusiveLock
);

NTSTATUS
STDCALL
ZwLockFile(
    IN  HANDLE FileHandle,
    IN  HANDLE Event OPTIONAL,
    IN  PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN  PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER ByteOffset,
    IN  PLARGE_INTEGER Length,
    IN  PULONG Key,
    IN  BOOLEAN FailImmediatedly,
    IN  BOOLEAN ExclusiveLock
);

NTSTATUS
STDCALL
NtLockVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    ULONG NumberOfBytesToLock,
    PULONG NumberOfBytesLocked
);

NTSTATUS
STDCALL
ZwLockVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    ULONG NumberOfBytesToLock,
    PULONG NumberOfBytesLocked
);

NTSTATUS
STDCALL
NtMakePermanentObject(
    IN HANDLE Object
);

NTSTATUS
STDCALL
ZwMakePermanentObject(
    IN HANDLE Object
);

NTSTATUS
STDCALL
NtMakeTemporaryObject(
    IN HANDLE Handle
);

NTSTATUS
STDCALL
ZwMakeTemporaryObject(
    IN HANDLE Handle
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtOpenDirectoryObject(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenDirectoryObject(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenEvent(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
);

NTSTATUS
STDCALL
ZwOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
);

NTSTATUS
STDCALL
NtOpenIoCompletion(
    OUT PHANDLE CompetionPort,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenIoCompletion(
    OUT PHANDLE CompetionPort,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenJobObject(
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenMutant(
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
NtOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSTATUS
STDCALL
ZwOpenProcess(
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSTATUS
STDCALL
NtOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
ZwOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
NtOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
ZwOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
NtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenSemaphore(
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAcces,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
ZwOpenSymbolicLinkObject(
    OUT PHANDLE SymbolicLinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSTATUS
STDCALL
ZwOpenThread(
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId
);

NTSTATUS
STDCALL
NtOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
ZwOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
NtOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
ZwOpenThreadTokenEx(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSTATUS
STDCALL
NtOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);
NTSTATUS
STDCALL
ZwOpenTimer(
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

NTSTATUS
STDCALL
NtPlugPlayControl(
    IN PLUGPLAY_CONTROL_CLASS PlugPlayControlClass,
    IN OUT PVOID Buffer,
    IN ULONG BufferSize
);

NTSTATUS
STDCALL
NtPowerInformation(
    POWER_INFORMATION_LEVEL PowerInformationLevel,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);

NTSTATUS
STDCALL
ZwPowerInformation(
    POWER_INFORMATION_LEVEL PowerInformationLevel,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength
);

NTSTATUS
STDCALL
NtPrivilegeCheck(
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET RequiredPrivileges,
    IN PBOOLEAN Result
);

NTSTATUS
STDCALL
ZwPrivilegeCheck(
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET RequiredPrivileges,
    IN PBOOLEAN Result
);

NTSTATUS
STDCALL
NtPrivilegedServiceAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSTATUS
STDCALL
ZwPrivilegedServiceAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSTATUS
STDCALL
NtPrivilegeObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSTATUS
STDCALL
ZwPrivilegeObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSTATUS
STDCALL
NtProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN ULONG *NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection
);

NTSTATUS
STDCALL
ZwProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID *BaseAddress,
    IN ULONG *NumberOfBytesToProtect,
    IN ULONG NewAccessProtection,
    OUT PULONG OldAccessProtection
);

NTSTATUS
STDCALL
NtPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSTATUS
STDCALL
ZwPulseEvent(
    IN HANDLE EventHandle,
    IN PLONG PulseCount OPTIONAL
);

NTSTATUS
STDCALL
NtQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
);

NTSTATUS
STDCALL
ZwQueryAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_BASIC_INFORMATION FileInformation
);


NTSTATUS
STDCALL
NtQueryBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
STDCALL
ZwQueryBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
STDCALL
NtQueryBootOptions(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
STDCALL
ZwQueryBootOptions(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);
NTSTATUS
STDCALL
NtQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
);

NTSTATUS
STDCALL
ZwQueryDefaultLocale(
    IN BOOLEAN UserProfile,
    OUT PLCID DefaultLocaleId
);

NTSTATUS
STDCALL
NtQueryDefaultUILanguage(
    PLANGID LanguageId
);

NTSTATUS
STDCALL
ZwQueryDefaultUILanguage(
    PLANGID LanguageId
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtQueryDirectoryObject(
    IN     HANDLE DirectoryHandle,
    OUT    PVOID Buffer,
    IN     ULONG BufferLength,
    IN     BOOLEAN ReturnSingleEntry,
    IN     BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT    PULONG ReturnLength OPTIONAL
);

NTSTATUS
STDCALL
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
STDCALL
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

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
NtQueryEvent(
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);
NTSTATUS
STDCALL
ZwQueryEvent(
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
NtQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
);

NTSTATUS
STDCALL
ZwQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
);

NTSTATUS
STDCALL
NtQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

NTSTATUS
STDCALL
ZwQueryInformationAtom(
    IN  RTL_ATOM Atom,
    IN  ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN  ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

NTSTATUS
STDCALL
NtQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
STDCALL
ZwQueryInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
STDCALL
NtQueryInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwQueryInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
NtQueryInformationPort(
    HANDLE PortHandle,
    CINT PortInformationClass,
    PVOID PortInformation,
    ULONG PortInformationLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwQueryInformationPort(
    HANDLE PortHandle,
    CINT PortInformationClass,
    PVOID PortInformation,
    ULONG PortInformationLength,
    PULONG ReturnLength
);

#ifndef _NTDDK_
NTSTATUS
STDCALL
NtQueryInformationProcess(
    IN HANDLE  ProcessHandle,
    IN PROCESSINFOCLASS  ProcessInformationClass,
    OUT PVOID  ProcessInformation,
    IN ULONG  ProcessInformationLength,
    OUT PULONG  ReturnLength OPTIONAL
);

NTSTATUS
STDCALL
ZwQueryInformationProcess(
    IN HANDLE  ProcessHandle,
    IN PROCESSINFOCLASS  ProcessInformationClass,
    OUT PVOID  ProcessInformation,
    IN ULONG  ProcessInformationLength,
    OUT PULONG  ReturnLength OPTIONAL
);
#endif

NTSTATUS
STDCALL
NtQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwQueryInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
NtQueryInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwQueryInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
NtQueryInstallUILanguage(
    PLANGID LanguageId
);

NTSTATUS
STDCALL
ZwQueryInstallUILanguage(
    PLANGID LanguageId
);

NTSTATUS
STDCALL
NtQueryIntervalProfile(
    IN  KPROFILE_SOURCE ProfileSource,
    OUT PULONG Interval
);

NTSTATUS
STDCALL
ZwQueryIntervalProfile(
    OUT PULONG Interval,
    OUT KPROFILE_SOURCE ClockSource
);

NTSTATUS
STDCALL
NtQueryIoCompletion(
    IN  HANDLE IoCompletionHandle,
    IN  IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
    OUT PVOID IoCompletionInformation,
    IN  ULONG IoCompletionInformationLength,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
STDCALL
ZwQueryIoCompletion(
    IN  HANDLE IoCompletionHandle,
    IN  IO_COMPLETION_INFORMATION_CLASS IoCompletionInformationClass,
    OUT PVOID IoCompletionInformation,
    IN  ULONG IoCompletionInformationLength,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
STDCALL
NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN OUT PKEY_VALUE_ENTRY ValueList,
    IN ULONG NumberOfValues,
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN OUT PKEY_VALUE_ENTRY ValueList,
    IN ULONG NumberOfValues,
    OUT PVOID Buffer,
    IN OUT PULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
NtQueryMutant(
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQueryMutant(
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
STDCALL
ZwQueryObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
STDCALL
NtQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSTATUS
STDCALL
ZwQueryPerformanceCounter(
    IN PLARGE_INTEGER Counter,
    IN PLARGE_INTEGER Frequency
);

NTSTATUS
STDCALL
NtQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQuerySecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtQuerySemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN  ULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwQuerySemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN  ULONG Length,
    OUT PULONG ReturnLength
);

NTSTATUS
STDCALL
NtQuerySymbolicLinkObject(
    IN HANDLE SymLinkObjHandle,
    OUT PUNICODE_STRING LinkTarget,
    OUT PULONG DataWritten OPTIONAL
);

NTSTATUS
STDCALL
ZwQuerySymbolicLinkObject(
    IN HANDLE SymLinkObjHandle,
    OUT PUNICODE_STRING LinkName,
    OUT PULONG DataWritten OPTIONAL
);

NTSTATUS
STDCALL
NtQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PWSTR Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwQuerySystemEnvironmentValue(
    IN PUNICODE_STRING Name,
    OUT PVOID Value,
    ULONG Length,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
NtQuerySystemInformation(
    IN  SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN  ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQuerySystemInformation(
    IN  SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN  ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSTATUS
STDCALL
ZwQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
);

NTSTATUS
STDCALL
NtQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQueryTimer(
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSTATUS
STDCALL
ZwQueryTimerResolution(
    OUT PULONG MinimumResolution,
    OUT PULONG MaximumResolution,
    OUT PULONG ActualResolution
);

NTSTATUS
STDCALL
NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN IN CINT VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
ZwQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID Address,
    IN IN CINT VirtualMemoryInformationClass,
    OUT PVOID VirtualMemoryInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
);

NTSTATUS
STDCALL
NtQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS
STDCALL
ZwQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS
STDCALL
NtQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSTATUS
STDCALL
ZwQueueApcThread(
    HANDLE ThreadHandle,
    PKNORMAL_ROUTINE ApcRoutine,
    PVOID NormalContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

NTSTATUS
STDCALL
NtRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSTATUS
STDCALL
ZwRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN BOOLEAN SearchFrames
);

NTSTATUS
STDCALL
NtRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN PUNICODE_STRING UnicodeStringParameterMask OPTIONAL,
    IN PVOID *Parameters,
    IN HARDERROR_RESPONSE_OPTION ResponseOption,
    OUT PHARDERROR_RESPONSE Response
);

NTSTATUS
STDCALL
ZwRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN PUNICODE_STRING UnicodeStringParameterMask OPTIONAL,
    IN PVOID *Parameters,
    IN HARDERROR_RESPONSE_OPTION ResponseOption,
    OUT PHARDERROR_RESPONSE Response
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtReadRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwReadRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
NtReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN ULONG  NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead
);
NTSTATUS
STDCALL
ZwReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN ULONG  NumberOfBytesToRead,
    OUT PULONG NumberOfBytesRead
);

NTSTATUS
STDCALL
NtRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSTATUS
STDCALL
ZwRegisterThreadTerminatePort(
    HANDLE TerminationPort
);

NTSTATUS
STDCALL
NtReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSTATUS
STDCALL
ZwReleaseMutant(
    IN HANDLE MutantHandle,
    IN PLONG ReleaseCount OPTIONAL
);

NTSTATUS
STDCALL
NtReleaseSemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSTATUS
STDCALL
ZwReleaseSemaphore(
    IN  HANDLE SemaphoreHandle,
    IN  LONG ReleaseCount,
    OUT PLONG PreviousCount
);

NTSTATUS
STDCALL
NtRemoveIoCompletion(
    IN  HANDLE IoCompletionHandle,
    OUT PVOID *CompletionKey,
    OUT PVOID *CompletionContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS
STDCALL
ZwRemoveIoCompletion(
    IN  HANDLE IoCompletionHandle,
    OUT PVOID *CompletionKey,
    OUT PVOID *CompletionContext,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PLARGE_INTEGER Timeout OPTIONAL
);

NTSTATUS
STDCALL
NtReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSTATUS
STDCALL
ZwReplaceKey(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN HANDLE Key,
    IN POBJECT_ATTRIBUTES ReplacedObjectAttributes
);

NTSTATUS
STDCALL
NtReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply
);

NTSTATUS
STDCALL
ZwReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply
);

NTSTATUS
STDCALL
NtReplyWaitReceivePort(
    HANDLE PortHandle,
    PULONG PortId,
    PPORT_MESSAGE MessageReply,
    PPORT_MESSAGE MessageRequest
);

NTSTATUS
STDCALL
ZwReplyWaitReceivePort(
    HANDLE PortHandle,
    PULONG PortId,
    PPORT_MESSAGE MessageReply,
    PPORT_MESSAGE MessageRequest
);

NTSTATUS
STDCALL
NtReplyWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ReplyMessage
);

NTSTATUS
STDCALL
ZwReplyWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE ReplyMessage
);

NTSTATUS
STDCALL
NtRequestPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcMessage);

NTSTATUS
STDCALL
ZwRequestPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcMessage
);

NTSTATUS
STDCALL
NtRequestWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply,
    PPORT_MESSAGE LpcRequest
);

NTSTATUS
STDCALL
ZwRequestWaitReplyPort(
    HANDLE PortHandle,
    PPORT_MESSAGE LpcReply,
    PPORT_MESSAGE LpcRequest
);

NTSTATUS
STDCALL
NtResetEvent(
    HANDLE EventHandle,
    PLONG NumberOfWaitingThreads OPTIONAL
);

NTSTATUS
STDCALL
ZwResetEvent(
    HANDLE EventHandle,
    PLONG NumberOfWaitingThreads OPTIONAL
);

NTSTATUS
STDCALL
NtRestoreKey(
    HANDLE KeyHandle,
    HANDLE FileHandle,
    ULONG RestoreFlags
);

NTSTATUS
STDCALL
ZwRestoreKey(
    HANDLE KeyHandle,
    HANDLE FileHandle,
    ULONG RestoreFlags
);

NTSTATUS
STDCALL
NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
STDCALL
ZwResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG SuspendCount
);

NTSTATUS
STDCALL
NtResumeProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
STDCALL
ZwResumeProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
STDCALL
NtSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);
NTSTATUS
STDCALL
ZwSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSTATUS
STDCALL
NtSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);

NTSTATUS
STDCALL
ZwSaveKeyEx(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
);

NTSTATUS
STDCALL
NtSetBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
STDCALL
ZwSetBootEntryOrder(
    IN ULONG Unknown1,
    IN ULONG Unknown2
);

NTSTATUS
STDCALL
NtSetBootOptions(
    ULONG Unknown1,
    ULONG Unknown2
);

NTSTATUS
STDCALL
ZwSetBootOptions(
    ULONG Unknown1,
    ULONG Unknown2
);

NTSTATUS
STDCALL
NtSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);
NTSTATUS
STDCALL
ZwSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT Context
);

NTSTATUS
STDCALL
NtSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
);

NTSTATUS
STDCALL
ZwSetDefaultLocale(
    IN BOOLEAN UserProfile,
    IN LCID DefaultLocaleId
);

NTSTATUS
STDCALL
NtSetDefaultUILanguage(
    LANGID LanguageId
);

NTSTATUS
STDCALL
ZwSetDefaultUILanguage(
    LANGID LanguageId
);
NTSTATUS
STDCALL
NtSetDefaultHardErrorPort(
    IN HANDLE PortHandle
);
NTSTATUS
STDCALL
ZwSetDefaultHardErrorPort(
    IN HANDLE PortHandle
);

NTSTATUS
STDCALL
NtSetEaFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    PVOID EaBuffer,
    ULONG EaBufferSize
);

NTSTATUS
STDCALL
ZwSetEaFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    PVOID EaBuffer,
    ULONG EaBufferSize
);

NTSTATUS
STDCALL
NtSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSTATUS
STDCALL
ZwSetEvent(
    IN HANDLE EventHandle,
    OUT PLONG PreviousState  OPTIONAL
);

NTSTATUS
STDCALL
NtSetHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
STDCALL
ZwSetHighEventPair(
    IN HANDLE EventPairHandle
);
NTSTATUS
STDCALL
NtSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);
NTSTATUS
STDCALL
ZwSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
STDCALL
NtSetInformationFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
STDCALL
ZwSetInformationFile(
    IN HANDLE FileHandle,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
STDCALL
NtSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

NTSTATUS
STDCALL
ZwSetInformationJobObject(
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength
);

NTSTATUS
STDCALL
NtSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

NTSTATUS
STDCALL
ZwSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG KeyInformationLength
);

NTSTATUS
STDCALL
NtSetInformationObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG Length
);

NTSTATUS
STDCALL
ZwSetInformationObject(
    IN HANDLE ObjectHandle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG Length
);

NTSTATUS
STDCALL
NtSetInformationProcess(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength
);

NTSTATUS
STDCALL
NtSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);
NTSTATUS
STDCALL
ZwSetInformationThread(
    IN HANDLE ThreadHandle,
    IN THREADINFOCLASS ThreadInformationClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength
);

NTSTATUS
STDCALL
NtSetInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength
);

NTSTATUS
STDCALL
ZwSetInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength
);

NTSTATUS
STDCALL
NtSetIoCompletion(
    IN HANDLE IoCompletionPortHandle,
    IN PVOID CompletionKey,
    IN PVOID CompletionContext,
    IN NTSTATUS CompletionStatus,
    IN ULONG CompletionInformation
);

NTSTATUS
STDCALL
ZwSetIoCompletion(
    IN HANDLE IoCompletionPortHandle,
    IN PVOID CompletionKey,
    IN PVOID CompletionContext,
    IN NTSTATUS CompletionStatus,
    IN ULONG CompletionInformation
);

NTSTATUS
STDCALL
NtSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSTATUS
STDCALL
ZwSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ClockSource
);

NTSTATUS
STDCALL
NtSetLdtEntries(
    ULONG Selector1,
    LDT_ENTRY LdtEntry1,
    ULONG Selector2,
    LDT_ENTRY LdtEntry2
);

NTSTATUS
STDCALL
NtSetLowEventPair(
    HANDLE EventPair
);

NTSTATUS
STDCALL
ZwSetLowEventPair(
    HANDLE EventPair
);

NTSTATUS
STDCALL
NtSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSTATUS
STDCALL
ZwSetLowWaitHighEventPair(
    HANDLE EventPair
);

NTSTATUS
STDCALL
NtSetQuotaInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PFILE_QUOTA_INFORMATION Buffer,
    ULONG BufferLength
);

NTSTATUS
STDCALL
ZwSetQuotaInformationFile(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PFILE_QUOTA_INFORMATION Buffer,
    ULONG BufferLength
);

NTSTATUS
STDCALL
NtSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTSTATUS
STDCALL
ZwSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTSTATUS
STDCALL
NtSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);
NTSTATUS
STDCALL
ZwSetSystemEnvironmentValue(
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING Value
);

NTSTATUS
STDCALL
NtSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
);

NTSTATUS
STDCALL
ZwSetSystemInformation(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    IN PVOID SystemInformation,
    IN ULONG SystemInformationLength
);

NTSTATUS
STDCALL
NtSetSystemPowerState(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags
);

NTSTATUS
STDCALL
NtSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSTATUS
STDCALL
ZwSetSystemTime(
    IN PLARGE_INTEGER SystemTime,
    IN PLARGE_INTEGER NewSystemTime OPTIONAL
);

NTSTATUS
STDCALL
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
STDCALL
NtSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSTATUS
STDCALL
ZwSetTimerResolution(
    IN ULONG RequestedResolution,
    IN BOOLEAN SetOrUnset,
    OUT PULONG ActualResolution
);

NTSTATUS
STDCALL
NtSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSTATUS
STDCALL
ZwSetUuidSeed(
    IN PUCHAR UuidSeed
);

NTSTATUS
STDCALL
NtSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
);

NTSTATUS
STDCALL
ZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
);

NTSTATUS
STDCALL
NtSetVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS
STDCALL
ZwSetVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS
STDCALL
NtShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSTATUS
STDCALL
ZwShutdownSystem(
    IN SHUTDOWN_ACTION Action
);

NTSTATUS
STDCALL
NtSignalAndWaitForSingleObject(
    IN HANDLE SignalObject,
    IN HANDLE WaitObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
STDCALL
ZwSignalAndWaitForSingleObject(
    IN HANDLE SignalObject,
    IN HANDLE WaitObject,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
STDCALL
NtStartProfile(
    HANDLE ProfileHandle
);

NTSTATUS
STDCALL
ZwStartProfile(
    HANDLE ProfileHandle
);

NTSTATUS
STDCALL
NtStopProfile(
    HANDLE ProfileHandle
);

NTSTATUS
STDCALL
ZwStopProfile(
    HANDLE ProfileHandle
);

NTSTATUS
STDCALL
NtSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
STDCALL
ZwSuspendProcess(
    IN HANDLE ProcessHandle
);

NTSTATUS
STDCALL
NtSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSTATUS
STDCALL
ZwSuspendThread(
    IN HANDLE ThreadHandle,
    IN PULONG PreviousSuspendCount
);

NTSTATUS
STDCALL
NtSystemDebugControl(
    DEBUG_CONTROL_CODE ControlCode,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
NtTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
STDCALL
ZwTerminateProcess(
    IN HANDLE ProcessHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
STDCALL
NtTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
STDCALL
ZwTerminateThread(
    IN HANDLE ThreadHandle,
    IN NTSTATUS ExitStatus
);

NTSTATUS
STDCALL
NtTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

NTSTATUS
STDCALL
ZwTerminateJobObject(
    HANDLE JobHandle,
    NTSTATUS ExitStatus
);

NTSTATUS
STDCALL
NtTestAlert(
    VOID
);

NTSTATUS
STDCALL
ZwTestAlert(
    VOID
);

NTSTATUS
STDCALL
NtTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN struct _EVENT_TRACE_HEADER* TraceHeader
);

NTSTATUS
STDCALL
ZwTraceEvent(
    IN ULONG TraceHandle,
    IN ULONG Flags,
    IN ULONG TraceHeaderLength,
    IN struct _EVENT_TRACE_HEADER* TraceHeader
);

NTSTATUS
STDCALL
NtTranslateFilePath(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

NTSTATUS
STDCALL
ZwTranslateFilePath(
    ULONG Unknown1,
    ULONG Unknown2,
    ULONG Unknown3
);

NTSTATUS
STDCALL
NtUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSTATUS
STDCALL
ZwUnloadDriver(
    IN PUNICODE_STRING DriverServiceName
);

NTSTATUS
STDCALL
NtUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSTATUS
STDCALL
ZwUnloadKey(
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSTATUS
STDCALL
NtUnlockFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER ByteOffset,
    IN PLARGE_INTEGER Lenght,
    OUT PULONG Key OPTIONAL
);

NTSTATUS
STDCALL
ZwUnlockFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER ByteOffset,
    IN PLARGE_INTEGER Lenght,
    OUT PULONG Key OPTIONAL
);

NTSTATUS
STDCALL
NtUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG  NumberOfBytesToUnlock,
    OUT PULONG NumberOfBytesUnlocked OPTIONAL
);

NTSTATUS
STDCALL
ZwUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG  NumberOfBytesToUnlock,
    OUT PULONG NumberOfBytesUnlocked OPTIONAL
);

NTSTATUS
STDCALL
NtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
);

NTSTATUS
STDCALL
ZwUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
);

NTSTATUS
STDCALL
NtVdmControl(
    ULONG ControlCode,
    PVOID ControlData
);

NTSTATUS
STDCALL
NtW32Call(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID* Result OPTIONAL,
    OUT PULONG ResultLength OPTIONAL
);

NTSTATUS
STDCALL
NtWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Object[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
STDCALL
ZwWaitForMultipleObjects(
    IN ULONG Count,
    IN HANDLE Object[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
STDCALL
NtWaitForSingleObject(
    IN HANDLE Object,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
STDCALL
ZwWaitForSingleObject(
    IN HANDLE Object,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Time
);

NTSTATUS
STDCALL
NtWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
STDCALL
ZwWaitHighEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
STDCALL
NtWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
STDCALL
ZwWaitLowEventPair(
    IN HANDLE EventPairHandle
);

NTSTATUS
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
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

NTSTATUS
STDCALL
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
STDCALL
NtWriteRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
ZwWriteRequestData(
    HANDLE PortHandle,
    PPORT_MESSAGE Message,
    ULONG Index,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG ReturnLength
);

NTSTATUS
STDCALL
NtWriteVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  BaseAddress,
    IN PVOID Buffer,
    IN ULONG NumberOfBytesToWrite,
    OUT PULONG NumberOfBytesWritten
);

NTSTATUS
STDCALL
ZwWriteVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID  BaseAddress,
    IN PVOID Buffer,
    IN ULONG NumberOfBytesToWrite,
    OUT PULONG NumberOfBytesWritten
);

NTSTATUS
STDCALL
NtYieldExecution(
    VOID
);

NTSTATUS
STDCALL
ZwYieldExecution(
    VOID
);


static __inline struct _PEB* NtCurrentPeb (void) 
{
    struct _PEB * pPeb;

#if defined(__GNUC__)

    __asm__ __volatile__
    (
      "movl %%fs:0x30, %0\n" /* fs:30h == Teb->Peb */
      : "=r" (pPeb) /* can't have two memory operands */
      : /* no inputs */
    );

#elif defined(_MSC_VER)

    __asm mov eax, fs:0x30;
    __asm mov pPeb, eax

#else
#error Unknown compiler for inline assembler
#endif

    return pPeb;
}
#endif
