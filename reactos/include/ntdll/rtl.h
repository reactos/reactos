/* $Id: rtl.h,v 1.9 2000/02/18 00:48:25 ekohl Exp $
 *
 */

VOID WINAPI __RtlInitHeap(PVOID	base,
			  ULONG	minsize,
			  ULONG	maxsize);

#define HEAP_BASE (0xa0000000)

VOID RtlDeleteCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);
VOID RtlEnterCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);
VOID RtlInitializeCriticalSection (LPCRITICAL_SECTION	pcritical);
VOID RtlLeaveCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);
WINBOOL RtlTryEntryCriticalSection (LPCRITICAL_SECTION	lpCriticalSection);

UINT
STDCALL
RtlCompactHeap (
	HANDLE	heap,
	DWORD	flags
	);

VOID
STDCALL
RtlEraseUnicodeString (
	IN	PUNICODE_STRING	String
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
	PVOID	*Environment
	);

VOID
STDCALL
RtlDestroyEnvironment (
	PVOID	Environment
	);

NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U (
	PVOID		Environment,
	PUNICODE_STRING	Source,
	PUNICODE_STRING	Destination,
	PULONG		Length
	);

NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PVOID		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

VOID
STDCALL
RtlSetCurrentEnvironment (
	PVOID	NewEnvironment,
	PVOID	*OldEnvironment
	);

NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PVOID		*Environment,
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
	IN OUT	PULONG			StackReserved,
	IN OUT	PULONG			StackCommit,
	IN	PTHREAD_START_ROUTINE	StartAddress,
	IN	PVOID			Parameter,
	IN OUT	PHANDLE			ThreadHandle,
	IN OUT	PCLIENT_ID		ClientId
	);

/*
 * Preliminary prototype!!
 *
 * This prototype is not finished yet. It will change in the future.
 */
NTSTATUS
STDCALL
RtlCreateUserProcess (
	PUNICODE_STRING			CommandLine,
	ULONG				Unknown1,
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	PSECURITY_DESCRIPTOR		ProcessSd,
	PSECURITY_DESCRIPTOR		ThreadSd,
	WINBOOL				bInheritHandles,
	DWORD				dwCreationFlags,
	PCLIENT_ID			ClientId,
	PHANDLE				ProcessHandle,
	PHANDLE				ThreadHandle
	);

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	*ProcessParameters,
	IN	PUNICODE_STRING	CommandLine,
	IN	PUNICODE_STRING	LibraryPath,
	IN	PUNICODE_STRING	CurrentDirectory,
	IN	PUNICODE_STRING	ImageName,
	IN	PVOID		Environment,
	IN	PUNICODE_STRING	Title,
	IN	PUNICODE_STRING	Desktop,
	IN	PUNICODE_STRING	Reserved,
	IN	PUNICODE_STRING	Reserved2
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

VOID
STDCALL
RtlDestroyProcessParameters (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams (
	IN OUT	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

/* EOF */
