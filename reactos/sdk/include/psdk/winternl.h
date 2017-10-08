/*
 * winternl.h
 *
 * Windows NT internal data structures and functions
 *
 * Note: This header exists only for compatibility with the native SDK.
 * It's definitions are incomplete and potentially unsuitable.
 * ReactOS modules should not make use of it!
 *
 * USE NDK INSTEAD!
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#pragma once

#ifdef __REACTOS__
#error "Do not use this header, use NDK!"
#endif

#ifndef _WINTERNL_
#define _WINTERNL_

//#include <winapifamily.h>
#include <windef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef NT_INFORMATION
#define NT_INFORMATION(Status) ((((ULONG)(Status)) >> 30) == 1)
#endif

#ifndef NT_WARNING
#define NT_WARNING(Status) ((((ULONG)(Status)) >> 30) == 2)
#endif

#ifndef NT_ERROR
#define NT_ERROR(Status) ((((ULONG)(Status)) >> 30) == 3)
#endif

typedef CONST char *PCSZ;

typedef struct _STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef PSTRING PCANSI_STRING; // yes, thats the definition from MS!
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef const STRING *PCOEM_STRING;

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB_LDR_DATA
{
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    PVOID Reserved1[2];
    LIST_ENTRY InMemoryOrderLinks;
    PVOID Reserved2[2];
    PVOID DllBase;
    PVOID Reserved3[2];
    UNICODE_STRING FullDllName;
    BYTE Reserved4[8];
    PVOID Reserved5[3];
    union {
        ULONG CheckSum;
        PVOID Reserved6;
    } DUMMYUNIONNAME;
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef
VOID
(NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(
    VOID);

typedef struct _PEB
{
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID Reserved4[3];
    PVOID AtlThunkSListPtr;
    PVOID Reserved5;
    ULONG Reserved6;
    PVOID Reserved7;
    ULONG Reserved8;
    ULONG AtlThunkSListPtr32;
    PVOID Reserved9[45];
    BYTE Reserved10[96];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved11[128];
    PVOID Reserved12[1];
    ULONG SessionId;
} PEB, *PPEB;

typedef struct _TEB
{
    PVOID Reserved1[12];
    PPEB ProcessEnvironmentBlock;
    PVOID Reserved2[399];
    BYTE Reserved3[1952];
    PVOID TlsSlots[64];
    BYTE Reserved4[8];
    PVOID Reserved5[26];
    PVOID ReservedForOle;
    PVOID Reserved6[4];
    PVOID TlsExpansionSlots;
} TEB, *PTEB;

typedef enum _FILE_INFORMATION_CLASS
{
    FileDirectoryInformation = 1
} FILE_INFORMATION_CLASS;

#define INTERNAL_TS_ACTIVE_CONSOLE_ID   (*((volatile ULONG*)0x7ffe02d8))
#define LOGONID_CURRENT     ((ULONG)-1)
#define SERVERNAME_CURRENT ((HANDLE)NULL)

/* Flags for NtCreateFile and NtOpenFile */
#define FILE_DIRECTORY_FILE              0x00000001
#define FILE_WRITE_THROUGH               0x00000002
#define FILE_SEQUENTIAL_ONLY             0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING   0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT        0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT     0x00000020
#define FILE_NON_DIRECTORY_FILE          0x00000040
#define FILE_CREATE_TREE_CONNECTION      0x00000080
#define FILE_COMPLETE_IF_OPLOCKED        0x00000100
#define FILE_NO_EA_KNOWLEDGE             0x00000200
#define FILE_OPEN_REMOTE_INSTANCE        0x00000400
#define FILE_RANDOM_ACCESS               0x00000800
#define FILE_DELETE_ON_CLOSE             0x00001000
#define FILE_OPEN_BY_FILE_ID             0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT      0x00004000
#define FILE_NO_COMPRESSION              0x00008000
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
#define FILE_OPEN_REQUIRING_OPLOCK       0x00010000
#endif
#define FILE_RESERVE_OPFILTER            0x00100000
#define FILE_OPEN_REPARSE_POINT          0x00200000
#define FILE_OPEN_NO_RECALL              0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY   0x00800000

/* Status for NtCreateFile or NtOpenFile */
#define FILE_SUPERSEDED                  0x00000000
#define FILE_OPENED                      0x00000001
#define FILE_CREATED                     0x00000002
#define FILE_OVERWRITTEN                 0x00000003
#define FILE_EXISTS                      0x00000004
#define FILE_DOES_NOT_EXIST              0x00000005

#define FILE_VALID_OPTION_FLAGS          0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS     0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS 0x00000032
#define FILE_VALID_SET_FLAGS             0x00000036

/* Disposition for NtCreateFile */
#define FILE_SUPERSEDE                   0x00000000
#define FILE_OPEN                        0x00000001
#define FILE_CREATE                      0x00000002
#define FILE_OPEN_IF                     0x00000003
#define FILE_OVERWRITE                   0x00000004
#define FILE_OVERWRITE_IF                0x00000005
#define FILE_MAXIMUM_DISPOSITION         0x00000005

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

#define OBJ_INHERIT            0x00000002L
#define OBJ_PERMANENT          0x00000010L
#define OBJ_EXCLUSIVE          0x00000020L
#define OBJ_CASE_INSENSITIVE   0x00000040L
#define OBJ_OPENIF             0x00000080L
#define OBJ_OPENLINK           0x00000100L
#define OBJ_KERNEL_HANDLE      0x00000200L
#define OBJ_FORCE_ACCESS_CHECK 0x00000400L
#define OBJ_VALID_ATTRIBUTES   0x000007F2L

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes(p, n, a, r, s) \
{                                                 \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);      \
    (p)->RootDirectory = r;                       \
    (p)->Attributes = a;                          \
    (p)->ObjectName = n;                          \
    (p)->SecurityDescriptor = s;                  \
    (p)->SecurityQualityOfService = NULL;         \
}
#endif

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } DUMMYUNIONNAME;

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
    _In_ HANDLE Handle);

typedef
VOID
(NTAPI *PIO_APC_ROUTINE)(
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    _In_ HANDLE Object,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRenameKey(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING NewName);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtNotifyChangeMultipleKeys(
    _In_ HANDLE MasterKeyHandle,
    _In_opt_ ULONG Count,
    _In_reads_opt_(Count) OBJECT_ATTRIBUTES SubordinateObjects[],
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG CompletionFilter,
    _In_ BOOLEAN WatchTree,
    _Out_writes_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize,
    _In_ BOOLEAN Asynchronous);

typedef struct _KEY_VALUE_ENTRY
{
    PUNICODE_STRING ValueName;
    ULONG DataLength;
    ULONG DataOffset;
    ULONG Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMultipleValueKey(
    _In_ HANDLE KeyHandle,
    _Inout_updates_(EntryCount) PKEY_VALUE_ENTRY ValueEntries,
    _In_ ULONG EntryCount,
    _Out_writes_bytes_(*BufferLength) PVOID ValueBuffer,
    _Inout_ PULONG BufferLength,
    _Out_opt_ PULONG RequiredBufferLength);

typedef enum _KEY_SET_INFORMATION_CLASS
{
    KeyWriteTimeInformation,
    KeyWow64FlagsInformation,
    KeyControlFlagsInformation,
    KeySetVirtualizationInformation,
    KeySetDebugInformation,
    KeySetHandleTagsInformation,
    MaxKeySetInfoClass
} KEY_SET_INFORMATION_CLASS;

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationKey(
    _In_ HANDLE KeyHandle,
    _In_ _Strict_type_match_
        KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    _In_reads_bytes_(KeySetInformationLength) PVOID KeySetInformation,
    _In_ ULONG KeySetInformationLength);

typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation = 0,
    ProcessDebugPort = 7,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessBreakOnTermination = 29
} PROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION
{
    PVOID Reserved1;
    PPEB PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength);

typedef enum _THREADINFOCLASS
{
    ThreadIsIoPending = 16
} THREADINFOCLASS;

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_ PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength);

typedef enum _OBJECT_INFORMATION_CLASS
{
    ObjectBasicInformation = 0,
    ObjectTypeInformation = 2
} OBJECT_INFORMATION_CLASS;

typedef struct _PUBLIC_OBJECT_BASIC_INFORMATION
{
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG Reserved[10];
} PUBLIC_OBJECT_BASIC_INFORMATION, *PPUBLIC_OBJECT_BASIC_INFORMATION;

typedef struct __PUBLIC_OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING TypeName;
    ULONG Reserved [22];
} PUBLIC_OBJECT_TYPE_INFORMATION, *PPUBLIC_OBJECT_TYPE_INFORMATION;

_IRQL_requires_max_(PASSIVE_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryObject(
    _In_opt_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength);

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation = 0,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5,
    SystemProcessorPerformanceInformation = 8,
    SystemInterruptInformation = 23,
    SystemExceptionInformation = 33,
    SystemRegistryQuotaInformation = 37,
    SystemLookasideInformation = 45,
    SystemPolicyInformation = 134,
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION
{
    BYTE Reserved1[24];
    PVOID Reserved2[4];
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
    BYTE Reserved1[312];
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_TIMEOFDAY_INFORMATION
{
    BYTE Reserved1[48];
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    BYTE Reserved1[52];
    PVOID Reserved2[3];
    HANDLE UniqueProcessId;
    PVOID Reserved3;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_INTERRUPT_INFORMATION
{
    BYTE Reserved1[24];
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

typedef struct _SYSTEM_EXCEPTION_INFORMATION
{
    BYTE Reserved1[16];
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION
{
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    PVOID Reserved1;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION
{
    BYTE Reserved1[32];
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_POLICY_INFORMATION
{
    PVOID Reserved1[2];
    ULONG Reserved2[3];
} SYSTEM_POLICY_INFORMATION, *PSYSTEM_POLICY_INFORMATION;

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_ PVOID SystemInformation,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ResultLength);

typedef enum _WINSTATIONINFOCLASS
{
    WinStationInformation = 8
} WINSTATIONINFOCLASS;

typedef struct _WINSTATIONINFORMATIONW
{
    BYTE Reserved2[70];
    ULONG LogonId;
    BYTE Reserved3[1140];
} WINSTATIONINFORMATIONW, * PWINSTATIONINFORMATIONW;

typedef
BOOLEAN
(WINAPI * PWINSTATIONQUERYINFORMATIONW)(
    HANDLE,
    ULONG,
    WINSTATIONINFOCLASS,
    PVOID,
    ULONG,
    PULONG);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemTime(
    _Out_ PLARGE_INTEGER SystemTime);

#define RtlMoveMemory(Dest,Source,Length) memmove((Dest),(Source),(Length))
#define RtlFillMemory(Dest,Length,Fill) memset((Dest),(Fill),(Length))
#define RtlZeroMemory(Dest,Length) RtlFillMemory((Dest),(Length),0)

VOID
NTAPI
RtlInitString(
    PSTRING DestinationString,
    PCSZ SourceString);

VOID
NTAPI
RtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSZ SourceString);

VOID
NTAPI
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString);

VOID
NTAPI
RtlFreeAnsiString(
    PANSI_STRING AnsiString);

VOID
NTAPI
RtlFreeOemString(
    POEM_STRING OemString);

VOID
NTAPI
RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString);

NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString);

NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString);

NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
    POEM_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString);

NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
    _Out_ PULONG BytesInMultiByteString,
    _In_reads_bytes_(BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString);

NTSTATUS
NTAPI
RtlCharToInteger(
    PCSZ String,
    ULONG Base,
    PULONG Value);

BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(
    _In_ PUNICODE_STRING Name,
    _Inout_opt_ POEM_STRING OemName,
    _Inout_opt_ PBOOLEAN NameContainsSpaces);

NTSTATUS
NTAPI
RtlLocalTimeToSystemTime(
    IN PLARGE_INTEGER LocalTime,
    _Out_ PLARGE_INTEGER SystemTime);

BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
    PLARGE_INTEGER Time,
    PULONG ElapsedSeconds);

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
ULONG
NTAPI
RtlNtStatusToDosError(
    _In_ NTSTATUS Status);

NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString);

ULONG
NTAPI
RtlUniform(
    PULONG Seed);


#ifdef __cplusplus
}
#endif

#endif /* _WINTERNL_ */
