/* $Id: rtl.h,v 1.26 2001/05/24 11:27:10 ekohl Exp $
 *
 */

#ifndef __INCLUDE_NTDLL_RTL_H
#define __INCLUDE_NTDLL_RTL_H

#include <napi/teb.h>
#include <ddk/ntddk.h>

typedef struct _CRITICAL_SECTION_DEBUG {
    WORD   Type;
    WORD   CreatorBackTraceIndex;
    struct _CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Depth;
    PVOID OwnerBackTrace[ 5 ];
} CRITICAL_SECTION_DEBUG, *PCRITICAL_SECTION_DEBUG;

typedef struct _CRITICAL_SECTION {
    PCRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD Reserved;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

typedef struct _RTL_PROCESS_INFO
{
   ULONG Size;
   HANDLE ProcessHandle;
   HANDLE ThreadHandle;
   CLIENT_ID ClientId;
   SECTION_IMAGE_INFORMATION ImageInfo;
} RTL_PROCESS_INFO, *PRTL_PROCESS_INFO;

typedef struct _RTL_RESOURCE
{
   CRITICAL_SECTION Lock;
   HANDLE SharedSemaphore;
   ULONG SharedWaiters;
   HANDLE ExclusiveSemaphore;
   ULONG ExclusiveWaiters;
   LONG NumberActive;
   HANDLE OwningThread;
   ULONG TimeoutBoost; /* ?? */
   PVOID DebugInfo; /* ?? */
} RTL_RESOURCE, *PRTL_RESOURCE;


#define HEAP_BASE (0xa0000000)

VOID
STDCALL
RtlDeleteCriticalSection (
	PCRITICAL_SECTION	CriticalSection
	);

VOID
STDCALL
RtlEnterCriticalSection (
	PCRITICAL_SECTION	CriticalSection
	);

NTSTATUS
STDCALL
RtlInitializeCriticalSection (
	PCRITICAL_SECTION	CriticalSection
	);

VOID
STDCALL
RtlLeaveCriticalSection (
	PCRITICAL_SECTION	CriticalSection
	);

BOOLEAN
STDCALL
RtlTryEnterCriticalSection (
	PCRITICAL_SECTION	CriticalSection
	);

DWORD
STDCALL
RtlCompactHeap (
	HANDLE	heap,
	DWORD	flags
	);

BOOLEAN
STDCALL
RtlEqualComputerName (
	IN	PUNICODE_STRING	ComputerName1,
	IN	PUNICODE_STRING	ComputerName2
	);

BOOLEAN
STDCALL
RtlEqualDomainName (
	IN	PUNICODE_STRING	DomainName1,
	IN	PUNICODE_STRING	DomainName2
	);

VOID
STDCALL
RtlEraseUnicodeString (
	IN	PUNICODE_STRING	String
	);

NTSTATUS
STDCALL
RtlLargeIntegerToChar (
	IN	PLARGE_INTEGER	Value,
	IN	ULONG		Base,
	IN	ULONG		Length,
	IN OUT	PCHAR		String
	);


/* Path functions */

ULONG
STDCALL
RtlDetermineDosPathNameType_U (
	PWSTR Path
	);

BOOLEAN
STDCALL
RtlDoesFileExists_U (
	PWSTR FileName
	);

BOOLEAN
STDCALL
RtlDosPathNameToNtPathName_U (
	PWSTR		dosname,
	PUNICODE_STRING	ntname,
	PWSTR		*shortname,
	PCURDIR		nah
	);

ULONG
STDCALL
RtlDosSearchPath_U (
	WCHAR *sp,
	WCHAR *name,
	WCHAR *ext,
	ULONG buf_sz,
	WCHAR *buffer,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetCurrentDirectory_U (
	ULONG MaximumLength,
	PWSTR Buffer
	);

ULONG
STDCALL
RtlGetFullPathName_U (
	WCHAR *dosname,
	ULONG size,
	WCHAR *buf,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetLongestNtPathLength (
	VOID
	);

ULONG STDCALL RtlGetNtGlobalFlags(VOID);

BOOLEAN STDCALL RtlGetNtProductType(PNT_PRODUCT_TYPE ProductType);

ULONG
STDCALL
RtlGetProcessHeaps (
	ULONG	HeapCount,
	HANDLE	*HeapArray
	);

ULONG
STDCALL
RtlIsDosDeviceName_U (
	PWSTR DeviceName
	);

NTSTATUS
STDCALL
RtlSetCurrentDirectory_U (
	PUNICODE_STRING name
	);

/* Environment functions */
VOID
STDCALL
RtlAcquirePebLock (
	VOID
	);

VOID
STDCALL
RtlReleasePebLock (
	VOID
	);

NTSTATUS
STDCALL
RtlCreateEnvironment (
	BOOLEAN	Inherit,
	PWSTR	*Environment
	);

VOID
STDCALL
RtlDestroyEnvironment (
	PWSTR	Environment
	);

NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U (
	PWSTR		Environment,
	PUNICODE_STRING	Source,
	PUNICODE_STRING	Destination,
	PULONG		Length
	);

NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PWSTR		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

VOID
STDCALL
RtlSetCurrentEnvironment (
	PWSTR	NewEnvironment,
	PWSTR	*OldEnvironment
	);

NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PWSTR		*Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

NTSTATUS
STDCALL
RtlCreateUserThread (
	IN	HANDLE			ProcessHandle,
	IN	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	IN	BOOLEAN			CreateSuspended,
	IN	LONG			StackZeroBits,
	IN OUT	PULONG			StackReserve,
	IN OUT	PULONG			StackCommit,
	IN	PTHREAD_START_ROUTINE	StartAddress,
	IN	PVOID			Parameter,
	IN OUT	PHANDLE			ThreadHandle,
	IN OUT	PCLIENT_ID		ClientId
	);

NTSTATUS
STDCALL
RtlFreeUserThreadStack (
	IN	HANDLE	ProcessHandle,
	IN	HANDLE	ThreadHandle
	);

NTSTATUS
STDCALL
RtlCreateUserProcess (
	IN	PUNICODE_STRING			ImageFileName,
	IN	ULONG				Attributes,
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	IN	PSECURITY_DESCRIPTOR		ProcessSecutityDescriptor OPTIONAL,
	IN	PSECURITY_DESCRIPTOR		ThreadSecurityDescriptor OPTIONAL,
	IN	HANDLE				ParentProcess OPTIONAL,
	IN	BOOLEAN				CurrentDirectory,
	IN	HANDLE				DebugPort OPTIONAL,
	IN	HANDLE				ExceptionPort OPTIONAL,
	OUT	PRTL_PROCESS_INFO		ProcessInfo
	);

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	OUT	PRTL_USER_PROCESS_PARAMETERS	*ProcessParameters,
	IN	PUNICODE_STRING	ImagePathName OPTIONAL,
	IN	PUNICODE_STRING	DllPath OPTIONAL,
	IN	PUNICODE_STRING	CurrentDirectory OPTIONAL,
	IN	PUNICODE_STRING	CommandLine OPTIONAL,
	IN	PWSTR		Environment OPTIONAL,
	IN	PUNICODE_STRING	WindowTitle OPTIONAL,
	IN	PUNICODE_STRING	DesktopInfo OPTIONAL,
	IN	PUNICODE_STRING	ShellInfo OPTIONAL,
	IN	PUNICODE_STRING	RuntimeInfo OPTIONAL
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams (
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

VOID
STDCALL
RtlDestroyProcessParameters (
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams (
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

NTSTATUS
STDCALL
RtlLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime,
	PLARGE_INTEGER	SystemTime
	);

NTSTATUS
STDCALL
RtlSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	);

VOID
STDCALL
RtlRaiseStatus (
	IN	NTSTATUS	Status
	);


/* resource functions */

BOOLEAN
STDCALL
RtlAcquireResourceExclusive (
	IN	PRTL_RESOURCE	Resource,
	IN	BOOLEAN		Wait
	);

BOOLEAN
STDCALL
RtlAcquireResourceShared (
	IN	PRTL_RESOURCE	Resource,
	IN	BOOLEAN		Wait
	);

VOID
STDCALL
RtlConvertExclusiveToShared (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlConvertSharedToExclusive (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlDeleteResource (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlDumpResource (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlInitializeResource (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlReleaseResource (
	IN	PRTL_RESOURCE	Resource
	);

#endif /* __INCLUDE_NTDLL_RTL_H */

/* EOF */
